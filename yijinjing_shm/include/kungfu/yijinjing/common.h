// SPDX-License-Identifier: Apache-2.0

#ifndef KUNGFU_YIJINJING_COMMON_H
#define KUNGFU_YIJINJING_COMMON_H

#include <csignal>
#include <cstdarg>
#include <filesystem>
#include <rxcpp/rx.hpp>

#include <kungfu/common.h>
#include <kungfu/yijinjing/util/stacktrace.h>
#include <kungfu/yijinjing/util/util.h>
#include <nng/compat/nanomsg/nn.h>

namespace kungfu {
namespace yijinjing {
/** size related */
constexpr int KB = 1024;
constexpr int MB = KB * KB;

class yijinjing_error : public std::runtime_error {
public:
  explicit yijinjing_error(const std::string &message) : runtime_error(message) {}
};

class resource {
public:
  virtual bool is_usable() = 0;

  virtual void setup() = 0;
};

class publisher : public resource {
public:
  virtual ~publisher() = default;

  virtual int notify() = 0;

  virtual int publish(const std::string &json_message, int flags = NN_DONTWAIT) = 0;
};

DECLARE_PTR(publisher)

class observer : public resource {
public:
  virtual ~observer() = default;

  virtual bool wait() = 0;

  virtual const std::string &get_notice() = 0;
};

DECLARE_PTR(observer)

namespace data {
FORWARD_DECLARE_STRUCT_PTR(location)
FORWARD_DECLARE_CLASS_PTR(locator)
typedef std::unordered_map<uint32_t, location_ptr> location_map;

enum class mode : int { LIVE, DATA, REPLAY, BACKTEST };

enum class category : int { MD, TD, STRATEGY, SYSTEM };

enum class layout : int { JOURNAL, SQLITE, LOG };

class locator {
public:
  locator();

  explicit locator(mode m, const std::vector<std::string> &tag = {});

  explicit locator(const std::string &root) : root_(root), dir_mode_(mode::LIVE) {}

  virtual ~locator() = default;

  [[nodiscard]] virtual bool has_env(const std::string &name) const;

  [[nodiscard]] virtual std::string get_env(const std::string &name) const;

  [[nodiscard]] virtual std::string layout_dir(const location_ptr &location, layout l) const;

  [[nodiscard]] virtual std::string layout_file(const location_ptr &location, layout l,
                                                const std::string &name) const;

  [[maybe_unused]] [[maybe_unused]] [[nodiscard]] virtual std::string
  default_to_system_db(const location_ptr &location, const std::string &name) const;

  [[nodiscard]] virtual std::vector<uint32_t> list_page_id(const location_ptr &location, uint32_t dest_id) const;

  [[nodiscard]] virtual std::vector<location_ptr> list_locations(const std::string &category, const std::string &group,
                                                                 const std::string &name,
                                                                 const std::string &mode) const;

  [[nodiscard]] virtual std::vector<uint32_t> list_location_dest(const location_ptr &location) const;

  [[nodiscard]] virtual std::vector<uint32_t> list_location_dest_by_db(const location_ptr &location) const;

  bool operator==(const locator &another) const;

private:
  std::filesystem::path root_;
  mode dir_mode_;
};

struct location : public std::enable_shared_from_this<location> {
  static constexpr uint32_t PUBLIC = 0;
  static constexpr uint32_t SYNC = 1;

  const locator_ptr locator;
  const std::string uname;
  const uint32_t uid;
  mode mode_;
  category category_;
  std::string group;
  std::string name;
  uint32_t location_uid;

  location(mode m, category c, std::string g, std::string n, locator_ptr l)
      : locator(std::move(l)), uname(fmt::format("{}/{}/{}/{}", (int)c, g, n, (int)m)),
        uid(util::hash_str_32(uname)) {
    category_ = c;
    group = std::move(g);
    name = std::move(n);
    mode_ = m;
    location_uid = uid;
  }
};
} // namespace data
} // namespace yijinjing
} // namespace kungfu
#endif // KUNGFU_YIJINJING_COMMON_H
