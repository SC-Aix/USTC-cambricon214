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

#ifndef ANCHOR_GENERTOR_H_
#define ANCHOR_GENERTOR_H_

#include <iostream>
#include <map>
#include <vector>
#include "shape.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#if (CV_MAJOR_VERSION >= 3)
#include "opencv2/imgcodecs/imgcodecs.hpp"
#endif
 //#include "postproc.hpp"

class AnchorCfg {
public:
  std::vector<float> SCALES;
  std::vector<float> RATIOS;
  int BASE_SIZE;

  AnchorCfg() {}
  ~AnchorCfg() {}
  AnchorCfg(const std::vector<float> s, const std::vector<float> r, int size) {
    SCALES = s;
    RATIOS = r;
    BASE_SIZE = size;
  }
};  // class AnchorCfg

extern const unsigned _feat_stride_num;
extern std::vector<int> _feat_stride_fpn;
extern std::map<int, AnchorCfg> anchor_cfg;

// class FeatureStride {
// public:
//    FeatureStride();
//
//    ~FeatureStride();
//
//    // init different anchors
//    int Init(int stride, const AnchorCfg& cfg, bool dense_anchor);
//
// private:
//    float *clsDataPtr;
//    float *regDataPtr;
//    float *ptsDataPtr;
//
//};

typedef struct feature_map {
  float* dataPtr;
  infer_server::Shape shape;
} FeatureMap;

class CRect2f {
public:
  CRect2f(float x1, float y1, float x2, float y2) {
    val[0] = x1;
    val[1] = y1;
    val[2] = x2;
    val[3] = y2;
  }

  float& operator[](int i) { return val[i]; }

  float operator[](int i) const { return val[i]; }

  float val[4];

  void print() { printf("rect %f %f %f %f\n", val[0], val[1], val[2], val[3]); }
};  // class CRect2f

class Anchor {
public:
  Anchor() {}

  ~Anchor() {}

  bool operator<(const Anchor& t) const { return score < t.score; }

  bool operator>(const Anchor& t) const { return score > t.score; }

  float& operator[](int i) {
    assert(0 <= i && i <= 4);

    if (i == 0) return finalbox.x;
    if (i == 1) return finalbox.y;
    if (i == 2) return finalbox.width;
    if (i == 3) return finalbox.height;

    return finalbox.x;
  }

  float operator[](int i) const {
    assert(0 <= i && i <= 4);

    if (i == 0) return finalbox.x;
    if (i == 1) return finalbox.y;
    if (i == 2) return finalbox.width;
    if (i == 3) return finalbox.height;
    return .0f;
  }

  cv::Rect_<float> anchor;       // x1,y1,x2,y2
  float reg[4];                  // offset reg
  cv::Point center;              // anchor feat center
  float score;                   // cls score
  std::vector<cv::Point2f> pts;  // pred pts
  cv::Rect_<float> finalbox;     // final box res

  void print() {
    printf("finalbox %f %f %f %f, score %f\n", finalbox.x, finalbox.y, finalbox.width, finalbox.height, score);
    printf("landmarks ");
    for (unsigned i = 0; i < pts.size(); ++i) {
      printf("%f %f, ", pts[i].x, pts[i].y);
    }
    printf("\n");
  }
};  // class Anchor

class AnchorGenerator {
public:
  AnchorGenerator();
  ~AnchorGenerator();

  // init different anchors
  int Init(int stride, const AnchorCfg& cfg, bool dense_anchor);

  // anchor plane
  int Generate(int fwidth, int fheight, int stride, float step, std::vector<int>& size, std::vector<float>& ratio,
    bool dense_anchor);

  // filter anchors and return valid anchors
  // int FilterAnchor(const caffe::Blob<float>* cls, const caffe::Blob<float>* reg,
  // const caffe::Blob<float>* pts, std::vector<Anchor>& result);
  // int FilterAnchor(const caffe::Blob<float> *cls, const caffe::Blob<float> *reg,
  // const caffe::Blob<float> *pts, std::vector<Anchor>& result,float confidence_threshold);

  // int FilterAnchor(const caffe::Blob<float> *cls, const caffe::Blob<float> *reg,
  // const caffe::Blob<float> *pts, std::vector<Anchor>& result,float ratio_w,float ratio_h,float
  // confidence_threshold);

  int FilterAnchor(const FeatureMap* cls, const FeatureMap* reg, const FeatureMap* pts, std::vector<Anchor>& result,
    float ratio_w, float ratio_h, float confidence_threshold);

  int FilterAnchor(const float* cls, const float* reg, const float* pts, std::vector<Anchor>& result, int featmap_w,
    int featmap_h, float ratio_w, float ratio_h, float confidence_threshold);

private:
  void _ratio_enum(const CRect2f& anchor, const std::vector<float>& ratios, std::vector<CRect2f>& ratio_anchors);

  void _scale_enum(const std::vector<CRect2f>& ratio_anchor, const std::vector<float>& scales,
    std::vector<CRect2f>& scale_anchors);

  void bbox_pred(const CRect2f& anchor, const CRect2f& delta, cv::Rect_<float>& box);

  void landmark_pred(const CRect2f anchor, const std::vector<cv::Point2f>& delta, std::vector<cv::Point2f>& pts);

  std::vector<std::vector<Anchor>> anchor_planes;  // corrspont to channels

  std::vector<int> anchor_size;
  std::vector<float> anchor_ratio;
  float anchor_step;  // scale step
  int anchor_stride;  // anchor tile stride
  int feature_w;      // feature map width
  int feature_h;      // feature map height

  std::vector<CRect2f> preset_anchors;
  unsigned anchor_num;  // anchor type num

  float ratiow = 0.0;
  float ratioh = 0.0;
};  // class AnchorGenerator

extern void nms_cpu(std::vector<Anchor>& boxes, float threshold, std::vector<Anchor>& filterOutBoxes);

#endif  // ANCHOR_GENERTOR_H_
