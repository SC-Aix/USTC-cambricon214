#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <iostream>
#include <fstream>
#include <utility>
#include <vector>

#include "face_config.hpp"

namespace facealign
{

  bool FAModuleConfig::ParseByJSONStr(const std::string& jstr)
  {
    rapidjson::Document doc;
    if (doc.Parse<rapidjson::kParseCommentsFlag>(jstr.c_str()).HasParseError())
    {
      std::cout << "Parse module configuration failed. Error code [" << std::to_string(doc.GetParseError()) << "]"
        << " Offset [" << std::to_string(doc.GetErrorOffset()) << "]. JSON:" << jstr;
      return false;
    }

    /* get members */
    const auto end = doc.MemberEnd();

    // className
    if (end == doc.FindMember("class_name"))
    {
      std::cout << "Module has to have a class_name.";
      return false;
    }
    else
    {
      if (!doc["class_name"].IsString())
      {
        std::cout << "class_name must be string type.";
        return false;
      }
      this->className = doc["class_name"].GetString();
    }
    return true;
  }

  bool FAModuleConfig::ParseByJSONFile(const std::string& jfname)
  {
    std::ifstream ifs(jfname);

    if (!ifs.is_open())
    {
      //LOGE(CORE) << "File open failed :" << jfname;
      return false;
    }

    std::string jstr((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();

    if (!ParseByJSONStr(jstr))
    {
      return false;
    }

    /***************************************************
   * add config file path to custom parameters
   ***************************************************/

    std::string jf_dir = "";
    auto slash_pos = jfname.rfind("/");
    if (slash_pos == std::string::npos)
    {
      jf_dir = ".";
    }
    else
    {
      jf_dir = jfname.substr(0, slash_pos);
    }
    jf_dir += '/';

    if (this->parameters.end() != this->parameters.find(CNS_JSON_DIR_PARAM_NAME))
    {
      std::cout << "Parameter [" << CNS_JSON_DIR_PARAM_NAME << "] does not take effect. It is set "
        << "up by cnstream as the directory where the configuration file is located and passed to the module.";
    }

    this->parameters[CNS_JSON_DIR_PARAM_NAME] = jf_dir;
    return true;
  }

  bool ConfigsFromJsonFile(const std::string& config_file,
    std::vector<FAModuleConfig>* pmodule_configs)
  {
    auto& module_configs = *pmodule_configs;

    std::ifstream ifs(config_file);
    if (!ifs.is_open())
    {
      return false;
    }

    std::string jstr((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();

    /* traversing config items */
    std::vector<std::string> namelist;
    rapidjson::Document doc;
    if (doc.Parse<rapidjson::kParseCommentsFlag>(jstr.c_str()).HasParseError())
    {
      return false;
    }

    for (rapidjson::Document::ConstMemberIterator iter = doc.MemberBegin(); iter != doc.MemberEnd(); ++iter)
    {
      rapidjson::StringBuffer sbuf;
      rapidjson::Writer<rapidjson::StringBuffer> jwriter(sbuf);
      iter->value.Accept(jwriter);

      std::string item_name = iter->name.GetString();


      FAModuleConfig mconf;
      mconf.name = item_name;
      if (find(namelist.begin(), namelist.end(), mconf.name) != namelist.end())
      {
        return false;
      }
      namelist.push_back(mconf.name);

      if (!mconf.ParseByJSONStr(std::string(sbuf.GetString())))
      {
        return false;
      }

      /***************************************************
     * add config file path to custom parameters
     ***************************************************/

      std::string jf_dir = "";
      auto slash_pos = config_file.rfind("/");
      if (slash_pos == std::string::npos)
      {
        jf_dir = ".";
      }
      else
      {
        jf_dir = config_file.substr(0, slash_pos);
      }
      jf_dir += '/';

      if (mconf.parameters.end() != mconf.parameters.find(CNS_JSON_DIR_PARAM_NAME))
      {
      }

      mconf.parameters[CNS_JSON_DIR_PARAM_NAME] = jf_dir;
      module_configs.push_back(mconf);
    }
    return true;
  }
}