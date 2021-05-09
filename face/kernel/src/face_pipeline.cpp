#include <thread>
#include <assert.h>

#include "face_pipeline.hpp"

namespace facealign {
  Pipeline::Pipeline(const std::string& name): name_(name){}
  Pipeline::~Pipeline() { 
    running = false;
  }

  bool Pipeline::ProvideData(const Module* module, std::shared_ptr<FAFrameInfo> data) {
    std::string moduleName = module->GetName();
  
    if (modules_map.find(moduleName) == modules_map.end())return false;
  
    TransmitData(moduleName, data);
    return true;
  }

  bool Pipeline::AddModule(std::shared_ptr<Module> module) {
    std::string moduleName = module->GetName();
    
    if (modules_map.find(moduleName) != modules_map.end()) {
     std::cout << "module has been added!" << std::endl;
     return false; 
    }
    std::cout << "Add module " << moduleName << " to pipeline" << std::endl;
    module->SetContainer(this);
    ConnectorInfo link_connector;
    modules_conector_.insert(std::make_pair(moduleName, std::make_shared<ConnectorInfo>(link_connector)));
    return true;
  }

  bool Pipeline::SetModuleAttribute(std::shared_ptr<Module> module, size_t queue_capacity) {
    std::string moduleName = module->GetName();
    if (modules_conector_.find(moduleName) != modules_conector_.end())return false;
    modules_conector_[moduleName]->connector = std::make_shared<Connector>(queue_capacity);
    return true;
  }

  bool Pipeline::LinkModules(std::shared_ptr<Module> up_node, std::shared_ptr<Module> down_node) {
    if (up_node == nullptr || down_node == nullptr) {
      std::cout << "upnode or downnode is nullptr";
      return false;
    }

    std::string up_node_name = up_node->GetName();
    std::string down_node_name = down_node->GetName();

    if (modules_conector_.find(up_node_name) == modules_conector_.end() ||
      modules_conector_.find(down_node_name) == modules_conector_.end()) {
    std::cout << "module has not been added to this pipeline" << std::endl;
    return false;
    }
    
    auto up_node_info = modules_conector_.find(up_node_name)->second;
    auto down_node_info = modules_conector_.find(down_node_name)->second;
    up_node_info->output_connectors.push_back(down_node_name);
    down_node_info->input_connectors.push_back(up_node_name);
    return true;
  }

  bool Pipeline::Start() {
    if (IsRunning()) return true;
    std::vector<std::shared_ptr<Module>> modules_open;
    bool open_failed = false;
    for (auto& it : modules_map) {
      if (!it.second->Open(GetModuleParamSet(it.second->GetName()))) {
        open_failed = true;
        std::cout << "Open Module failed !--------------" << std::endl;
        break;
      } else {
        modules_open.push_back(it.second);
      }
    }

    if (open_failed) {
      for (auto &it : modules_open) {
        it->Close();
        return false;
      }
    }

    running.store(true);
    for (const auto &it : modules_conector_) {
      it.second->connector->Start();
    }
    
    for (auto &it : modules_conector_) {
      threads_.push_back(std::thread(&Pipeline::TaskLoop, this, it.first)); // 这个this 是做什么用的呢。
    }

    std::cout << "Pipeline Start" << std::endl;
    std::cout << "All modules, except the first module, total  threads  is: " << threads_.size() << std::endl;
    return true;
  }
  
  bool Pipeline::Stop() {
    if (!IsRunning()) return true;
    
    for (auto &it : modules_conector_) {
      if (it.second->connector) {
        it.second->connector->Stop();
        it.second->connector->EmptyQueue();
      }
    }

    running.store(false);
    
    for (std::thread& it : threads_) {
      if (it.joinable())it.join();
    }

    threads_.clear();

    for (auto &it : modules_map) {
      it.second->Close();
    }
    return true;
  }

  void Pipeline::TaskLoop(std::string module_name) {
    auto module_ = modules_map[module_name];
    auto& module_info = modules_conector_[module_name];
    std::shared_ptr<Connector> connector = module_info->connector;
    while(1) {
      FramePtr data = nullptr;
      while (!connector->IsStop() && data == nullptr) {
        data = connector->GetData();
      }

      if (connector->IsStop()) {
        break;
      }

      if (data == nullptr) continue;
      int ret = module_->DoProcess(data);
      if (ret < 0) {
        std::cout << module_name << " process data failed!" << std::endl;
        return;
      }
    }
    return;
  }

  void Pipeline::TransmitData(const std::string &module_name, std::shared_ptr<FAFrameInfo> data) {
    if (modules_conector_.find(module_name) == modules_conector_.end()) {
      std::cout << "TransmitData, module connector is not found!" << std::endl;
    }
    auto connector_info = modules_conector_[module_name];
    for (auto &it : connector_info->output_connectors) {
      auto down_node_info = modules_conector_.find(it)->second;
      assert(down_node_info->connector);
      assert(0 < down_node_info->input_connectors.size());

      while (!down_node_info->connector->PushData(data) && !down_node_info->connector->IsStop()) {
        std::this_thread::sleep_for(std::chrono::microseconds(20));
      }
    }

    if (frame_done_callback_ && !connector_info->output_connectors.size()) {
      frame_done_callback_(data);
    }
  }

  int Pipeline::AddModuleConfig(const FAModuleConfig &config) {
    modules_config_[config.name] = config;
    modules_next_[config.name] = config.next;
    return 0;
  }

  ModuleParamSet Pipeline::GetModuleParamSet(const std::string& module_name) {
    ModuleParamSet paramset;
    auto iter = modules_config_.find(module_name);
    if (iter != modules_config_.end()) {
      for (auto &v : modules_config_[module_name].parameters)
        paramset[v.first] = v.second;
    }
    return paramset;
  }

  FAModuleConfig Pipeline::GetModuleConfig(const std::string& module_name) {
    FAModuleConfig config = {};
    auto iter = modules_config_.find(module_name);
    if (iter != modules_config_.end()) {
      config = iter->second;
    }
    return config;
  }

  int Pipeline::BuildPipeline(const std::vector<FAModuleConfig> &module_configs) {
    ModuleCreatorWorker creator;
    std::vector<std::shared_ptr<Module>> modules;
    for (auto& v : module_configs) {
      this->AddModuleConfig(v);
      std::shared_ptr<Module> instance(creator.Create(v.className, v.name));
      if (!instance) {
        std::cout << "error occurs in creating module, name: " << v.className << std::endl;
      }
      this->AddModule(instance);
      this->SetModuleAttribute(instance, v.queue_capicity);
      modules.push_back(instance);
    }

    for (auto& v : module_configs) {
      if (modules_map.find(v.name) == modules_map.end() || 
          modules_map.find(v.next[0]) == modules_map.end()) {
        std::cout << "module has not been added!" << std::endl;
      }
      if(!LinkModules(modules_map[v.name], modules_map[v.next[0]])) {
        std::cout << "link modules failed! "<< std::endl;
      }; // not support 1 to more nodes.
    }
    return 0;
  }

  int Pipeline::BuildPipelineByJSONFile(const std::string& config_file) {
    std::vector<FAModuleConfig> configs;
    bool ret = ConfigsFromJsonFile(config_file, configs);
    if (ret != true) {
      return -1;
    }
    return BuildPipeline(configs);
  }







}
