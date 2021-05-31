#ifndef REFLEX_OBJECT_HPP_
#define REFLEX_OBJECT_HPP_

#include <functional>
#include <map>
#include <iostream>

#define DECLARE_REFLEX_OBJECT(Class)                              \
 public:                                                          \
  static facealign::ClassInfo<facealign::ReflexObject> sclass_info; \
                                                                  \
 protected:                                                       \
  const facealign::ClassInfo<facealign::ReflexObject>& class_info() const;

#define IMPLEMENT_REFLEX_OBJECT(Class)                                                                                \
  facealign::ClassInfo<ReflexObject> Class::sclass_info(std::string(#Class),                                           \
                                                       facealign::ObjectConstructor<facealign::ReflexObject>([]() {     \
                                                         return reinterpret_cast<facealign::ReflexObject*>(new Class); \
                                                       }),                                                            \
                                                       true);                                            \
  const facealign::ClassInfo<facealign::ReflexObject>& Class::class_info() const { return sclass_info; }

#define DECLARE_REFLEX_OBJECT_EX(Class, BaseType)   \
 public:                                            \
  static facealign::ClassInfo<BaseType> sclass_info; \
                                                    \
 protected:                                         \
  const facealign::ClassInfo<BaseType>& class_info() const;

#define IMPLEMENT_REFLEX_OBJECT_EX(Class, BaseType)                                                          \
  facealign::ClassInfo<BaseType> Class::sclass_info(                                                          \
      std::string(#Class),                                                                                   \
      facealign::ObjectConstructor<BaseType>([]() { return reinterpret_cast<BaseType*>(new Class); }), true); \
  const facealign::ClassInfo<BaseType>& Class::class_info() const { return sclass_info; }


namespace facealign {
  template<class T>
  using ObjectConstructor = std::function<T* ()>;

  template<typename T>
  class ClassInfo {
  public:
    ClassInfo(const std::string& name, const ObjectConstructor<T>& constructor, bool regist = false);

    T* CreateObject() const;
    std::string name() const;
    bool Register() const;
    const ObjectConstructor<T>& constructor() const;

  private:
    std::string name_;
    ObjectConstructor<T> constructor_;
  };

  class ReflexObject {
  public:
    static ReflexObject* CreateObject(const std::string& name);
    static bool Register(const ClassInfo<ReflexObject>& info);
    virtual ~ReflexObject() = 0;
  };

  template <typename T>
  class ReflexObjectEx : public ReflexObject {
  public:
    static T* CreateObject(const std::string& name);
    static bool Register(const ClassInfo <T>& info);
    virtual ~ReflexObjectEx() = 0;
  };

  template <typename T>
  ClassInfo<T>::ClassInfo(const std::string& name, const ObjectConstructor<T>& constructor, bool regist)
    : name_(name), constructor_(constructor) {
    if (regist) {
      Register();
    }
  }

  template <typename T>
  inline std::string ClassInfo<T>::name() const {
    return name_;
  }

  template <typename T>
  inline const ObjectConstructor<T>& ClassInfo<T>::constructor() const {
    return constructor_;
  }

  template <typename T>
  inline bool ClassInfo<T>::Register() const {
    return ReflexObjectEx<T>::Register(*this);
  }

  template <typename T>
  T* ClassInfo<T>::CreateObject() const {
    if (NULL != constructor_) {
      return constructor_();
    }
    return nullptr;
  }

  template <typename T>
  T* ReflexObjectEx<T>::CreateObject(const std::string& name) {
    auto ptr = ReflexObject::CreateObject(name);
    if (nullptr == ptr) return nullptr;
    T* ret = dynamic_cast<T*>(ptr);
    if (nullptr == ret) {
      delete ptr;
      return nullptr;
    }
    return ret;
  }

  template <typename T>
  bool ReflexObjectEx<T>::Register(const ClassInfo<T>& info) {
    // build base ClassInfo(ClassInfo<ReflexObjectEx>)
    ObjectConstructor<ReflexObject> base_constructor = NULL;
    if (info.constructor() != NULL) {
      base_constructor = [info]() { return static_cast<ReflexObject*>(info.constructor()()); };
    }
    ClassInfo<ReflexObject> base_info(info.name(), base_constructor);

    return ReflexObject::Register(base_info);
  }

  template <typename T>
  ReflexObjectEx<T>::~ReflexObjectEx() {}
}

#endif // !REFLEX_OBJECT_HPP_