#ifndef FACE_RWLOCK_HPP_
#define FACE_RWLOCK_HPP_

#include <pthread.h>  // for pthread_rwlock_t
#include <memory>
#include <utility>
namespace facealign {

class Rwlock {
 public:
  Rwlock() { pthread_rwlock_init(&rwlock_, NULL);}
  ~Rwlock() { pthread_rwlock_destroy(&rwlock_);}
  void wrlock() { pthread_rwlock_wrlock(&rwlock_); }
  void rdlock() { pthread_rwlock_rdlock(&rwlock_); }
  void unlock() { pthread_rwlock_unlock(&rwlock_); }
  
  void rlock() { pthread_rwlock_rdlock(&rwlock_);}
 private:
  pthread_rwlock_t rwlock_;
};

class RwLockWriteGuard {
 public:
  explicit RwLockWriteGuard(Rwlock& lock) : lock_(lock) { lock_.wrlock(); }
  ~RwLockWriteGuard() { lock_.unlock(); }

 private:
  Rwlock& lock_;
};

class RwLockReadGuard {
 public:
  explicit RwLockReadGuard(Rwlock& lock) : lock_(lock) { lock_.rdlock(); }
  ~RwLockReadGuard() { lock_.unlock(); }

 private:
  Rwlock& lock_;
};

}
#endif // !FACE_RWLOCK_HPP_


