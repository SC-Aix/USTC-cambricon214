#ifndef FACE_CONFIG_HPP_
#define FACE_CONFIG_HPP_

#include <string.h>

#include <unordered_map>
#include <vector>
#include <string>
#include <utility>

namespace facealign {
  using  ModuleParamSet = std::unordered_map<std::string, std::string>;
  
#define CNS_JSON_DIR_PARAM_NAME "json_file_dir" // 什么意思？？？

  struct FAModuleConfig {
    std::string name;  // the name of module
    ModuleParamSet parameters;
    size_t queue_capicity;
    std::string className;
    std::vector<std::string> next;
    
    bool ParseByJSONStr(const std::string &jstr);
    bool ParseByJSONFile(const std::string &jfname);
  };

 class ParamRegister {
  private:
    std::vector<std::pair<std::string, std::string>> params_desc_;
    std::string module_desc_;

  public:
    void Register(const std::string &param, const std::string &desc) {
      params_desc_.push_back(std::make_pair(param, desc));
    }

    void RegisterDesc(const std::string &desc) { module_desc_ = desc; }
    std::vector<std::pair<std::string, std::string>> GetParams() { return params_desc_;}
    bool IsRegisted(const std::string &key) const {
    if (strcmp(key.c_str(), CNS_JSON_DIR_PARAM_NAME) == 0) {
      return true;
    }
    for (auto &it : params_desc_) {
      if (key == it.first) {
        return true;
      }
    }
    return false;
   }

   std::string GetModuleDesc() { return module_desc_; }
    
 };


bool ConfigsFromJsonFile(const std::string& config_file, std::vector<FAModuleConfig> pmodule_configs);

}
#endif // !FACE_CONFIG_HPP_
