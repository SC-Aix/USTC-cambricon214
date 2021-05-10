#ifndef FACE_FRAME_HPP_
#define FACE_FRAME_HPP_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "frame.hpp"
#include "face_common.hpp"

namespace facealign {

class Module;
class Pipline;

enum FAFrameFlag {
  FA_FRAME_FLAG_EOS = 1 << 0,      ///< Identifies the end of data stream.
  FA_FRAME_FLAG_INVALID = 1 << 1,  ///< Identifies the invalid of frame.
//  FA_FRAME_FLAG_REMOVED = 2 << 1   ///< Identifies the stream has been removed.
};

/**
 *  A structure holding the information of a frame.
 */
class FAFrameInfo : private NonCopyable {
 public:

  static std::shared_ptr<FAFrameInfo> Create(bool eos = false, std::shared_ptr<FAFrameInfo> payload = nullptr);
  ~FAFrameInfo();

  bool IsEos() { return flags & facealign::FA_FRAME_FLAG_EOS ? true : false; }

  bool IsInvalid() { return (flags & facealign::FA_FRAME_FLAG_INVALID) ? true : false; }  //  这样做的好处是什么？

 // void SetStreamIndex(uint32_t index) { channel_idx = index; }
  // GetStreamIndex() will be removed later
 // uint32_t GetStreamIndex() const { return channel_idx; }

  //std::string stream_id;    ///< the data strema aliases where this frame is located to.
  int64_t timestamp = -1;   ///< the time stamp of this frame.
  size_t flags = 0;         ///< the mask for this frame. "FAFrameFlag"

  //  user-defined DataFrame, InferResult etc...
  std::unordered_map<int, std::shared_ptr<FAFrame>> datas; // 为什么这里是int？？
  std::mutex datas_lock_;

  //  FAFrameInfo instance of parent pipeline
  std::shared_ptr<facealign::FAFrameInfo> payload = nullptr;

// private:
  // std::mutex mask_lock_;
  // uint64_t modules_mask_ =0;
 
 private:
   FAFrameInfo() {}
//   static std::mutex stream_count_lock_;
//   static std::unordered_map<std::string, int> stream_count_map_;
//
// public:
//   static int flow_depth_;

};


}


#endif // FACE_FRAME_HPP_
