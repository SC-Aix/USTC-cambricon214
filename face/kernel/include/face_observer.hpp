#ifndef FACE_OBSERVER_HPP_
#define FACE_OBSERVER_HPP_
#include <thread>
#include <mutex>
#include "face_pipeline.hpp"

namespace facealign {
  class Pipeline;
  class Observer {
    public:
      Observer(Pipeline* ptr);
      ~Observer();
      void Start();
     // void Stop();
      void EventLoop();
      void SetStopTrue() {
        stop_pipeline_.store(true);
      }
    private:
      Rwlock container_lock;
      Pipeline *pipline = nullptr;
      std::thread *watcher_ = nullptr;
      std::atomic<bool> stop_pipeline_{false};
     // std::atomic<bool> stop_{false};
  };
}

#endif // !FACE_OBSERVER_HPP_
