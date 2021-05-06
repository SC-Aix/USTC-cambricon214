#ifndef FACE_PIPELINE_HPP
#define FACE_PIPELINE_HPP


#include <fstream>
#include <bitset>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <iostream>

#include "face_common.hpp"
#include "face_module.hpp"
#include "face_frame.hpp"
#include "face_connector.hpp"
#include "face_config.hpp"

namespace facealign {
enum StreamMsgType {
  EOS_MSG = 0,     ///< The end of a stream message. The stream has received EOS message in all modules.
  ERROR_MSG,       ///< An error message. The stream process has failed in one of the modules.
  STREAM_ERR_MSG,  ///< Stream error message, stream process failed at source.
  FRAME_ERR_MSG,   ///< Frame error message, frame decode failed at source.
};

struct StreamMsg {
  StreamMsgType type;       ///< The type of a message.
  std::string stream_id;    ///< Stream id, set by user in FaceFrameINfo::stream_id.
  std::string module_name;  ///< The module that posts this event.
  int64_t pts = -1;  ///< The pts of this frame.
};

/**
 * @brief Stream message observer.
 *
 * Receives stream messages from a pipeline.
 * To receive stream messages from the pipeline, you can define a class to inherit the
 * StreamMsgObserver class and call the ``Update`` function. The
 * observer instance is bounded to the pipeline using the
 * Pipeline::SetStreamMsgObserver function .
 *
 * @see Pipeline::SetStreamMsgObserver StreamMsg StreamMsgType.
 */
class StreamMsgObserver {
 public:
  /**
   * Receives stream messages from a pipeline passively.
   *
   * @param msg The stream message from a pipeline.
   */
  virtual void Update(const StreamMsg& msg) = 0;
  virtual ~StreamMsgObserver() = default;
};

/**
 * The link status between modules.
 */
struct LinkStatus {   // todo 具体的使用方法？
  bool stopped;    ///< Whether the data transmissions between the modules are stopped.
  std::vector<uint32_t> cache_size; ///< The size of each queue that is used to cache data between modules.
};

static constexpr uint32_t MAX_STREAM_NUM = 64;

/**
 * @brief ModuleId&StreamIdx manager for pipeline.
 *
 * Allocates and deallocates id for Pipeline modules & Streams.
 *
 *
class IdxManager {
 public:
   IdxManager() = default;
   IdxManager(const IdxManager&) = delete;
   IdxManager& operator=(const IdxManager&) = delete;
   uint32_t GetStreamIndex(const std::string& stream_id);
   void ReturnStreamIndex(const std::string& stream_id);
   size_t GetModuleIdx(); // 不需要参数吗？
   void ReturnModuleIdx(size_t id_);
 private:
   std::mutex id_lock;
   std::unordered_map<std::string, uint32_t> stream_idx_map;
   std::bitset<MAX_STREAM_NUM> stream_bitset; // 好用的方式 bitset 注意使用方式
   uint64_t module_id_mask_ = 0; // 使用方式？
};
*/

class Pipeline : private NonCopyable {
 public:
  explicit Pipeline(const std::string& name);
  ~Pipeline();
  const std::string& GetName() const { return name_; }
  bool ProvideData(const Module* module, std::shared_ptr<FAFrameInfo> data);
  void TransmitData(const std::string &module_name, std::shared_ptr<FAFrameInfo> data);
  bool Start();
  bool Stop();
  
 public:
  int AddModuleConfig(const FAModuleConfig& config);
  int BuildPipeline(const std::vector<FAModuleConfig>& module_configs);
  int BuildPipelineByJSONFile(const std::string& config_file);
  Module* GetModule(const std::string& name);
  ModuleParamSet GetModuleParamSet(const std::string &module_name);
  FAModuleConfig GetModuleConfig(const std::string &module_name);
  bool AddModule(std::shared_ptr<Module> module);
  bool SetModuleAttribute(std::shared_ptr<Module> module, size_t queue_capacity = 20);
  bool LinkModules(std::shared_ptr<Module> up_node, std::shared_ptr<Module> down_node);
  //bool QueryLinkStatus(LinkStatus* status, const std::string& kid);
  inline bool IsRunning() { return running; }
  inline void RegisterCallback(std::function<void(std::shared_ptr<FAFrameInfo>)> callback) {
    frame_done_callback_ = std::move(callback); //这里为什么用move？？？？
  }

  struct ConnectorInfo {
    std::vector<std::string> input_connectors;
    std::vector<std::string> output_connectors;
    std::shared_ptr<Connector> connector;
  };
 
  std::function<void(std::shared_ptr<FAFrameInfo>)> frame_done_callback_ = nullptr;
 private:
  void TaskLoop(std::string module_name_);

  std::atomic<bool> running {false};
 private:
  std::string name_;
  std::vector<std::thread> threads_;
  std::unordered_map<std::string, std::shared_ptr<Module>> modules_map;
  std::unordered_map<std::string, std::shared_ptr<ConnectorInfo>> modules_conector_;
  std::unordered_map<std::string, FAModuleConfig> modules_config_;
  std::unordered_map<std::string, std::vector<std::string>> modules_next_;
};


}

#endif // FACE_PIPELINE_HPP