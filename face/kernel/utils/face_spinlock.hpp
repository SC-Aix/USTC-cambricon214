#ifndef FACE_SPINLOCK_HPP_
#define FACE_SPINLOCK_HPP_

#include <unistd.h>
#include <atomic>

namespace facealign {

class SpinLock {
 public:
  void lock() {
    while (lock_.test_and_set(std::memory_order_acquire)) {
    }  // spin
  }
  void unlock() { lock_.clear(std::memory_order_release); }

 private:
  std::atomic_flag lock_ = ATOMIC_FLAG_INIT;
};

class SpinLockGuard {
 public:
  explicit SpinLockGuard(SpinLock& lock) : lock_(lock) { lock_.lock(); }
  ~SpinLockGuard() { lock_.unlock(); }

 private:
  SpinLock& lock_;
};

}
#endif  // FACE_SPINLOCK_HPP_
