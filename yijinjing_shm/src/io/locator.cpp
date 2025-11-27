// SPDX-License-Identifier: Apache-2.0

//
// Created by Keren Dong on 2021/5/21.
//

#include <algorithm>
#include <cstdlib>
#include <kungfu/common.h>
#include <kungfu/yijinjing/common.h>
#include <regex>

namespace kungfu::yijinjing::data {

namespace fs = std::filesystem;

const std::string get_category_name(category c) {
    switch(c) {
        case category::MD: return "md";
        case category::TD: return "td";
        case category::STRATEGY: return "strategy";
        case category::SYSTEM: return "system";
        default: return "unknown";
    }
}

const std::string get_mode_name(mode m) {
    switch(m) {
        case mode::LIVE: return "live";
        case mode::DATA: return "data";
        case mode::REPLAY: return "replay";
        case mode::BACKTEST: return "backtest";
        default: return "unknown";
    }
}

const std::string get_layout_name(layout l) {
    switch(l) {
        case layout::JOURNAL: return "journal";
        case layout::SQLITE: return "sqlite";
        case layout::LOG: return "log";
        default: return "unknown";
    }
}

mode get_mode_by_name(const std::string& name) {
    if (name == "live") return mode::LIVE;
    if (name == "data") return mode::DATA;
    if (name == "replay") return mode::REPLAY;
    if (name == "backtest") return mode::BACKTEST;
    return mode::LIVE;
}

category get_category_by_name(const std::string& name) {
    if (name == "md") return category::MD;
    if (name == "td") return category::TD;
    if (name == "strategy") return category::STRATEGY;
    if (name == "system") return category::SYSTEM;
    return category::SYSTEM;
}


fs::path get_default_root() {
  char *kf_home = std::getenv("KF_HOME");
  if (kf_home != nullptr) {
    return fs::path{kf_home};
  }
#ifdef _WINDOWS
  auto appdata = std::getenv("APPDATA");
  auto root = fs::path(appdata);
#elif __APPLE__
  auto user_home = std::getenv("HOME");
  auto root = fs::path(user_home) / "Library" / "Application Support";
#elif __linux__
  auto user_home = std::getenv("HOME");
  auto root = fs::path(user_home) / ".config";
#endif // _WINDOWS
  return root / "kungfu" / "home";
}

std::string get_runtime_dir() {
  auto runtime_dir = std::getenv("KF_RUNTIME_DIR");
  if (runtime_dir != nullptr) {
    return runtime_dir;
  }
  return (get_default_root() / "runtime").string();
}

std::string get_root_dir(mode m, const std::vector<std::string> &tags) {
  static const std::unordered_map<mode, std::pair<std::string, std::string>> map_env = {
      {mode::LIVE, std::pair("KF_RUNTIME_DIR", "runtime")},
      {mode::BACKTEST, std::pair("KF_BACKTEST_DIR", "backtest")},
      {mode::DATA, std::pair("KF_DATASET_DIR", "dataset")},
      {mode::REPLAY, std::pair("KF_REPLAY_DIR", "replay")},
  };

  auto iter = map_env.find(m);
  if (iter == map_env.end()) {
    SPDLOG_WARN("unrecognized mode: {}, init root_ as mode::LIVE", (int)m);
    return get_runtime_dir();
  } else {
    auto dir_path = std::getenv(iter->second.first.c_str());
    if (dir_path != nullptr) {
      return dir_path;
    }
    auto home_dir_path = get_default_root() / iter->second.second;
    home_dir_path /= std::accumulate(tags.begin(), tags.end(), fs::path{},
                                     [&](const fs::path &p, const std::string &tag) { return p / tag; });
    return home_dir_path.string();
  }
}

locator::locator() : root_(get_runtime_dir()), dir_mode_(mode::LIVE) {}

locator::locator(mode m, const std::vector<std::string> &tags) : dir_mode_(m) {
  root_ = get_root_dir(dir_mode_, tags);
}

bool locator::has_env(const std::string &name) const { return std::getenv(name.c_str()) != nullptr; }

std::string locator::get_env(const std::string &name) const { return std::getenv(name.c_str()); }

std::string locator::layout_dir(const location_ptr &location, layout l) const {
  auto dir = root_ /                                     //
             get_category_name(location->category_) / //
             location->group /                           //
             location->name /                            //
             get_layout_name(l) /               //
             get_mode_name(location->mode_);
  if (not fs::exists(dir)) {
    fs::create_directories(dir);
  }
  return dir.string();
}

std::string locator::layout_file(const location_ptr &location, layout l, const std::string &name) const {
  auto path = fs::path(layout_dir(location, l)) / fmt::format("{}.{}", name, get_layout_name(l));
  return path.string();
}

[[maybe_unused]] std::string locator::default_to_system_db(const location_ptr &location,
                                                           const std::string &name) const {
  auto sqlite_layout = layout::SQLITE;
  auto db_file = layout_file(location, sqlite_layout, name);
  if (not fs::exists(db_file)) {
    auto system_db_file = root_ /                                     //
                          get_category_name(location->category_) / //
                          location->group /                           //
                          location->name /                            //
                          get_layout_name(sqlite_layout) /        //
                          get_mode_name(location->mode_);
    fs::copy(system_db_file, db_file);
  }
  return db_file;
}

std::vector<uint32_t> locator::list_page_id(const location_ptr &location, uint32_t dest_id) const {
  std::vector<uint32_t> result = {};
  auto dest_id_str = fmt::format("{:08x}", dest_id);
  auto dir = fs::path(layout_dir(location, layout::JOURNAL));
  for (auto &it : fs::recursive_directory_iterator(dir)) {
    auto basename = it.path().stem();
    if (it.is_regular_file() and it.path().extension() == ".journal" and basename.stem() == dest_id_str) {
      auto index = std::atoi(basename.extension().string().c_str() + 1);
      result.push_back(index);
    }
  }
  std::sort(result.begin(), result.end());
  return result;
}

static constexpr auto w = [](const std::string &pattern) { return pattern == "*" ? ".*" : pattern; };

static constexpr auto g = [](const std::string &pattern) { return fmt::format("({})", w(pattern)); };

std::vector<location_ptr> locator::list_locations(const std::string &category, const std::string &group,
                                                  const std::string &name, const std::string &mode) const {
  fs::path search_path = root_ / g(category) / g(group) / g(name) / "journal" / g(mode);
  std::string pattern = std::regex_replace(search_path.string(), std::regex("\\\\"), "\\\\");
  std::regex search_regex(pattern);
  std::vector<location_ptr> result = {};
  std::smatch match;
  for (auto &it : fs::recursive_directory_iterator(root_)) {
    auto path = it.path().string();
    if (it.is_directory() and std::regex_match(path, match, search_regex)) {
      auto l = std::make_shared<location>(get_mode_by_name(match[4].str()),     //
                                     get_category_by_name(match[1].str()), //
                                     match[2].str(),                           //
                                     match[3].str(),                           //
                                     std::make_shared<locator>(root_.string()));
      result.push_back(l);
    }
  }
  return result;
}

std::vector<uint32_t> locator::list_location_dest(const location_ptr &location) const {
  std::unordered_set<uint32_t> set = {};
  auto dir = fs::path(layout_dir(location, layout::JOURNAL));
  for (auto &it : fs::recursive_directory_iterator(dir)) {
    auto basename = it.path().stem();
    if (it.is_regular_file() and it.path().extension() == ".journal") {
      set.emplace(std::stoul(basename.stem(), nullptr, 16));
    }
  }
  return std::vector<uint32_t>{set.begin(), set.end()};
}

std::vector<uint32_t> locator::list_location_dest_by_db(const location_ptr &location) const {
  std::unordered_set<uint32_t> set = {};
  auto dir = fs::path(layout_dir(location, layout::SQLITE));
  for (auto &it : fs::recursive_directory_iterator(dir)) {
    auto basename = it.path().stem();
    if (it.is_regular_file() and it.path().extension() == ".db") {
      set.emplace(std::stoul(basename.stem(), nullptr, 16));
    }
  }
  return std::vector<uint32_t>{set.begin(), set.end()};
}

bool locator::operator==(const locator &another) const {
  return dir_mode_ == another.dir_mode_ and root_.string() == another.root_.string();
}
} // namespace kungfu::yijinjing::data
