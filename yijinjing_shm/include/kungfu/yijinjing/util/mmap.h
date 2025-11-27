// SPDX-License-Identifier: Apache-2.0

#ifndef YIJINJING_MMAP_H
#define YIJINJING_MMAP_H

#include <string>

namespace kungfu::yijinjing::os {
uintptr_t load_mmap_buffer(const std::string &path, size_t size, bool is_writing = false, bool lazy = true);
bool release_mmap_buffer(uintptr_t address, [[maybe_unused]] size_t size, bool lazy);
} // namespace kungfu::yijinjing::os

#endif // YIJINJING_MMAP_H
