// SPDX-License-Identifier: Apache-2.0

#ifndef YIJINJING_HASH_H
#define YIJINJING_HASH_H

#include <string>

namespace kungfu::yijinjing::util {
uint32_t hash_str_32(const std::string &key, uint32_t seed = 0);
} // namespace kungfu::yijinjing::util

#endif // YIJINJING_HASH_H
