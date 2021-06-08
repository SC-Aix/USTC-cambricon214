#ifndef FACE_THREADPOOL_HPP_
#define FACE_THREADPOOL_HPP_
#include <functional>
#include <iostream>
#include <atomic>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

#include "face_queue.hpp"


namespace facealign {
  struct Task {
    Task() = default;
    Task(std::function<void()>&& f) : func(f) {};
    ~Task() = default;
    void operator()() {
      if (func) {
        func();
      }
      else {
        std::cerr << "no task funciton" << std::endl;
      }
    }
    std::function<void()> func = nullptr;
  };

  template<typename Q = ThreadSafeQueue<Task>,
    typename = std::enable_if<std::is_same<typename Q::value_type, Task>::value>>
    class ThreadPool {
    public:
      using queue_type = Q;
      using task_type = typename std::enable_if<std::is_same<typename Q::value_type, Task>::value, Task>::type;
      explicit ThreadPool(std::function<bool()> th_init_func, int n_threads = 0) : thread_init_func(th_init_func) {
        if (n_threads) Resize(n_threads);
      }
      ~ThreadPool() {
        Stop(true);
      }

      size_t Size() const noexcept { return threads_.size(); }
      uint32_t IdleNumber() const noexcept { return n_waiting_.load(); }
      std::thread& GetThread(int i) { return *threads_[i]; }

      void Resize(size_t n_threads) noexcept {
        if (!is_stop_ && !is_done_) {
          size_t old_n_threads = threads_.size();
          if (n_threads >= old_n_threads) {
            threads_.resize(n_threads);
            flags_.resize(n_threads);

            for (size_t i = old_n_threads; i < n_threads; ++i) {
              flags_[i] = std::make_shared<std::atomic<bool>>(false);
              SetThread(i);
            }
          }
          else {
            for (size_t i = old_n_threads - 1; i < n_threads; --i) {
              flags_[i]->store(true);
              threads_[i]->detach();
            }
            cv_.notify_all();
            threads_.resize(n_threads);
            flags_.resize(n_threads);
          }
        }
      }

      void SetThread(int i) noexcept {
        std::shared_ptr<std::atomic<bool>> tmp(flags_[i]);
        auto f = [this, i, tmp]() {
          std::atomic<bool>& flag = *tmp;
          if (thread_init_func) {
            if (thread_init_func()) {
            }
            else {
              std::cerr << "thread init failed!" << std::endl;
            }
          }

          task_type t;
          bool have_task = task_q_.TryPop(t);
          while (true) {
            while (have_task) {
              t();
              t.func = nullptr;
              if (flag.load())return;
              else {
                have_task = task_q_.TryPop(t);
              }
            }

            std::unique_lock<std::mutex> lk(mutex_);
            ++n_waiting_;
            cv_.wait(lk, [this, &t, &have_task, &flag]() {
              have_task = task_q_.TryPop(t);
              return have_task || is_done_ ||flag.load();
            });
            --n_waiting_;
            lk.unlock();

            if(!have_task) return;
          }
        };
        threads_[i].reset(new std::thread(f));
      }

      void Stop(bool wait_all_tasl_done = false) noexcept {
        if (!wait_all_tasl_done) {
          if (is_stop_) return;
          is_stop_ = true;
          for (size_t i = 0; i < threads_.size(); ++i) {
            flags_[i]->store(true);
            this->ClearQueue();
          }
        }
        else {
          if (is_done_ || is_stop_) return;
          is_done_ = true;
        }
        for (int i = 0; i < threads_.size(); ++i) {
          if (threads_[i]->joinable())threads_[i]->join();
        }
        this->ClearQueue();
        threads_.clear();
        flags_.clear();
      }

      task_type Pop() {
        task_type t;
        task_q_.TryPop(t);
        return t;
      }

      void ClearQueue() {
        task_type t;
        while (!task_q_.Empty())
          task_q_.TryPop(t);
      }

      template<typename callable, typename... Args>
      auto Push(callable&& f, Args&&... args) \
      ->std::future<typename std::result_of<callable(Args...)>::type> {
        auto pck = std::make_shared<std::packaged_task<typename std::result_of<callable(Args...)>::type()>>(
          std::bind(std::forward<callable>(f), std::forward<Args>(args)...));
        task_q_.Emplace([pck]() {(*pck)();});
        cv_.notify_one();
        return pck->get_future();
      }


    private:
      ThreadPool(const ThreadPool&) = delete;
      ThreadPool(ThreadPool&&) = delete;
      ThreadPool& operator=(const ThreadPool&) = delete;
      ThreadPool& operator=(ThreadPool&&) = delete;

      queue_type task_q_;
      std::atomic<int> n_waiting_{ 0 };
      std::function<bool()> thread_init_func = nullptr;
      std::vector<std::unique_ptr<std::thread>> threads_;

      std::condition_variable cv_;
      std::vector<std::shared_ptr<std::atomic<bool>>> flags_;
      std::atomic<bool> is_done_{ false };
      std::atomic<bool> is_stop_{ false };
      std::mutex mutex_;
  };
  using Threadpool = ThreadPool<ThreadSafeQueue<Task>>;
}


#endif // !FACE_THREADPOOL_HPP_
