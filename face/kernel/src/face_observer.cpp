#include "face_observer.hpp"

namespace facealign {
  Observer::Observer(Pipeline* ptr) : pipline(ptr) {
    Start();
  }

  Observer::~Observer() {
    if (watcher_->joinable())watcher_->join();
  }

  void Observer::Start() {
    watcher_ = new std::thread(&Observer::EventLoop, this);
  }

  void Observer::EventLoop() {
    while(!stop_pipeline_.load()) {}
    {
      RwLockWriteGuard lk(container_lock);
      pipline->Stop();
    }
  }
}