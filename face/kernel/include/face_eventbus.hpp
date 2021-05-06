#ifndef FACE_EVENTBUS_HPP_
#define FACE_EVENTBUS_HPP_

#include <string>
#include <thread>

#include "face_common.hpp"
namespace facealign {

/**
 * @brief A structure holding the event information.
 */
struct Event {
  EventType type;             ///< The event type.
  std::string stream_id;      ///< The stream that posts this event.
  std::string message;        ///< Additional event messages.
  std::string module_name;    ///< The module that posts this event.
  std::thread::id thread_id;  ///< The thread id from which the event is posted.
};

}

#endif // !FACE_EVENTBUS_HPP_
