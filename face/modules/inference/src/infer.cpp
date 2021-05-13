
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"

#define CLIP(x) ((x) < 0 ? 0 : ((x) > 1 ? 1 : (x)))

namespace facealign {

//bool Inference::Preprocess() {

//}

bool Inference::Postprocess(const std::vector<float*> &net_outputs, 
                            const infer_server::ModelPtr &model,
                            std::shared_ptr<FAFrameInfo> frame_info) {
  if (net_outputs.size() != 9) {
    std::cerr << "netoutput is wrong!" << std::endl; 
    return false;
  }

  auto frame = reinterpret_cast<FrameOpencv*>(frame_info->datas[0].get());
  int frame_width = frame->image_ptr->rows; // 是否包含depth
  int frame_height = frame->image_ptr->cols;
  float ratio_w = float(frame_width) / float(model->InputShape(0).GetW());
  float ratio_h = float(frame_height) / float(model->InputShape(0).GetH());
  std::vector<AnchorGenerator> ac(_feat_stride_fpn.size());
  for (unsigned i = 0; i < _feat_stride_fpn.size(); ++i) {
    int stride = _feat_stride_fpn[i];
    ac[i].Init(stride, anchor_cfg[stride], false);
  }
  std::vector<Anchor> proposals;
  proposals.clear();

  for (unsigned i = 0; i < _feat_stride_fpn.size(); ++i) {
    int cls_idx = i * 3;
    int reg_idx = i * 3 + 1;
    int pts_idx = i * 3 + 2;
    FeatureMap cls, reg, pts;
    cls.dataPtr = net_outputs[cls_idx];
    reg.dataPtr = net_outputs[reg_idx];
    pts.dataPtr = net_outputs[pts_idx];
    cls.shape = model->OutputShape(cls_idx);
    reg.shape = model->OutputShape(reg_idx);
    pts.shape = model->OutputShape(pts_idx);
    ac[i].FilterAnchor(&cls, &reg, &pts, proposals, ratio_w, ratio_h, 0.8);
  }
  std::vector<Anchor> result;
  nms_cpu(proposals, 0.4, result);
  for (auto anchor : result) {
    // left > 45 face
    if (anchor.pts[2].x < anchor.pts[0].x) {
      continue;
    }
    // right > 45 face
    if (anchor.pts[2].x > anchor.pts[1].x) {
      continue;
    }
    
    frame->detect_obj.score = anchor.score;
    //obj->id = "0";
    frame->detect_obj.x = CLIP(anchor.finalbox.x / frame_width);
    frame->detect_obj.y = CLIP(anchor.finalbox.y / frame_height);
    frame->detect_obj.w = CLIP(anchor.finalbox.width / frame_width) - CLIP(anchor.finalbox.x / frame_width);
    frame->detect_obj.h = CLIP(anchor.finalbox.height / frame_height) - CLIP(anchor.finalbox.y / frame_height);
  /*
    // pts
    std::vector<float> face_pts;
    for (auto &pts : anchor.pts) {
      face_pts.push_back(pts.x);
      face_pts.push_back(pts.y);
    }
    obj->AddFeature("face_pts", face_pts);

    std::vector<float> landmark_location;
    float temp_z = (obj->bbox.w * obj->bbox.y / 60 / 60 - 1);
    float face_area_score = 1.0 / (1 + expf(-temp_z));
    float face_width = anchor.finalbox.width - anchor.finalbox.x;
    float eye_middle = (anchor.pts[0].x + anchor.pts[1].x) / 2.0;
    float mouth_middle = (anchor.pts[3].x + anchor.pts[4].x) / 2.0;
    float face_middle = fabsf(anchor.finalbox.width + anchor.finalbox.x) / 2;
    float ratio_eye_face = fabsf(anchor.pts[1].x - anchor.pts[0].x) / face_width;
    float ratio_mouth_face = fabsf(anchor.pts[4].x - anchor.pts[3].x) / face_width;
    float nose_eye_diff = -fabsf(eye_middle - anchor.pts[2].x) / face_width;
    float nose_mouth_diff = -fabsf(mouth_middle - anchor.pts[2].x) / face_width;
    float eye_face_diff = -fabsf(eye_middle - face_middle) / face_width;
    float mouth_face_diff = -fabsf(mouth_middle - face_middle) / face_width;
    float location_score = face_area_score + ratio_eye_face + ratio_mouth_face + nose_eye_diff + nose_mouth_diff +
                           eye_face_diff + mouth_face_diff;
    landmark_location.push_back(face_area_score);
    landmark_location.push_back(ratio_eye_face);
    landmark_location.push_back(ratio_mouth_face);
    landmark_location.push_back(nose_eye_diff);
    landmark_location.push_back(nose_mouth_diff);
    landmark_location.push_back(eye_face_diff);
    landmark_location.push_back(mouth_face_diff);
    landmark_location.push_back(location_score);

    obj->AddFeature("landmark_location", landmark_location);
    objs_ptr->objs_.push_back(obj);

#if 0
    // save face image
    auto& image = *frame->ImageBGR();
    static int filenumber = 0;
    std::stringstream ssfn;
    std::string filename;
    ssfn << "./output/frameid_" << frame->frame_id << "_imagenum_" << filenumber << ".jpg";
    filename = ssfn.str();
    cv::Rect face;
    face.x = obj->bbox.x * frame_width;
    face.y = obj->bbox.y * frame_height;
    face.width = obj->bbox.w * frame_width;
    face.height = obj->bbox.h * frame_height;
    cv::imwrite(filename, image(face));

    // save origin image
    ssfn.clear();
    ssfn.str("");
    ssfn << "./output/frameid_" << frame->frame_id << "_imagenum_" << filenumber << "_origin.jpg";
    filename = ssfn.str();
    cv::imwrite(filename, image);

    filenumber++;

    // save face landmarks to file
    std::string landmark_fname = "./output/landmarks.txt";
    std::fstream file_stream;
    file_stream.open(landmark_fname, std::ios::out | std::ios::app);
    file_stream << frame->frame_id << "," << anchor.finalbox.x << "," << anchor.finalbox.y << ","
                << anchor.finalbox.width << "," << anchor.finalbox.height << ",";
    for (auto landmark : anchor.pts) {
      file_stream << landmark.x << "," << landmark.y << ",";
    }
    file_stream << "\n";
    file_stream.close();
#endif
  }
*/

  // add face boxes and landmarks pts to frame
  auto image = frame->image_ptr.get();
  for (size_t i = 0; i < result.size(); i++) {
    cv::rectangle(*image, cv::Point((int)result[i].finalbox.x, (int)result[i].finalbox.y), \
                  cv::Point((int)result[i].finalbox.width, (int)result[i].finalbox.height), cv::Scalar(0, 255, 255), 2, \
                  8, 0);
    cv::circle(*image, result[i].pts[0], 10, cv::Scalar(0, 255, 0), -1);
    cv::circle(*image, result[i].pts[1], 10, cv::Scalar(0, 255, 0), -1);
    cv::circle(*image, result[i].pts[2], 10, cv::Scalar(0, 255, 0), -1);
    cv::circle(*image, result[i].pts[3], 10, cv::Scalar(0, 255, 0), -1);
    cv::circle(*image, result[i].pts[4], 10, cv::Scalar(0, 255, 0), -1);
  }
  }
  //package->datas[cnstream::CNObjsVecKey] = objs;
  
  return 0;

}
struct InferParam {
};

bool Inference::DetectCpuPostproc(infer_server::PackagePtr package, const infer_server::ModelPtr& model_info, std::shared_ptr<FAFrameInfo> data_) {
  std::vector<std::vector<float>> one_output;
  std::vector<float*> net_outputs_ptr;
  for (auto one_img_data : package->data) {
    for (size_t output_number_idx = 0; output_number_idx < model_info->OutputNum(); output_number_idx++) {
      infer_server::ModelIO model_output = infer_server::any_cast<infer_server::ModelIO>(one_img_data->data);
      float* net_output = reinterpret_cast<float*>(model_output.buffers[output_number_idx].MutableData());
      float* tmp_ptr = new float[model_info->OutputShape(output_number_idx).DataCount() * sizeof(float)];
      memcpy(tmp_ptr, net_output, model_info->OutputShape(output_number_idx).DataCount() * sizeof(float));
      std::shared_ptr<float*> tmp = std::make_shared<float*>(tmp_ptr);
      net_outputs_ptr.push_back(tmp_ptr);
    }
  }
  Postprocess(net_outputs_ptr, model_info, data_);

  return true;
}

bool Inference::MoveDataPostproc(infer_server::InferData* output_data, const infer_server::ModelIO& model_output) {
  output_data->Set(model_output);
  return true;
}

bool Inference::Open(ModuleParamSet parameters) {
  if (parameters.find("model_path") == parameters.end()) {
    std::cout << "model path is not set" << std::endl;
    return false;
  } else {
    model_path = parameters["model_path"];
  }
  return true;
}

infer_server::Session_t Inference::CreateSession() {
  infer_server::SessionDesc desc;
  desc.name = "detect"; // (TODO) this can set by json
  desc.strategy = infer_server::BatchStrategy::STATIC;
  desc.engine_num = 1;
  desc.show_perf = false;
  desc.priority = 0;
  desc.model = model_ptr;
  desc.host_input_layout = {infer_server::DataType::FLOAT32, infer_server::DimOrder::NHWC};
  desc.host_output_layout = {infer_server::DataType::FLOAT32, infer_server::DimOrder::NCHW};
  desc.preproc = std::make_shared<infer_server::Postprocessor>();
 // auto preproc_func = std::bind();
  desc.preproc->SetParams("process_function", infer_server::video::DefaultOpencvPreproc::GetFunction());
  auto postproc_func = std::bind(&Inference::MoveDataPostproc, this, std::placeholders::_1,
                                 std::placeholders::_2);
  desc.postproc->SetParams<infer_server::Postprocessor::ProcessFunction>("process_function", postproc_func);
  return server_->CreateSyncSession(desc);
}

bool Inference::Init() {
  server_.reset(new infer_server::video::VideoInferServer(dev_id));
  model_ptr = server_->LoadModel(model_path, model_name);

  //create session
  session_ptr = CreateSession();
  if (!session_ptr) {
    std::cerr << "*************create session failed!*************" << std::endl;
    return false;
  }
  return true;
}

int Inference::Process(std::shared_ptr<FAFrameInfo> data) {
  infer_server::Status status = infer_server::Status::SUCCESS;
  infer_server::PackagePtr detect_input_package = std::make_shared<infer_server::Package>();
  detect_input_package->data.push_back(std::make_shared<infer_server::InferData>());
  auto frame = reinterpret_cast<FrameOpencv*>(data->datas[0].get());
  cv::Mat& frame_cv = *(frame->image_ptr.get());
  detect_input_package->data[0]->Set(&frame_cv);
  //detect_input_package->tag = data_->stream_id;
  infer_server::PackagePtr detect_output_package = std::make_shared<infer_server::Package>();
  server_->InferServer::RequestSync(session_ptr, detect_input_package, &status, detect_output_package);
  if (infer_server::Status::SUCCESS != status) {
    std::cerr << "[CalculateFeature] Run detector offline model failed.";
    return -1;
  }
  DetectCpuPostproc(detect_output_package, model_ptr, data);
  return 0;
}

void Inference::Close() {
 if (server_) {
    if (session_ptr) {
      server_->DestroySession(session_ptr);
      session_ptr = nullptr;
    }
 }
 model_ptr.reset();
}

}

