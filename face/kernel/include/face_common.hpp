#ifndef FACE_COMMON_HPP_
#define FACE_COMMON_HPP_

#include <limits.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <unistd.h>

#include <atomic>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace facealign {
  /**
   * @brief Flag to specify how bus watcher handle a single event.
   */
  enum EventType {
    EVENT_INVALID,  ///< An invalid event type.
    EVENT_ERROR,    ///< An error event.
    EVENT_WARNING,  ///< A warning event.
    EVENT_EOS,      ///< An EOS event.
    EVENT_STOP,     ///< Stops an event that is called by application layer usually.
    EVENT_STREAM_ERROR,  ///< A stream error event.
    EVENT_TYPE_END  ///< Reserved for your custom events.
  };


  class NonCopyable {
  public:
    NonCopyable() = default;
    ~NonCopyable() = default;
  private:
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable(NonCopyable&&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
    NonCopyable& operator=(NonCopyable&&) = delete;
  };

}
#endif //  FACE_COMMON_HPP_