// SPDX-License-Identifier: Apache-2.0

#ifndef YIJINJING_NOP_H
#define YIJINJING_NOP_H

#include <kungfu/yijinjing/common.h>

namespace kungfu::yijinjing {
struct noop_publisher : public publisher {
  noop_publisher() = default;
  bool is_usable() override { return true; }
  void setup() override {}
  int notify() override { return 0; }
  int publish(const std::string &json_message, int flags = 0) override { return 0; }
};
} // namespace kungfu::yijinjing

#endif // YIJINJING_NOP_H
