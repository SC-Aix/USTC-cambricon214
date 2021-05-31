#ifndef DECODER_HPP_
#define DECODER_HPP_ 

#include <iostream>
#include <vector>
#include <string>
#include <dirent.h>

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "face_pipeline.hpp"
#include "face_module.hpp"
#include "frame.hpp"

namespace facealign {
  enum DecoderType {
    FFOPENCV = 0,
    FFCODEC = 1,
    OTHRES
  };

  struct SourceParam {
    DecoderType type = FFOPENCV;
  };

  class Decoder;

  class Source : public Module, public ModuleCreator<Source> {
  public:
    Source(const std::string& name);
    bool Open(ModuleParamSet parameters) override;
    void Close() override;
    int Process(std::shared_ptr<FAFrameInfo> data) override;
    bool CheckParamSet(const ModuleParamSet& parameters) const override;
    bool AddDecoder();
    void ImgLoop();
    void VideoLoop();
    Decoder* GetDecoder() { return decoder_; }
  private:
    Decoder* decoder_ = nullptr;
    SourceParam param;
  };

  class Decoder {
  public:
    virtual void DecoderImg(const std::string& path) = 0;
    virtual std::shared_ptr<FAFrame> CreateFrame() = 0;
    virtual void ImgLoop() = 0;
    virtual void VideoLoop() = 0;
  protected:
    Source* source_module = nullptr;
    Decoder(Source* ptr) { source_module = ptr; }
  };

  class DecoderOpencv : public Decoder {
  public:
    DecoderOpencv(Source* ptr) : Decoder(ptr) { mat_ptr = std::make_shared<cv::Mat>(); };
    void DecoderImg(const std::string& path) override;
    std::shared_ptr<FAFrame> CreateFrame() override;
    virtual void ImgLoop() override;
    static void GetFiles(const std::string& path, std::vector<std::string>& files);
    void AddFiles(const std::string& path) { GetFiles(path, files); }
    void DecoderVideo(const std::string& path);
    void VideoLoop() override;
  private:
    static int count;
    std::shared_ptr<cv::Mat> mat_ptr = nullptr;
    std::vector<std::string> files;
  };


  /*
    class SourceHandler {
      public:

      private:
    }
  */
} //facealign
#endif // !DECODER_HPP_
