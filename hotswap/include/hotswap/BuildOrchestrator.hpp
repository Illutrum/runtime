#ifndef HOTSWAP_BUILD_ORCHESTRATOR_HPP
#define HOTSWAP_BUILD_ORCHESTRATOR_HPP

#include <efsw/efsw.hpp>
#include <atomic>
#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace hotswap {

  struct BuildConfig {
    std::string sourceDir;
    std::string buildDir;
    std::string cmakeSourceDir;
    std::string modelSoPath;
    std::string viewSoPath;
    std::string manifestPath;
  };

  class BuildOrchestrator : public std::enable_shared_from_this<BuildOrchestrator> {
  public:
    explicit BuildOrchestrator(BuildConfig config);
    ~BuildOrchestrator();

    void start();
    void stop();
    void notifySourceChanged() { changeNumber_++; }

  private:
    struct ScanResult {
      bool needCmake = false;
      bool needNinja = false;
      bool buildModel = false;
      bool buildView = false;
    };

    void buildLoop();
    bool runCommand(const std::string& command);
    ScanResult scanSources();
    std::vector<std::string> ninjaTargets(const ScanResult& scan) const;
    void runBuild();

    BuildConfig config_;
    std::unique_ptr<efsw::FileWatcher> watcher_;
    efsw::WatchID watchId_ = -1;
    std::atomic<int> changeNumber_{0};
    std::atomic<int> buildNumber_{0};
    std::atomic<bool> finished_{false};
    std::thread buildThread_;
    std::unique_ptr<efsw::FileWatchListener> listener_;

    std::set<std::pair<std::string, std::filesystem::file_time_type>> modTimesSet_;
    std::set<std::string> filesSet_;
    std::filesystem::file_time_type cmakeListsModTime_{};
    bool hasCmakeListsModTime_ = false;
  };

}

#endif
