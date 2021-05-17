#ifndef FACE_CONNECTOR_HPP_
#define FACE_CONNECTOR_HPP_
#include <atomic>
#include <memory>
#include <vector>

#include "face_frame.hpp"
#include "face_common.hpp"
#include "face_queue.hpp"

namespace facealign {
  using FramePtr = std::shared_ptr<FAFrameInfo>;

  class Connector : public NonCopyable {
  public:
      // Connector(const std::string& input_module, const std::string output_module, size_t size);
      Connector(size_t size) : max_size_(size){};
      ~Connector(){};
      void Start();
      void Stop();
      bool IsStop() { return stop_; }
      std::vector<FramePtr> EmptyQueue();
      bool PushData(FramePtr frame);
      FramePtr GetData();
      size_t GetCurrentSize() { return current_size_;}

  private:
      //std::string input_module_ = "";
      // std::string output_moudule_ = "";
      std::mutex size_lock_;
      size_t current_size_ = 0;
      size_t max_size_ = 100;
      std::atomic<bool> stop_{false};
      ThreadSafeQueue<FramePtr> queue_;
  };
}

#endif // !FACE_CONNECTOR_HPP_