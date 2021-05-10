#ifndef FACE_MODULE_HPP_
#define FACE_MODULE_HPP_

#include <cxxabi.h>
#include <unistd.h>

//#include <glog/logging.h>

#include <atomic>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <thread>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "face_common.hpp"
#include "face_rwlock.hpp"
#include "face_spinlock.hpp"
#include "face_config.hpp"
#include "face_eventbus.hpp"
#include "face_frame.hpp"
#include "face_pipeline.hpp"

namespace facealign {

class Pipeline;

class IModuleObserver {
 public:
  /**
   * @brief Notify "data" after processed by this module.
   */
  virtual void notify(std::shared_ptr<FAFrameInfo> data) = 0;
  virtual ~IModuleObserver() {}


};

class Module {
 public:

  explicit Module(const std::string &name) : name_(name) {}
  virtual ~Module();


  //void SetObserver(IModuleObserver *observer) {
  //  RwLockWriteGuard guard(observer_lock_);
  //  observer_ = observer;
  //}

  virtual bool Open(ModuleParamSet param_set) = 0; //注意这个函数的使用方式


  virtual void Close() = 0;
  

  virtual int Process(std::shared_ptr<FAFrameInfo> data) = 0;


  virtual void OnEos(const std::string &stream_id) {}

  inline  const std::string& GetName() const { return name_; }


 // bool PostEvent(EventType type, const std::string &msg);
  

  //bool PostEvent(Event e);


  bool TransmitData(std::shared_ptr<FAFrameInfo> data);


  virtual bool CheckParamSet(const ModuleParamSet &paramSet) const { return true; }

  //virtual void RecordTime(std::shared_ptr<FaceFrameInfo> data, bool is_finished); // 为什么要有帧数据

public:

  ParamRegister param_register_;

  /* useless for users */
  //size_t GetId(); // 这个做什么的呢？？？？

  friend class Pipeline;
  friend class FAFrameInfo;


  void SetContainer(Pipeline *container);

  /* useless for users */
 // std::vector<size_t> GetParentIds() const { return parent_ids_; }

  /* useless for users, set upstream node id to this module */
  //void SetParentId(size_t id) {
 //   parent_ids_.push_back(id);
  //  mask_ = 0;
  //  for (auto &v : parent_ids_) mask_ |= (uint64_t)1 << v;
 // }

  /* useless for users */
  //uint64_t GetModulesMask() const { return mask_; }


  //bool HasTransmit() const { return hasTransmit_.load(); }


  int DoProcess(std::shared_ptr<FAFrameInfo> data); // 和process 有什么区别？？

 protected:
    std::string name_;
    Rwlock container_lock_; // 这个是干嘛什么？
    Pipeline *container_;
    // 为什么用atomic？
   // std::atomic<bool> hasTransmit_{false};  ///< Whether it has permission to transmit data.

 private:
  // size_t id_ = INVALID_MODULE_ID;
  // static SpinLock module_id_spinlock_; // 这个是干嘛什么？
  // static uint64_t module_id_mask_; // 详细用法？

  // std::vector<size_t> parent_ids_;
  // uint64_t mask_ = 0;

  // RwLock observer_lock_;
 //  IModuleObserver* observer_; //为什么这里要用指针？

/*   void NotifyObserver(std::shared_ptr<FaceFrameInfo> data) { // 这个的作用？IModuleObserver到底是干嘛的？
       RwLockReadGuard guard(observer_lock_);
       if (observer_) {
           observer_->notify(data);
       }
   }*/
   int DoTransmitData(std::shared_ptr<FAFrameInfo> data);  // 这个的作用？
};

class ModuleFactory {
 public:
  /**
   * @brief Creates or gets the instance of the ModuleFactory class.
   *
   * @return Returns the instance of the ModuleFactory class.
   */
  static ModuleFactory *Instance() {
    if (nullptr == factory_) {
      factory_ = new (std::nothrow) ModuleFactory();
      //LOG_IF(FATAL, nullptr == factory_) << "ModuleFactory::Instance() new ModuleFactory failed.";
    }
    return (factory_);
  }
  virtual ~ModuleFactory() {}

  /**
   * Registers ``ModuleClassName`` and ``CreateFunction``.
   *
   * @param strTypeName The module class name.
   * @param pFunc The ``CreateFunction`` of a Module object that has a parameter ``moduleName``.
   *
   * @return Returns true if this function has run successfully.
   */
  bool Regist(const std::string &strTypeName, std::function<Module *(const std::string &)> pFunc) {
    if (nullptr == pFunc) {
      return (false);
    }
    bool ret = map_.insert(std::make_pair(strTypeName, pFunc)).second;
    return ret;
  }

  /**
   * Creates a module instance with ``ModuleClassName`` and ``moduleName``.
   *
   * @param strTypeName The module class name.
   * @param name The ``CreateFunction`` of a Module object that has a parameter ``moduleName``.
   *
   * @return Returns the module instance if this function has run successfully. Otherwise, returns nullptr if failed.
   */
  Module *Create(const std::string &strTypeName, const std::string &name) {
    auto iter = map_.find(strTypeName);
    if (iter == map_.end()) {
      std::cout << "nothat construct" << std::endl;
      std::cout << "-----------is_in" << map_.count("facealign::Source") << std::endl;
      return (nullptr);
    } else {
      return (iter->second(name));
    }
  }

  /**
   * Gets all registered modules.
   *
   * @return All registered module class names.
   */
  std::vector<std::string> GetRegisted() {
    std::vector<std::string> registed_modules;
    for (auto &it : map_) {
      registed_modules.push_back(it.first);
    }
    return registed_modules;
  }

 private:
  ModuleFactory() {}
  static ModuleFactory *factory_;
  std::unordered_map<std::string, std::function<Module *(const std::string &)>> map_;
};

//ModuleFactory* ModuleFactory::factory_
/**
 * @brief ModuleCreator
 *   A concrete ModuleClass needs to inherit ModuleCreator to enable reflection mechanism.
 *   ModuleCreator provides ``CreateFunction``, and registers ``ModuleClassName`` and ``CreateFunction`` to
 * ModuleFactory().
 */
template <typename T>
class ModuleCreator {
 public:
  struct Register {
    Register() {
      char *szDemangleName = nullptr;
      std::string strTypeName;
#ifdef __GNUC__
      szDemangleName = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
#else
      // in this format?:     szDemangleName =  typeid(T).name();
      szDemangleName = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
#endif
      

      if (nullptr != szDemangleName) {
        strTypeName = szDemangleName;
        free(szDemangleName);
      }
      std::cout << "--------------" << strTypeName.substr(11) << std::endl;
      ModuleFactory::Instance()->Regist(strTypeName.substr(11), CreateObject);
    }
    inline void do_nothing() const {} 
  };
  ModuleCreator() { register_.do_nothing(); }
  virtual ~ModuleCreator() { register_.do_nothing(); }
  /**
   * @brief Creates an instance of template (T) with specified instance name.
   *
   * This is a template function.
   *
   * @param name The name of the instance.
   *
   * @return Returns the instance of template (T).
   */
  static T *CreateObject(const std::string &name) { return new (std::nothrow) T(name); }
  static Register register_;
};

template <typename T>
typename ModuleCreator<T>::Register ModuleCreator<T>::register_;

/**
 * @brief ModuleCreatorWorker, a dynamic-creator helper.
 */
class ModuleCreatorWorker {
 public:
  /**
   * @brief Creates a module instance with ``ModuleClassName`` and ``moduleName``.
   *
   * @param strTypeName The module class name.
   * @param name The module name.
   *
   * @return Returns the module instance if the module instance is created successfully. Returns nullptr if failed.
   * @see ModuleFactory::Create
   */
  Module *Create(const std::string &strTypeName, const std::string &name) {
    Module *p = ModuleFactory::Instance()->Create(strTypeName, name);
    return (p);
  }
};
} //  facealign
#endif // FAE_MODULE_HPP_
