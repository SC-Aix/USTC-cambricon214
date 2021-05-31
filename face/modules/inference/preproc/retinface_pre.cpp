#include "retinface_pre.hpp"

namespace facealign {
  static bool FloatEQ(float a, float b) { return std::abs(a - b) < 1E-5; }
  cv::Mat PreprocRetin::ExpandImage(cv::Mat input_img) {
    // expand input image to 16:9
    int expand_cols = 0, expand_rows = 0;
    if ((static_cast<float>(input_img.cols) / 16 > static_cast<float>(input_img.rows) / 9) &&
      !FloatEQ(static_cast<float>(input_img.cols) / 16, static_cast<float>(input_img.rows) / 9)) {
      int expect_rows = static_cast<int>(static_cast<float>(input_img.cols) / 16 * 9);
      expand_rows = 0.5f * (expect_rows - input_img.rows);
    }

    else if ((static_cast<float>(input_img.cols) / 16 < static_cast<float>(input_img.rows) / 9) &&
      !FloatEQ(static_cast<float>(input_img.cols) / 16, static_cast<float>(input_img.rows) / 9)) {
      int expect_cols = static_cast<int>(static_cast<float>(input_img.rows) / 9 * 16);
      expand_cols = 0.5f * (expect_cols - input_img.cols);
    }
    cv::copyMakeBorder(input_img, input_img, expand_rows, expand_rows, expand_cols, expand_cols, cv::BORDER_CONSTANT,
      cv::Scalar(123, 123, 123));
    return input_img;
  }

  int PreprocRetin::preproc(const cv::Mat& input, cv::Mat* output, const infer_server::ModelPtr& model) {
    ExpandImage(input);
    auto detector_input_shape = model->InputShape(0);
    cv::Size detector_input_size = cv::Size(detector_input_shape.GetW(), detector_input_shape.GetH());
    cv::resize(input, *output, detector_input_size);
    output->convertTo(*output, CV_32F);
    cv::cvtColor(*output, *output, CV_BGR2BGRA);
    return 0;
  }
  
  int PreprocRetin::Execute(infer_server::ModelIO* dst, const infer_server::InferData& src,
    const infer_server::ModelInfo& model_info) {
    auto frame = src.GetLref<cv::Mat>();
    cv::Mat output_image;
    if (0 != preproc(frame, &output_image, model_info_)) {
      return false;
    }
    memcpy(dst->buffers[0].MutableData(), output_image.data, model_info.InputShape(0).DataCount() * sizeof(float));
    return true;
  }

}