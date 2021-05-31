#include "reflex_object.hpp"

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>


namespace facealign {

static std::map<std::string, ClassInfo<ReflexObject>> sg_obj_map;

ReflexObject* ReflexObject::CreateObject(const std::string& name) {
  const auto& obj_map = sg_obj_map;
  auto info_iter = obj_map.find(name);

  if (obj_map.end() == info_iter) {
    std::cerr << "class name is not register!" << std::endl;
    return nullptr;
  }

  const auto& info = info_iter->second;
  return info.CreateObject();
}

bool ReflexObject::Register(const ClassInfo<ReflexObject>& info) {
  auto& obj_map = sg_obj_map;
  if (obj_map.find(info.name()) != obj_map.end()) {
    std::cout << "Register object named [" << info.name() << "] failed!!!"
                        << "Object name has been registered." << std::endl;
    return false;
  }

  obj_map.insert(std::pair<std::string, ClassInfo<ReflexObject>>(info.name(), info));

  std::cout << "Register object named [" << info.name() << "]" << std::endl;
  return true;
}

ReflexObject::~ReflexObject() {}

}  // namespace cnstream
