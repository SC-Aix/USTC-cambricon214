#include "face_module.hpp"

namespace facealign {

  Module::~Module() {
    if (container_) {
      //container_->ReturnMoudleIdx(id_);
    }
  }

  void Module::SetContainer(Pipeline* container) {
    if (container) {
      {
        RwLockWriteGuard guard(container_lock_);
        container_ = container;
      }
      // GetId();
    }
    else {
      RwLockWriteGuard guard(container_lock_);
      container_ = nullptr;
      //id_ = INVALID_MODULE_ID;
    }
  }
  /*
  size_t Module::GetId() {
    if (id_ == INVALID_MODULE_ID) {
      RwLockReadGuard guard(container_lock_);
      if (container_) {
        id_ = container_->GetModuleIdx();
      }
    }
    return id_;
  }

  bool Module::PostEvent(EventType type, const std::string& msg) {
    Event event;
    event.type = type;
    event.message = msg;
    event.module_name = name_;

    return PostEvent(event);
  }

  bool Module::PostEvent(Event e) {
    RwLockReadGuard guard(container_lock_);
    if (container_) {
      return container_->GetEventBus()->PostEvent(e);
    } else {
      LOGW(CORE) << "[" << GetName() << "] module's container is not set";
      return false;
    }
  }
  */
  int Module::DoTransmitData(std::shared_ptr<FAFrameInfo> data) {
    // if (data->IsEos() && data->payload && IsStreamRemoved(data->stream_id)) {
       // FIMXE
     //  SetStreamRemoved(data->stream_id, false);
    // }
    RwLockReadGuard guard(container_lock_);
    if (container_) {
      return container_->ProvideData(this, data);
    }
    else {
      // if (HasTransmit()) NotifyObserver(data);
      return 0;
    }
  }

  int Module::DoProcess(std::shared_ptr<FAFrameInfo> data) {  // 不太理解
   /* bool removed = IsStreamRemoved(data->stream_id);
    if (!removed) {
      // For the case that module is implemented by a pipeline
      if (data->payload && IsStreamRemoved(data->payload->stream_id)) {
        SetStreamRemoved(data->stream_id, true);
       removed = true;
      }
    }

    if (!HasTransmit()) {
      if (!data->IsEos()) {
        if (!removed) {
          int ret = Process(data);
          if (ret != 0) {
            return ret;
          }
        }
        return DoTransmitData(data);
      } else {
        this->OnEos(data->stream_id);*/
        //if (Process(data) < 0) return -1;
    Process(data);
    return DoTransmitData(data);/*
  }
} else {
  if (removed) {
    data->flags |= CN_FRAME_FLAG_REMOVED;
  }
  return Process(data);
}*/
//return -1;
  }

  bool Module::TransmitData(std::shared_ptr<FAFrameInfo> data) {
    //if (!HasTransmit()) {
   //   return true;
   // }
    if (!DoTransmitData(data)) {
      return true;
    }
    return false;
  }
  ModuleFactory* ModuleFactory::factory_ = nullptr;
}

