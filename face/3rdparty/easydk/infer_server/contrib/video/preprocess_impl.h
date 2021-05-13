/*************************************************************************
 * Copyright (C) [2020] by Cambricon, Inc. All rights reserved
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *************************************************************************/

#ifndef INFER_SERVER_PREPROCESS_IMPL_H_
#define INFER_SERVER_PREPROCESS_IMPL_H_

#include <cn_codec_common.h>
#include <cnrt.h>
#include <glog/logging.h>

#include <algorithm>
#include <future>
#include <map>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include "easybang/resize_and_colorcvt.h"
#include "infer_server.h"
#include "model/model.h"
#include "processor.h"
#include "shape.h"
#include "util/threadsafe_queue.h"
#include "video_helper.h"

#ifdef CNIS_HAVE_CNCV
#include "cncv.h"
#endif

namespace infer_server {
namespace video {
namespace detail {

void ClipBoundingBox(BoundingBox* box) noexcept;

class PreprocessBase {
 public:
  virtual ~PreprocessBase() {}
  virtual bool Init(PixelFmt src_fmt, PixelFmt dst_fmt) noexcept = 0;
  virtual bool Execute(Package* pack, Buffer* output) = 0;
};  // class PreprocessBase

class ResizeConvert : virtual public PreprocessBase {
 public:
  ResizeConvert(ModelPtr model, int dev_id, edk::CoreVersion core_version, int core_number, bool keep_aspect_ratio)
      : model_(model),
        dev_id_(dev_id),
        core_version_(core_version),
        core_number_(core_number),
        keep_aspect_ratio_(keep_aspect_ratio) {}
  ~ResizeConvert() { op.Destroy(); }

  bool Init(PixelFmt src_fmt, PixelFmt dst_fmt) noexcept override;
  bool Execute(Package* pack, Buffer* output) override;
  int DeviceId() const noexcept { return dev_id_; }

 private:
  edk::MluResizeConvertOp op;
  ModelPtr model_;
  int dev_id_;
  edk::CoreVersion core_version_;
  int core_number_;
  bool keep_aspect_ratio_;
};  // class ResizeConvert

class ScalerWorker {
 public:
  static ScalerWorker* GetInstance(int device_id) noexcept {
    static std::map<int, std::unique_ptr<ScalerWorker>> instance_map;
    static std::mutex map_mutex;
    std::unique_lock<std::mutex> lk(map_mutex);
    if (!instance_map.count(device_id)) {
      instance_map.emplace(device_id, std::unique_ptr<ScalerWorker>(new ScalerWorker(device_id)));
    }
    return instance_map.at(device_id).get();
  }

  ~ScalerWorker() {
    is_done_ = true;
    cv_.notify_all();
    for (auto& th : th_) {
      if (th.joinable()) {
        th.join();
      }
    }
  }

  int DeviceId() const noexcept { return device_id_; }

  template <typename Callable>
  std::future<bool> PushTask(Callable&& f) noexcept {
    auto pck = std::make_shared<std::packaged_task<bool(int)>>(std::forward<Callable>(f));
    task_q_.Push(pck);
    cv_.notify_one();
    return pck->get_future();
  }

 private:
  explicit ScalerWorker(int device_id) noexcept : device_id_(device_id) {
    th_.reserve(2);
    th_.emplace_back(&ScalerWorker::WorkLoop, this, 0);
    th_.emplace_back(&ScalerWorker::WorkLoop, this, 1);
  }

  void WorkLoop(int instance_id) noexcept;

  std::vector<std::thread> th_;
  TSQueue<std::shared_ptr<std::packaged_task<bool(int)>>> task_q_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::atomic<bool> is_done_{false};
  int device_id_;
};  // class ScalerWorker

void ScalerWorker::WorkLoop(int instance_id) noexcept {
  std::shared_ptr<std::packaged_task<bool(int)>> t;
  bool have_task = task_q_.TryPop(t);
  while (true) {
    while (have_task) {
      (*t)(instance_id);
      t.reset();
      have_task = task_q_.TryPop(t);
    }

    std::unique_lock<std::mutex> lk(mutex_);
    cv_.wait(lk, [this, &have_task, &t]() {
      have_task = task_q_.TryPop(t);
      return have_task || is_done_;
    });

    if (!have_task) return;
  }
}

class Scaler : virtual public PreprocessBase {
 public:
  Scaler(Shape s, int dev_id) : shape_(s), dev_id_(dev_id) {}
  ~Scaler() {}

  bool Init(PixelFmt src_fmt, PixelFmt dst_fmt) noexcept override;
  bool Execute(Package* pack, Buffer* output) override;
  bool Process(VideoFrame* frame, Buffer* model_input, int instance_id, int batch_idx);

 private:
  Shape shape_;
  ScalerWorker* worker_{nullptr};
  cncodecPixelFormat src_fmt_;
  cncodecPixelFormat dst_fmt_;
  int dev_id_{0};
};  // class ResizeConvert

std::ostream& operator<<(std::ostream& os, PixelFmt fmt) noexcept { return os << static_cast<int>(fmt); }

bool ResizeConvert::Init(PixelFmt src_fmt, PixelFmt dst_fmt) noexcept {
  enum : int64_t {
#define JOINT(src, dst) (static_cast<int64_t>(src) << 32 | static_cast<int>(dst))
    YUV2RGBA_NV21 = JOINT(PixelFmt::NV21, PixelFmt::RGBA),
    YUV2RGBA_NV12 = JOINT(PixelFmt::NV12, PixelFmt::RGBA),

    YUV2BGRA_NV21 = JOINT(PixelFmt::NV21, PixelFmt::BGRA),
    YUV2BGRA_NV12 = JOINT(PixelFmt::NV12, PixelFmt::BGRA),

    YUV2ARGB_NV21 = JOINT(PixelFmt::NV21, PixelFmt::ARGB),
    YUV2ARGB_NV12 = JOINT(PixelFmt::NV12, PixelFmt::ARGB),

    YUV2ABGR_NV21 = JOINT(PixelFmt::NV21, PixelFmt::ABGR),
    YUV2ABGR_NV12 = JOINT(PixelFmt::NV12, PixelFmt::ABGR)
  } color_cvt_mode = static_cast<decltype(color_cvt_mode)>(JOINT(src_fmt, dst_fmt));
#undef JOINT

  static const std::map<decltype(color_cvt_mode), edk::MluResizeConvertOp::ColorMode> cvt_mode_map = {
      {YUV2RGBA_NV21, edk::MluResizeConvertOp::ColorMode::YUV2RGBA_NV21},
      {YUV2RGBA_NV12, edk::MluResizeConvertOp::ColorMode::YUV2RGBA_NV12},
      {YUV2BGRA_NV21, edk::MluResizeConvertOp::ColorMode::YUV2BGRA_NV21},
      {YUV2BGRA_NV12, edk::MluResizeConvertOp::ColorMode::YUV2BGRA_NV12},
      {YUV2ARGB_NV21, edk::MluResizeConvertOp::ColorMode::YUV2ARGB_NV21},
      {YUV2ARGB_NV12, edk::MluResizeConvertOp::ColorMode::YUV2ARGB_NV12},
      {YUV2ABGR_NV21, edk::MluResizeConvertOp::ColorMode::YUV2ABGR_NV21},
      {YUV2ABGR_NV12, edk::MluResizeConvertOp::ColorMode::YUV2ABGR_NV12}};

  LOG_IF(FATAL, cvt_mode_map.count(color_cvt_mode) == 0)
      << "Unsupport color convert mode. src pixel format : " << src_fmt << ", dst pixel format : " << dst_fmt;

  edk::MluResizeConvertOp::Attr op_attr;
  // image shape is always nhwc
  op_attr.dst_h = model_->InputShape(0)[1];
  op_attr.dst_w = model_->InputShape(0)[2];
  op_attr.color_mode = cvt_mode_map.at(color_cvt_mode);
  op_attr.batch_size = model_->BatchSize();
  op_attr.core_version = core_version_;
  op_attr.keep_aspect_ratio = keep_aspect_ratio_;
  op_attr.core_number = core_number_;
  if (!op.Init(op_attr)) {
    LOG(ERROR) << "Init resize convert op failed: " << op.GetLastError();
    return false;
  }
  return true;
}

bool ResizeConvert::Execute(Package* pack, Buffer* model_input) {
  for (size_t i = 0; i < pack->data.size(); i++) {
    VideoFrame& frame = pack->data[i]->GetLref<VideoFrame>();

    if (frame.format != PixelFmt::NV12 && frame.format != PixelFmt::NV21) {
      LOG(ERROR) << "Not supported!";
      return false;
    }
    edk::MluResizeConvertOp::InputData input_data;
    input_data.src_w = frame.width;
    input_data.src_h = frame.height;
    input_data.src_stride = frame.stride[0];
    input_data.planes[0] = frame.plane[0].MutableData();
    input_data.planes[1] = frame.plane[1].MutableData();

    ClipBoundingBox(&frame.roi);
    int32_t crop_x = frame.roi.x * frame.width;
    int32_t crop_y = frame.roi.y * frame.height;
    input_data.crop_x = crop_x;
    input_data.crop_y = crop_y;
    input_data.crop_w = frame.roi.w * frame.width;
    input_data.crop_h = frame.roi.h * frame.height;
    op.BatchingUp(input_data);
  }
  op.SyncOneOutput(model_input->MutableData());
  return true;
}

static cncodecPixelFormat CastPixelFormat(PixelFmt fmt) noexcept {
  switch (fmt) {
    case PixelFmt::NV12:
      return CNCODEC_PIX_FMT_NV12;
    case PixelFmt::NV21:
      return CNCODEC_PIX_FMT_NV21;
    case PixelFmt::ABGR:
      return CNCODEC_PIX_FMT_ABGR;
    case PixelFmt::ARGB:
      return CNCODEC_PIX_FMT_ARGB;
    case PixelFmt::BGRA:
      return CNCODEC_PIX_FMT_BGRA;
    case PixelFmt::RGBA:
      return CNCODEC_PIX_FMT_RGBA;
    default:
      LOG(ERROR) << "Unsupport pixel format";
      return CNCODEC_PIX_FMT_TOTAL_COUNT;
  }
}

bool Scaler::Init(PixelFmt src_fmt, PixelFmt dst_fmt) noexcept {
  worker_ = ScalerWorker::GetInstance(dev_id_);
  src_fmt_ = CastPixelFormat(src_fmt);
  dst_fmt_ = CastPixelFormat(dst_fmt);
  return true;
}

bool Scaler::Process(VideoFrame* frame, Buffer* model_input, int instance_id, int batch_idx) {
  CHECK(frame->plane[0].OnMlu()) << "memory is on CPU, which shoule be on MLU";
  void* src_y = frame->plane[0].MutableData();
  void* src_uv = frame->plane[1].MutableData();
  void* dst = (*model_input)(shape_.DataCount() * batch_idx).MutableData();
  cncodecWorkInfo work_info;
  cncodecFrame src_frame;
  cncodecFrame dst_frame;
  memset(&work_info, 0, sizeof(work_info));
  memset(&src_frame, 0, sizeof(src_frame));
  memset(&dst_frame, 0, sizeof(dst_frame));

  src_frame.pixelFmt = src_fmt_;
  src_frame.colorSpace = CNCODEC_COLOR_SPACE_BT_709;
  src_frame.width = frame->width;
  src_frame.height = frame->height;
  src_frame.planeNum = frame->plane_num;
  src_frame.stride[0] = frame->stride[0];
  src_frame.stride[1] = frame->stride[1];
  src_frame.plane[0].size = frame->stride[0] * frame->height;
  src_frame.plane[0].addr = reinterpret_cast<u64_t>(src_y);
  src_frame.plane[1].size = frame->stride[0] * frame->height >> 1;  // FIXME
  src_frame.plane[1].addr = reinterpret_cast<u64_t>(src_uv);
  src_frame.channel = 1;  // FIXME
  src_frame.deviceId = dev_id_;

  static auto align_to_128 = [](uint32_t x) { return (x + 127) & ~127; };
  auto row_align = align_to_128(shape_[2] * 4);
  dst_frame.width = shape_[2];
  dst_frame.height = shape_[1];
  dst_frame.pixelFmt = dst_fmt_;
  dst_frame.planeNum = 1;
  dst_frame.plane[0].size = row_align * shape_[1];
  dst_frame.stride[0] = row_align;
  dst_frame.plane[0].addr = reinterpret_cast<u64_t>(dst);

  work_info.inMsg.instance = instance_id;

  ClipBoundingBox(&frame->roi);
  cncodecRectangle roi;
  roi.left = frame->roi.x * src_frame.width;
  roi.top = frame->roi.y * src_frame.height;
  roi.right = (frame->roi.x + frame->roi.w) * src_frame.width;
  roi.bottom = (frame->roi.y + frame->roi.h) * src_frame.height;

  i32_t ret;
  if (roi.left == 0 && roi.top == 0 && roi.right == 0 && roi.bottom == 0) {
    ret = cncodecImageTransform(&dst_frame, nullptr, &src_frame, nullptr, CNCODEC_Filter_BiLinear, &work_info);
  } else {
    ret = cncodecImageTransform(&dst_frame, &roi, &src_frame, nullptr, CNCODEC_Filter_BiLinear, &work_info);
  }

  if (CNCODEC_SUCCESS != ret) {
    LOG(ERROR) << "scaler failed, error code:" << ret;
    return false;
  }
  return true;
}

bool Scaler::Execute(Package* pack, Buffer* model_input) {
  // submit task
  std::vector<std::future<bool>> res;
  res.reserve(pack->data.size());
  for (size_t batch_idx = 0; batch_idx < pack->data.size(); ++batch_idx) {
    VideoFrame& frame = pack->data[batch_idx]->GetLref<VideoFrame>();
    res.emplace_back(
        worker_->PushTask(std::bind(&Scaler::Process, this, &frame, model_input, std::placeholders::_1, batch_idx)));
  }

  // wait for task done
  bool ret = true;
  for (auto& it : res) {
    ret &= it.get();
  }

  return ret;
}

#ifdef CNIS_HAVE_CNCV
#define CNRT_SAFE_CALL(func, val)                                 \
  do {                                                            \
    cnrtRet_t ret = (func);                                       \
    if (ret != CNRT_RET_SUCCESS) {                                \
      LOG(ERROR) << "Call " #func " failed. error code: " << ret; \
      return val;                                                 \
    }                                                             \
  } while (0)
#define CNCV_SAFE_CALL(func, val)                                 \
  do {                                                            \
    cncvStatus_t ret = (func);                                    \
    if (ret != CNCV_STATUS_SUCCESS) {                             \
      LOG(ERROR) << "Call " #func " failed. error code: " << ret; \
      return val;                                                 \
    }                                                             \
  } while (0)

class CncvResizeConvert : virtual public PreprocessBase {
 public:
  CncvResizeConvert(ModelPtr model, int dev_id, bool keep_aspect_ratio = true, uint8_t pad_value = 0,
                    cncvColorSpace colorspace = CNCV_COLOR_SPACE_BT_601, cncvDepth_t depth = CNCV_DEPTH_8U)
      : model_(model),
        dev_id_(dev_id),
        keep_aspect_ratio_(keep_aspect_ratio),
        pad_value_(pad_value),
        colorspace_(colorspace),
        depth_(depth) {}
  ~CncvResizeConvert() {
    if (handle_) {
      cncvStatus_t ret = cncvDestroy(handle_);
      if (ret != CNCV_STATUS_SUCCESS) LOG(ERROR) << "cncvDestroy failed, error code: " << ret;
    }
    if (queue_) {
      cnrtRet_t ret = cnrtDestroyQueue(queue_);
      if (ret != CNRT_RET_SUCCESS) LOG(ERROR) << "cnrtDestroyQueue failed, error code: " << ret;
    }
  }
  bool Init(PixelFmt src_fmt, PixelFmt dst_fmt) noexcept override;
  bool Execute(Package* pack, Buffer* output) override;
  int DeviceId() const noexcept { return dev_id_; }

 private:
  cncvPixelFormat GetCncvPixFmt(PixelFmt fmt);
  uint32_t GetCncvDepthSize(cncvDepth_t depth);
  void SetStride(cncvImageDescriptor* desc);
  void KeepAspectRatio(cncvRect* dst_roi, const cncvImageDescriptor& src, const cncvImageDescriptor& dst);

 private:
  ModelPtr model_;
  int dev_id_;
  bool keep_aspect_ratio_;
  uint8_t pad_value_ = 0;
  size_t batch_size_ = 1;
  cncvColorSpace colorspace_;
  cncvDepth_t depth_ = CNCV_DEPTH_8U;

  Buffer workspace_;
  Buffer mlu_input_y_;
  Buffer mlu_input_uv_;
  Buffer mlu_output_;
  Buffer cpu_input_y_;
  Buffer cpu_input_uv_;
  Buffer cpu_output_;

  std::vector<cncvImageDescriptor> src_descs_;
  std::vector<cncvImageDescriptor> dst_descs_;
  std::vector<cncvRect> src_rois_;
  std::vector<cncvRect> dst_rois_;
  cnrtQueue_t queue_ = nullptr;
  cncvHandle_t handle_ = nullptr;
  size_t workspace_size_ = 0;
};

uint32_t CncvResizeConvert::GetCncvDepthSize(cncvDepth_t depth) {
  switch (depth) {
    case CNCV_DEPTH_8U:
    case CNCV_DEPTH_8S:
      return 1;
    case CNCV_DEPTH_16U:
    case CNCV_DEPTH_16S:
    case CNCV_DEPTH_16F:
      return 2;
    case CNCV_DEPTH_32U:
    case CNCV_DEPTH_32S:
    case CNCV_DEPTH_32F:
      return 4;
    default:
      LOG(ERROR) << "Unsupport Depth, Size = 0 by default.";
      return 0;
  }
}

cncvPixelFormat CncvResizeConvert::GetCncvPixFmt(PixelFmt fmt) {
  switch (fmt) {
    case PixelFmt::I420:
      return CNCV_PIX_FMT_I420;
    case PixelFmt::NV12:
      return CNCV_PIX_FMT_NV12;
    case PixelFmt::NV21:
      return CNCV_PIX_FMT_NV21;
    case PixelFmt::BGR24:
      return CNCV_PIX_FMT_BGR;
    case PixelFmt::RGB24:
      return CNCV_PIX_FMT_RGB;
    case PixelFmt::BGRA:
      return CNCV_PIX_FMT_BGRA;
    case PixelFmt::RGBA:
      return CNCV_PIX_FMT_RGBA;
    case PixelFmt::ABGR:
      return CNCV_PIX_FMT_ABGR;
    case PixelFmt::ARGB:
      return CNCV_PIX_FMT_ARGB;
    default:
      LOG(ERROR) << "Unsupport input format, error occurs in CncvResizeConvert";
      return CNCV_PIX_FMT_INVALID;
  }
}

void CncvResizeConvert::SetStride(cncvImageDescriptor* desc) {
  int depth = GetCncvDepthSize(desc->depth);
  switch (desc->pixel_fmt) {
    case CNCV_PIX_FMT_I420:
      desc->stride[0] = depth * desc->width;
      desc->stride[1] = depth * desc->width / 2;
      desc->stride[2] = depth * desc->width / 2;
      break;
    case CNCV_PIX_FMT_NV12:
    case CNCV_PIX_FMT_NV21:
      desc->stride[0] = depth * desc->width;
      desc->stride[1] = depth * desc->width;
      break;
    case CNCV_PIX_FMT_BGR:
    case CNCV_PIX_FMT_RGB:
      desc->stride[0] = depth * desc->width * 3;
      break;
    case CNCV_PIX_FMT_BGRA:
    case CNCV_PIX_FMT_RGBA:
    case CNCV_PIX_FMT_ABGR:
    case CNCV_PIX_FMT_ARGB:
      desc->stride[0] = depth * desc->width * 4;
      break;
    default:
      LOG(ERROR) << "Unsupport input format, error occurs in CncvResizeConvert";
      return;
  }
}

void CncvResizeConvert::KeepAspectRatio(cncvRect* dst_roi, const cncvImageDescriptor& src,
                                        const cncvImageDescriptor& dst) {
  float src_ratio = static_cast<float>(src.width) / src.height;
  float dst_ratio = static_cast<float>(dst.width) / dst.height;
  if (src_ratio < dst_ratio) {
    int pad_lenth = dst.width - src_ratio * dst.height;
    pad_lenth = pad_lenth % 2 ? pad_lenth - 1 : pad_lenth;
    if (dst.width - pad_lenth / 2 < 0) return;
    dst_roi->w = dst.width - pad_lenth;
    dst_roi->x = pad_lenth / 2;
    dst_roi->y = 0;
    dst_roi->h = dst.height;
  } else if (src_ratio > dst_ratio) {
    int pad_lenth = dst.height - dst.width / src_ratio;
    pad_lenth = pad_lenth % 2 ? pad_lenth - 1 : pad_lenth;
    if (dst.height - pad_lenth / 2 < 0) return;
    dst_roi->h = dst.height - pad_lenth;
    dst_roi->y = pad_lenth / 2;
    dst_roi->x = 0;
    dst_roi->w = dst.width;
  }
}

bool CncvResizeConvert::Init(PixelFmt src_fmt, PixelFmt dst_fmt) noexcept {
  batch_size_ = model_->BatchSize();
  if (src_fmt != PixelFmt::NV12 && src_fmt != PixelFmt::NV21) {
    LOG(ERROR) << "Not supported input format!";
    return false;
  }
  dst_descs_.resize(batch_size_);
  src_descs_.resize(batch_size_);
  src_rois_.resize(batch_size_);
  dst_rois_.resize(batch_size_);
  // init descriptor params
  for (size_t i = 0; i < batch_size_; ++i) {
    // dst desc
    dst_descs_[i].pixel_fmt = GetCncvPixFmt(dst_fmt);
    dst_descs_[i].height = model_->InputShape(0)[1];
    dst_descs_[i].width = model_->InputShape(0)[1];
    dst_descs_[i].depth = depth_;
    SetStride(&dst_descs_[i]);
    dst_descs_[i].color_space = colorspace_;

    // src desc
    src_descs_[i].pixel_fmt = GetCncvPixFmt(src_fmt);
    src_descs_[i].color_space = colorspace_;
    src_descs_[i].depth = depth_;
  }

  size_t ptr_size = batch_size_ * sizeof(void*);
  mlu_input_y_ = Buffer(ptr_size, dev_id_);
  mlu_input_uv_ = Buffer(ptr_size, dev_id_);
  mlu_output_ = Buffer(ptr_size, dev_id_);
  cpu_input_y_ = Buffer(ptr_size);
  cpu_input_uv_ = Buffer(ptr_size);
  cpu_output_ = Buffer(ptr_size);

  // init mlu context
  if (handle_ == nullptr) {
    CNRT_SAFE_CALL(cnrtCreateQueue(&queue_), false);
    CNCV_SAFE_CALL(cncvCreate(&handle_), false);
    CNCV_SAFE_CALL(cncvSetQueue(handle_, queue_), false);
  }
  return true;
}

bool CncvResizeConvert::Execute(Package* pack, Buffer* output) {
  auto batch_size = pack->data.size();
  size_t ptr_size = batch_size * sizeof(void*);
  // init mlu buff
  void* mlu_input_y_ptr = mlu_input_y_.MutableData();
  void* mlu_input_uv_ptr = mlu_input_uv_.MutableData();
  void* mlu_output_ptr = mlu_output_.MutableData();
  CNRT_SAFE_CALL(cnrtMemset(output->MutableData(), pad_value_, output->MemorySize()), false);

  // init cpu ptr
  void** cpu_input_y_ptr = reinterpret_cast<void**>(cpu_input_y_.MutableData());
  void** cpu_input_uv_ptr = reinterpret_cast<void**>(cpu_input_uv_.MutableData());
  void** cpu_output_ptr = reinterpret_cast<void**>(cpu_output_.MutableData());
  size_t output_offset = 0;

  // init src_decs_ and rects and cpu ptr
  for (size_t i = 0; i < batch_size; ++i) {
    VideoFrame& frame = pack->data[i]->GetLref<VideoFrame>();
    if (frame.format != PixelFmt::NV12 && frame.format != PixelFmt::NV21) {
      LOG(ERROR) << "Not supported!";
      return false;
    }
    // init cpu ptr
    cpu_input_y_ptr[i] = frame.plane[0].MutableData();
    cpu_input_uv_ptr[i] = frame.plane[1].MutableData();
    cpu_output_ptr[i] = reinterpret_cast<void*>(reinterpret_cast<int64_t>(output->MutableData()) + output_offset);
    output_offset += dst_descs_[i].stride[0] * dst_descs_[i].height;

    // init src descs
    src_descs_[i].width = frame.width;
    src_descs_[i].height = frame.height;
    if (frame.stride[0] > 1) {
      src_descs_[i].stride[0] = frame.stride[0];
      src_descs_[i].stride[1] = frame.stride[1];
      src_descs_[i].stride[2] = frame.stride[2];
    } else {
      SetStride(&src_descs_[i]);
    }

    // init dst rect
    dst_rois_[i].x = 0;
    dst_rois_[i].y = 0;
    dst_rois_[i].w = dst_descs_[i].width;
    dst_rois_[i].h = dst_descs_[i].height;

    // init src rect
    ClipBoundingBox(&frame.roi);
    int32_t crop_x = frame.roi.x * frame.width;
    int32_t crop_y = frame.roi.y * frame.height;
    src_rois_[i].x = crop_x;
    src_rois_[i].y = crop_y;
    src_rois_[i].w = frame.roi.w ? frame.roi.w : 1 * frame.width;
    src_rois_[i].h = frame.roi.h ? frame.roi.h : 1 * frame.height;
    if (keep_aspect_ratio_) {
      KeepAspectRatio(&dst_rois_[i], src_descs_[i], dst_descs_[i]);
    }
  }

  // copy cpu ptr to mlu ptr
  mlu_input_y_.CopyFrom(cpu_input_y_, ptr_size);
  mlu_input_uv_.CopyFrom(cpu_input_uv_, ptr_size);
  mlu_output_.CopyFrom(cpu_output_, ptr_size);

  size_t current_workspace_size = 0;
  // use batch_size from model to reduce remalloc times
  cncvGetResizeConvertWorkspaceSize(batch_size_, src_descs_.data(), src_rois_.data(), dst_descs_.data(),
                                    dst_rois_.data(), &current_workspace_size);

  // prepare workspace
  if (!workspace_.OwnMemory() || workspace_size_ < current_workspace_size) {
    workspace_size_ = current_workspace_size;
    workspace_ = Buffer(workspace_size_, dev_id_);
  }

  // compute
  CNCV_SAFE_CALL(
      cncvResizeConvert(handle_, batch_size,
                        src_descs_.data(), src_rois_.data(),
                        reinterpret_cast<void**>(mlu_input_y_ptr),
                        reinterpret_cast<void**>(mlu_input_uv_ptr),
                        dst_descs_.data(), dst_rois_.data(),
                        reinterpret_cast<void**>(mlu_output_ptr),
                        workspace_size_, workspace_.MutableData(),
                        CNCV_INTER_BILINEAR),
      false);
  CNRT_SAFE_CALL(cnrtSyncQueue(queue_), false);
  return true;
}
#endif  // CNIS_HAVE_CNCV

}  // namespace detail
}  // namespace video
}  // namespace infer_server

#endif  // INFER_SERVER_PREPROCESS_IMPL_H_
