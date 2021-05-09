#ifndef DECODER_HPP_
#define DECODER_HPP_ 

#include <iostream>
#include <vector>
#include <string>
#include <io.h>

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "face_pipeline.hpp"
#include "face_module.hpp"
#include "frame.hpp"

namespace facealign {
  enum DecoderType {
    FFOPENCV = 0,
    FFRTSP = 1,
    OTHRES
  }

  struct SourceParam {
    DecoderType type = FFOPENCV;
  };

  class Source : public Module, public ModuleCreator<Source> {
  public:
    Source(const std::string& name);
    bool Open(ModuleParamSet& parameters) override;
    void Close() override;
    int Process(std::shared_ptr<FAFrameInfo> data) override;
    bool CheckParamSet(const ModuleParamSet& parameters) const override;
    bool AddDecoder();
    void Loop();

  private:
    Decoder* decoder_ = nullptr;
    SourceParam param;
  };

  class Decoder {
  public:
    virtual bool Decoder(const std::string& path) = 0; 
    virtual std::shared_ptr<FAFrame> CreateFrame() = 0;
    virtual void Loop() == 0;
  protected:
    Source* source_module = nullptr;
    Decoder(Source* ptr) { source_module = ptr; }
  };

  class DecoderOpencv : public Decoder {
  public:
    DecoderOpencv(Source* ptr) : Decoder(ptr) {};
    std::shared_ptr<FAFrame> DecoderImg(const std::string& path) override;
    std::shared_ptr<FAFrame> CreateFrame() override;
    virtual void Loop() override;
    static void GetFiles(cosnt std::string& path, std::vector<std::string> &files);
    void AddFiles(cosnt std::string &path) { GetFiles(name, files);}
  private:
    static count;
    std::shared_ptr<cv::Mat> mat_ptr = nullptr;
    std::vector<std::string> files;
  };

  DecoderOpencv::count = 0;

  /*
    class SourceHandler {
      public:

      private:
    }
  */
} //facealign
#endif // !DECODER_HPP_
