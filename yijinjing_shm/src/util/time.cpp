// SPDX-License-Identifier: Apache-2.0

#include <chrono>
#include <ctime>
#include <fmt/format.h>
#include <regex>

#include <kungfu/common.h>
#include <kungfu/yijinjing/time.h>
#include <kungfu/yijinjing/util/hash.h>

using namespace std::chrono;

namespace kungfu::yijinjing {

#ifdef __linux__

// Make sure to use clock_gettime instead of syscall to have better performance
// https://stackoverflow.com/questions/70444744/c-linux-fastest-way-to-measure-time-faster-than-stdchrono-benchmark-incl
// https://github.com/gcc-mirror/gcc/blob/releases/gcc-11.3.0/libstdc%2B%2B-v3/src/c%2B%2B11/chrono.cc

inline int64_t get_clock_count(clockid_t clk_id) {
  timespec ts;
  clock_gettime(clk_id, &ts);
  return ts.tv_sec * time_unit::NANOSECONDS_PER_SECOND + ts.tv_nsec;
}

inline int64_t system_clock_count() { return get_clock_count(CLOCK_REALTIME); }

inline int64_t steady_clock_count() { return get_clock_count(CLOCK_MONOTONIC); }

#else

#define NOW_SINCE_EPOCH(clock) clock::now().time_since_epoch()

inline int64_t system_clock_count() { return duration_cast<nanoseconds>(NOW_SINCE_EPOCH(system_clock)).count(); }

inline int64_t steady_clock_count() { return NOW_SINCE_EPOCH(steady_clock).count(); }

#endif

namespace time {
static int64_t a = kungfu::yijinjing::system_clock_count();
static int64_t b = kungfu::yijinjing::steady_clock_count();

int64_t now_in_nano() {
  auto duration = kungfu::yijinjing::steady_clock_count() - b;
  return a + duration;
}

uint32_t nano_hashed(int64_t nano_time) {
  return kungfu::yijinjing::util::hash_str_32(std::to_string(nano_time));
}

std::string strftime(int64_t nanotime, const std::string &format) {
  if (nanotime == INT64_MAX) {
    return "end of world";
  }
  time_point<steady_clock> tp_steady((nanoseconds(nanotime)));
  auto tp_epoch_steady = time_point<steady_clock>{};
  auto tp_diff = tp_steady - tp_epoch_steady;

  auto tp_epoch_system = time_point<system_clock>{};
  std::time_t time_since_epoch =
      system_clock::to_time_t(tp_epoch_system + duration_cast<system_clock::duration>(tp_diff));

  std::string normal_format = format;
  std::regex nano_format_regex("%N");
  if (std::regex_search(format, nano_format_regex)) {
    int64_t nano = tp_diff.count() % time_unit::NANOSECONDS_PER_SECOND;
    normal_format = std::regex_replace(format, nano_format_regex, fmt::format("{:09d}", nano));
  }

  std::ostringstream oss;
  oss << std::put_time(std::localtime(&time_since_epoch), normal_format.c_str());
  if (nanotime > 0) {
    return oss.str();
  } else if (nanotime == 0) {
    return std::regex_replace(oss.str(), std::regex("\\d"), "0");
  } else {
    return std::regex_replace(oss.str(), std::regex("\\d"), "#");
  }
}
}
}
