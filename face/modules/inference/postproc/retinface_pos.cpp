#include "retinface_pos.hpp"


namespace facealign {
#define CLIP(x) ((x) < 0 ? 0 : ((x) > 1 ? 1 : (x)))

bool PostprocRetin::Postprocess(const std::vector<float*> &net_outputs, 
                            const infer_server::ModelPtr &model,
                            std::shared_ptr<FAFrameInfo> frame_info) {
  if (net_outputs.size() != 9) {
    std::cerr << "netoutput is wrong!" << std::endl; 
    return false;
  }

  auto frame = reinterpret_cast<FrameOpencv*>(frame_info->datas[0].get());
  int frame_width = frame->image_ptr->cols; // 是否包含depth
  int frame_height = frame->image_ptr->rows;
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
  auto image = frame->image_ptr.get();

  for (auto anchor : result) {
    // left > 45 face
    if (anchor.pts[2].x < anchor.pts[0].x) {
      continue;
    }
    // right > 45 face
    if (anchor.pts[2].x > anchor.pts[1].x) {
      continue;
    }
    
    std::shared_ptr<DetectObjs> obj_ptr(new DetectObjs());
    obj_ptr->score = anchor.score;
    //obj->id = "0";
    obj_ptr->x = CLIP(anchor.finalbox.x / frame_width);
    obj_ptr->y = CLIP(anchor.finalbox.y / frame_height);
    obj_ptr->w = CLIP(anchor.finalbox.width / frame_width) - CLIP(anchor.finalbox.x / frame_width);
    obj_ptr->h = CLIP(anchor.finalbox.height / frame_height) - CLIP(anchor.finalbox.y / frame_height);

    // save feature point
    for (int i = 0; i < 3; ++i) {
      cv::Point2f tmp(anchor.pts[i].x - anchor.finalbox.x, anchor.pts[i].y - anchor.finalbox.y);
      obj_ptr->fp.emplace_back(std::move(tmp));
    }


    int b_x = std::max(anchor.finalbox.y, 0.0f);
    int b_x_ = std::min( anchor.finalbox.height, 720.0f);
    int b_y = std::max((int)(anchor.finalbox.x), 0);
    int b_y_ = std::min((int)( anchor.finalbox.width) , 1280);
    

    // TODO 是否需要clone处理，防止后续对图片的操作影响obj中的数据成员
    obj_ptr->obj = (*image)(cv::Range(b_x, b_x_),cv::Range(b_y, b_y_));
   // imshow("123", obj_ptr->obj);
   // cv::waitKey(30);

    frame->detect_objs.emplace_back(std::move(obj_ptr));

  }
  // add face boxes and landmarks pts to frame
  
  for (size_t i = 0; i < result.size(); i++) {
    cv::rectangle(*image, cv::Point((int)(result[i].finalbox.x), (int)(result[i].finalbox.y)), \
                  cv::Point((int)result[i].finalbox.width, (int)result[i].finalbox.height), cv::Scalar(0, 255, 255), 2, \
                  8, 0);
    /*
    int b_x = std::max(result[i].finalbox.y, 0.0f);
    int b_x_ = std::min( result[i].finalbox.height, 720.0f);
    int b_y = std::max((int)(result[i].finalbox.x), 0);
    int b_y_ = std::min((int)( result[i].finalbox.width) , 1280);
    
    cv::Mat img = (*image)(cv::Range(b_x, b_x_),cv::Range(b_y, b_y_));
    imshow("123", img);
    cv::waitKey(30);
    std::cout  << " x : " << result[i].finalbox.x +  result[i].finalbox.width << std::endl;
    std::cout  << " y : " << result[i].finalbox.y + result[i].finalbox.height << std::endl;
    */
   // cv::circle(*image, result[i].pts[0], 10, cv::Scalar(0, 255, 0), -1);
   // cv::circle(*image, result[i].pts[1], 10, cv::Scalar(0, 255, 0), -1);
   // cv::circle(*image, result[i].pts[2], 10, cv::Scalar(0, 255, 0), -1);
   // cv::circle(*image, result[i].pts[3], 10, cv::Scalar(0, 255, 0), -1);
    //cv::circle(*image, result[i].pts[4], 10, cv::Scalar(0, 255, 0), -1);
  }
  
  return 0;

}



int PostprocRetin::Execute(infer_server::PackagePtr package, const infer_server::ModelPtr& model_info, std::shared_ptr<FAFrameInfo> data_) {
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

}