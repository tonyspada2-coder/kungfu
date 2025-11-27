// SPDX-License-Identifier: Apache-2.0

#ifndef YIJINJING_TIME_H
#define YIJINJING_TIME_H

#include <chrono>
#include <string>

namespace kungfu::yijinjing {
    namespace time_unit {
        constexpr int64_t NANOSECONDS_PER_MILLISECOND = 1000000;
        constexpr int64_t NANOSECONDS_PER_SECOND = 1000 * NANOSECONDS_PER_MILLISECOND;
        constexpr int64_t NANOSECONDS_PER_MINUTE = 60 * NANOSECONDS_PER_SECOND;
        constexpr int64_t NANOSECONDS_PER_HOUR = 60 * NANOSECONDS_PER_MINUTE;
        constexpr int64_t NANOSECONDS_PER_DAY = 24 * NANOSECONDS_PER_HOUR;
        constexpr int64_t UTC_OFFSET = 8 * NANOSECONDS_PER_HOUR;
    }
}

namespace kungfu::yijinjing::time {
int64_t now_in_nano();
std::string strftime(int64_t nanotime, const std::string &format = "%F %T.%N");
uint32_t nano_hashed(int64_t nano_time);
} // namespace kungfu::yijinjing::time

#endif // YIJINJING_TIME_H
