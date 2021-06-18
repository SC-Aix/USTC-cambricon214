#ifndef FFMPEG_HPP_
#define FFMPEG_HPP_

#include <atomic>
#include <chrono>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "easycodec/easy_decode.h"
#include "easycodec/vformat.h"

#ifdef __cplusplus
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
}
#endif 

namespace facealign {
  struct VideoInfo {
    edk::CodecType codec_type;
    std::vector<uint8_t> extra_data;
    int width, height;
    int progressive;
  };

  class IDemuxEventHandle {
    public:
      virtual bool OnPacket(const edk::CnPacket& frame) = 0;
      virtual void OnEos() = 0;
      virtual bool Running() = 0;
  };

  class VideoParser {
    public:
      VideoParser(IDemuxEventHandle* handle, bool rtsp_flag = false)
      : handler_(handle), is_rtsp_(rtsp_flag) {}
      ~VideoParser() { Close(); }
      bool Open(const char* path);
      int ParseLoop(uint32_t frame_interval);
      void Close();
      bool CheckTimeOut();
      const VideoInfo& GetVideoInfo() { return info_; }
    private:
      static constexpr uint32_t max_recevie_timeout_{3000};

      AVFormatContext* p_format_ctx_;
      AVBitStreamFilterContext* p_bsfc_;
      AVPacket packet_;
      AVDictionary* options_{nullptr};
      
      VideoInfo info_;
      IDemuxEventHandle* handler_;
      std::chrono::time_point<std::chrono::steady_clock> last_receive_frame_time_{};

      uint64_t frame_index_{0};
      int32_t video_index_;
      std::atomic<bool> have_video_source_{false};
      bool first_frame_{true};
      bool is_rtsp_{false};
  };


}



#endif // !FFMPEG_HPP_#define 