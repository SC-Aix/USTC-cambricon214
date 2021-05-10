
#include <iostream>

#include "face_connector.hpp"

namespace facealign {
/*
Connector::Connector(const std::string& input_module, 
                    const std::string output_module, size_t size) : 
                    input_module_(input_module), output_module_(output_module), 
                    max_size_(size) {
}*/


void Connector::Start() {
  stop_.store(false);
}

void Connector::Stop() {
  stop_.store(true);
}

FramePtr Connector::GetData() {
  std::lock_guard<std::mutex> lk(size_lock_);
   FramePtr data;
   if(queue_.WaitAndTryPop(data, std::chrono::microseconds(1000))) {
     current_size_--;
   }
   return data;
}

bool Connector::PushData(std::shared_ptr<FAFrameInfo> frame) {
  std::lock_guard<std::mutex> lk(size_lock_);
  if (current_size_ + 1 > max_size_) {
    std::cout << "current_size" << current_size_ << std::endl;
    std::cout << "max_size" << max_size_ << std::endl;
    std::cout << "queue is full!" << std::endl;
    return false;
  }
  current_size_++;
  queue_.Push(frame);
  return true;
}

std::vector<FramePtr> Connector::EmptyQueue() {
    std::vector<FramePtr> vec_data;
    FramePtr data;
    while (!queue_.Empty()) {
        queue_.WaitAndTryPop(data, std::chrono::microseconds(1000));
        vec_data.push_back(data);
    }
    return vec_data;
}

}