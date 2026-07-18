#include <hotswap/BuildOrchestrator.hpp>
#include <hotswap/ManifestWriter.hpp>

#include <efsw/efsw.hpp>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <thread>

namespace hotswap {

  namespace {

    class BuildListener : public efsw::FileWatchListener {
    public:
      explicit BuildListener(BuildOrchestrator* owner) : owner_(owner) {}

      void handleFileAction(efsw::WatchID watchid, const std::string& dir, const std::string& filename,
                            efsw::Action action, std::string oldFilename) override {
        (void)watchid;
        (void)dir;
        (void)filename;
        (void)action;
        (void)oldFilename;
        if (owner_) owner_->notifySourceChanged();
      }

    private:
      BuildOrchestrator* owner_;
    };

  }

  BuildOrchestrator::BuildOrchestrator(BuildConfig config)
    : config_(std::move(config)) {}

  BuildOrchestrator::~BuildOrchestrator() {
    stop();
  }

  void BuildOrchestrator::start() {
    finished_ = false;
    buildThread_ = std::thread(&BuildOrchestrator::buildLoop, this);
  }

  void BuildOrchestrator::stop() {
    finished_ = true;
    if (watcher_ && watchId_ >= 0) {
      watcher_->removeWatch(watchId_);
    }
    if (buildThread_.joinable()) {
      buildThread_.join();
    }
  }

  bool BuildOrchestrator::runCommand(const std::string& command) {
    int code = std::system(command.c_str());
    return code == 0;
  }

  bool BuildOrchestrator::needsCmake() {
    namespace fs = std::filesystem;
    bool need = false;
    std::set<std::pair<std::string, fs::file_time_type>> newModTimeSet;
    std::set<std::string> newFileSet;

    if (!fs::exists(config_.sourceDir)) return false;

    for (const auto& entry : fs::recursive_directory_iterator(config_.sourceDir)) {
      if (!entry.is_regular_file()) continue;
      auto path = entry.path().string();
      auto lastMod = fs::last_write_time(entry.path());
      newModTimeSet.insert({path, lastMod});
      newFileSet.insert(path);
      if (entry.path().filename() == "CMakeLists.txt" && cmakeListsModTime_ != lastMod) {
        cmakeListsModTime_ = lastMod;
        need = true;
      }
    }

    if (newFileSet != filesSet_) {
      filesSet_ = newFileSet;
      need = true;
    }

    modTimesSet_ = newModTimeSet;
    return need;
  }

  void BuildOrchestrator::runBuild() {
    namespace fs = std::filesystem;
    fs::create_directories(config_.buildDir);

    if (needsCmake()) {
      std::string cmakeSource = config_.cmakeSourceDir.empty()
        ? config_.sourceDir + "/../../.."
        : config_.cmakeSourceDir;
      std::string cmd = "cmake -S \"" + cmakeSource + "\" -B \"" + config_.buildDir +
                        "\" -DCMAKE_BUILD_TYPE=Debug 2>&1";
      if (!runCommand(cmd)) {
        std::cerr << "hotswap: cmake failed\n";
        return;
      }
    }

    std::string cmd = "cmake --build \"" + config_.buildDir + "\" --target model view 2>&1";
    if (!runCommand(cmd)) {
      std::cerr << "hotswap: build failed\n";
      return;
    }

    if (!writeManifestAtomic(config_.manifestPath, config_.modelSoPath, config_.viewSoPath)) {
      std::cerr << "hotswap: manifest write failed\n";
    }
  }

  void BuildOrchestrator::buildLoop() {
    namespace fs = std::filesystem;
    fs::create_directories(config_.buildDir);

    auto listener = std::make_unique<BuildListener>(this);
    listener_ = std::move(listener);
    watcher_ = std::make_unique<efsw::FileWatcher>();
    watchId_ = watcher_->addWatch(config_.sourceDir, listener_.get(), true);
    watcher_->watch();

    changeNumber_++;

    while (!finished_) {
      if (changeNumber_.load() == buildNumber_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        continue;
      }

      auto start = std::chrono::high_resolution_clock::now();
      runBuild();
      auto finish = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> elapsed = finish - start;
      std::cout << "hotswap build: " << elapsed.count() << "s\n";

      buildNumber_.store(changeNumber_.load());
    }
  }

}
