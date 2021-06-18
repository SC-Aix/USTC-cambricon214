#include <iostream>
#include <thread>

#include "ffmpeg.hpp"
#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
}
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

namespace facealign {

static int InterruptCallBack(void* ctx) {
  VideoParser* parser = reinterpret_cast<VideoParser*>(ctx);
  if (parser->CheckTimeOut()) {
    std::cerr << "[Rtsp] Get interrupt!" << std::endl;
    return 1;
  }
  return 0;
}

bool VideoParser::CheckTimeOut() {
  std::chrono::duration<float, std::milli> dura =
    std::chrono::steady_clock::now() - last_receive_frame_time_;
  if (dura.count() > max_recevie_timeout_) {
    return true;
  }
  return false;
}

bool VideoParser::Open(const char *path) {
  static struct _InitFFmpeg {
    _InitFFmpeg() {
      avcodec_register_all();
      av_register_all();
      avformat_network_init();
    }
  } _init_ffmpeg;

  if (have_video_source_.load()) return false;
  p_format_ctx_ = avformat_alloc_context();
  if (!p_format_ctx_) {
    std::cerr << "alloc av_format_context failed!" << std::endl;
  }
  if (is_rtsp_) {
    AVIOInterruptCB inbk = {InterruptCallBack, this};
    p_format_ctx_->interrupt_callback = inbk;
    last_receive_frame_time_ = std::chrono::steady_clock::now();
    // options
    av_dict_set(&options_, "buffer_size", "1024000", 0);
    av_dict_set(&options_, "max_delay", "500000", 0);
    av_dict_set(&options_, "stimeout", "20000000", 0);
    av_dict_set(&options_, "rtsp_flags", "prefer_tcp", 0);
  } else {
    av_dict_set(&options_, "buffer_size", "1024000", 0);
    av_dict_set(&options_, "max_delay", "200000", 0);
  }
  std::cout << "path: " << path  << std::endl;
  int ret_code = avformat_open_input(&p_format_ctx_, path, NULL, &options_);
  if (0 != ret_code) {
    std::cerr << "open input stream failed!" << std::endl;
    return false;
  }

  ret_code = avformat_find_stream_info(p_format_ctx_, NULL);
  if (0 > ret_code) {
    std::cerr << "find stream info failed!" << std::endl;
    return false;
  }
  
  video_index_ = -1;
  AVStream *vstream = nullptr;
  for (uint32_t iloop = 0; iloop < p_format_ctx_->nb_streams; iloop++) {
    vstream = p_format_ctx_->streams[iloop];
    if (vstream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      video_index_ = iloop;
      break;
    }
  }

  if (video_index_ == -1) {
    std::cerr << "didn`t find a video frame" << std::endl;
  }

  info_.width = vstream->codec->width;
  info_.height = vstream->codec->height;

  auto codec_id = vstream->codecpar->codec_id;
  int filed_order = vstream->codecpar->field_order;

  switch(filed_order) {
    case AV_FIELD_TT:  
    case AV_FIELD_BB:
    case AV_FIELD_TB:
    case AV_FIELD_BT:
      info_.progressive = 0;
      break;
    case AV_FIELD_PROGRESSIVE:
    default:
      info_.progressive = 0;
      break;
  }

  uint8_t* extra_data = vstream->codecpar->extradata;
  int extra_datasize = vstream->codecpar->extradata_size;
  info_.extra_data = std::vector<uint8_t>(extra_data, extra_data + extra_datasize);

  p_bsfc_ = nullptr;
  // LOGI(SAMPLES) << p_format_ctx_->iformat->name;                                                                     
  if (strstr(p_format_ctx_->iformat->name, "mp4") || strstr(p_format_ctx_->iformat->name, "flv") ||
    strstr(p_format_ctx_->iformat->name, "matroska") || strstr(p_format_ctx_->iformat->name, "h264") ||
    strstr(p_format_ctx_->iformat->name, "rtsp")) {
    if (AV_CODEC_ID_H264 == codec_id) {
      p_bsfc_ = av_bitstream_filter_init("h264_mp4toannexb");
      info_.codec_type = edk::CodecType::H264;
      // if (save_file) saver_.reset(new detail::FileSaver("out.h264"));                                                
    }
    else if (AV_CODEC_ID_HEVC == codec_id) {
      p_bsfc_ = av_bitstream_filter_init("hevc_mp4toannexb");
      info_.codec_type = edk::CodecType::H265;
      //  if (save_file) saver_.reset(new detail::FileSaver("out.h265"));                                                
    }
    else {
      //  LOGE(SAMPLES) << "nonsupport codec id.";                                                                       
      return false;
    }
  }

  have_video_source_.store(true);
  first_frame_ = true;
  return true;
}

void VideoParser::Close() {
  if (!have_video_source_.load()) return;
  std::cout << "Close ffmpeg resources!" << std::endl;
  if (p_format_ctx_) {
    avformat_close_input(&p_format_ctx_);
    avformat_free_context(p_format_ctx_);
    av_dict_free(&options_);
    p_format_ctx_ = nullptr;
    options_= nullptr;
  }

  if (p_bsfc_) {
    av_bitstream_filter_close(p_bsfc_);
    frame_index_ = 0;
  }
}

int VideoParser::ParseLoop(uint32_t frame_interval) {
  if (!info_.extra_data.empty()) {
    edk::CnPacket pkt;
    pkt.data = const_cast<void*>(reinterpret_cast<const void*>(info_.extra_data.data()));
    pkt.length = info_.extra_data.size();
    pkt.pts = 0;
    if (!handler_->OnPacket(pkt)) {
      THROW_EXCEPTION(edk::Exception::INTERNAL, "send stream extra data failed!");
    }
  }

  auto now_time = std::chrono::steady_clock::now();
  auto last_time = std::chrono::steady_clock::now();
  std::chrono::duration<double, std::milli> dura;

  while (handler_->Running()) {
    if (!have_video_source_.load()) {
      std::cerr << "video source have been not init !" << std::endl;
      return -1;
    }

    if (av_read_frame(p_format_ctx_, &packet_) < 0) {
      handler_->OnEos();
    }

    last_receive_frame_time_ = std::chrono::steady_clock::now();

    if (packet_.stream_index != video_index_) {
      av_packet_unref(&packet_);
      continue;
    }

    if (first_frame_) {
      if (packet_.flags & AV_PKT_FLAG_KEY) {
        first_frame_ = false;
      } else {
        av_packet_unref(&packet_);
        continue;
      }
    }

    edk::CnPacket pkt;
    auto vstream = p_format_ctx_->streams[video_index_];

    if (p_bsfc_) {
      av_bitstream_filter_filter(p_bsfc_, vstream->codec, NULL, 
      reinterpret_cast<uint8_t **>(&pkt.data), reinterpret_cast<int *>(&pkt.length),
      packet_.data, packet_.size, 0);
    } else {
      pkt.data = packet_.data;
      pkt.length = packet_.size;
    }
   
    if (AV_NOPTS_VALUE == packet_.pts) {
      pkt.pts = frame_index_++;
    }
    else if (AV_NOPTS_VALUE != packet_.pts) {
      packet_.pts = av_rescale_q(packet_.pts, vstream->time_base, { 1, 90000 });
      pkt.pts = packet_.pts;
    }

    if (!handler_->OnPacket(pkt)) {
      // free packet                                                                                                  
      if (p_bsfc_) { //为什么这里是判断fiter的上下文。
        av_freep(&pkt.data);
      }
      av_packet_unref(&packet_);
      return 1;
    }

    // free packet                                                                                                  
    if (p_bsfc_) { //为什么这里是判断fiter的上下文。
      av_freep(&pkt.data);
    }
    av_packet_unref(&packet_);

    // frame rate control                                                                                           
    if (frame_interval) {
      now_time = std::chrono::steady_clock::now();
      dura = now_time - last_time;
      if (frame_interval > dura.count()) {
        std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(frame_interval - dura.count()));
      }
      last_time = std::chrono::steady_clock::now();
    }
  }  // while (true)                                                                                          
   return 1;                   
  }
}


