#include <hotswap/BuildOrchestrator.hpp>
#include <hotswap/ManifestWriter.hpp>

#include <efsw/efsw.hpp>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
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

    double elapsedSeconds(
      std::chrono::high_resolution_clock::time_point start,
      std::chrono::high_resolution_clock::time_point finish
    ) {
      return std::chrono::duration<double>(finish - start).count();
    }

    void logTiming(const char* label, double seconds) {
      std::cout << "hotswap " << label << ": " << std::fixed << std::setprecision(3) << seconds << "s\n";
    }

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

  BuildOrchestrator::ScanResult BuildOrchestrator::scanSources() {
    namespace fs = std::filesystem;
    ScanResult result;
    const bool firstScan = filesSet_.empty();

    if (!fs::exists(config_.sourceDir)) return result;

    std::map<std::string, fs::file_time_type> previousTimes;
    for (const auto& entry : modTimesSet_) {
      previousTimes[entry.first] = entry.second;
    }

    std::set<std::pair<std::string, fs::file_time_type>> newModTimeSet;
    std::set<std::string> newFileSet;
    std::vector<std::string> changedPaths;

    for (const auto& entry : fs::recursive_directory_iterator(config_.sourceDir)) {
      if (!entry.is_regular_file()) continue;
      auto path = entry.path().string();
      auto lastMod = fs::last_write_time(entry.path());
      newModTimeSet.insert({path, lastMod});
      newFileSet.insert(path);

      if (entry.path().filename() == "CMakeLists.txt") {
        if (hasCmakeListsModTime_ && cmakeListsModTime_ != lastMod) {
          result.needCmake = true;
        }
        cmakeListsModTime_ = lastMod;
        hasCmakeListsModTime_ = true;
      }

      if (!firstScan) {
        auto previous = previousTimes.find(path);
        if (previous == previousTimes.end() || previous->second != lastMod) {
          changedPaths.push_back(path);
        }
      }
    }

    if (!fs::exists(fs::path(config_.buildDir) / "CMakeCache.txt")) {
      result.needCmake = true;
    }

    if (newFileSet != filesSet_) {
      if (!firstScan) {
        result.needCmake = true;
        result.buildModel = true;
        result.buildView = true;
      }
      filesSet_ = newFileSet;
    }

    if (newModTimeSet != modTimesSet_) {
      result.needNinja = true;
      modTimesSet_ = newModTimeSet;
    }

    if (firstScan) {
      result.needNinja = true;
      result.buildModel = true;
      result.buildView = true;
      return result;
    }

    if (result.needCmake) {
      result.buildModel = true;
      result.buildView = true;
      result.needNinja = true;
      return result;
    }

    fs::path sourceRoot(config_.sourceDir);
    for (const auto& path : changedPaths) {
      std::error_code ec;
      auto relative = fs::relative(path, sourceRoot, ec);
      if (ec) {
        result.buildModel = true;
        result.buildView = true;
        continue;
      }
      std::string first = relative.empty() ? std::string() : relative.begin()->string();
      if (first == "view") result.buildView = true;
      else if (first == "model") result.buildModel = true;
      else {
        result.buildModel = true;
        result.buildView = true;
      }
    }

    if (!changedPaths.empty()) result.needNinja = true;
    return result;
  }

  std::vector<std::string> BuildOrchestrator::ninjaTargets(const ScanResult& scan) const {
    std::vector<std::string> targets;
    if (scan.buildModel) targets.push_back("model");
    if (scan.buildView) targets.push_back("view");
    return targets;
  }

  void BuildOrchestrator::runBuild() {
    namespace fs = std::filesystem;
    fs::create_directories(config_.buildDir);

    auto scanStart = std::chrono::high_resolution_clock::now();
    ScanResult scan = scanSources();
    logTiming("scan", elapsedSeconds(scanStart, std::chrono::high_resolution_clock::now()));

    if (scan.needCmake) {
      std::string cmakeSource = config_.cmakeSourceDir.empty()
        ? config_.sourceDir + "/../../.."
        : config_.cmakeSourceDir;
      std::string cmd = "cmake -S \"" + cmakeSource + "\" -B \"" + config_.buildDir +
                        "\" -DCMAKE_BUILD_TYPE=Debug 2>&1";
      auto cmakeStart = std::chrono::high_resolution_clock::now();
      if (!runCommand(cmd)) {
        std::cerr << "hotswap: cmake failed\n";
        return;
      }
      logTiming("cmake", elapsedSeconds(cmakeStart, std::chrono::high_resolution_clock::now()));
    }

    if (!scan.needNinja) return;

    auto targets = ninjaTargets(scan);
    if (targets.empty()) {
      targets.push_back("model");
      targets.push_back("view");
    }

    fs::path ninjaFile = fs::path(config_.buildDir) / "build.ninja";
    if (!fs::exists(ninjaFile)) {
      std::cerr << "hotswap: missing " << ninjaFile << " — run cmake with Ninja generator\n";
      return;
    }

    for (const auto& target : targets) {
      std::string cmd = "ninja -C \"" + config_.buildDir + "\" " + target + " 2>&1";
      auto ninjaStart = std::chrono::high_resolution_clock::now();
      if (!runCommand(cmd)) {
        std::cerr << "hotswap: ninja " << target << " failed\n";
        return;
      }
      logTiming(("ninja " + target).c_str(), elapsedSeconds(ninjaStart, std::chrono::high_resolution_clock::now()));
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

      runBuild();
      buildNumber_.store(changeNumber_.load());
    }
  }

}
