// SPDX-License-Identifier: Apache-2.0

//
// Created by Keren Dong on 2020/3/25.
//
#include <fstream>
#include <kungfu/common.h>
#include <kungfu/yijinjing/io.h>
#include <kungfu/yijinjing/log.h>
#include <kungfu/yijinjing/time.h>
#include <tabulate/table.hpp>

#define TIME_FORMAT "%T.%N"

using namespace tabulate;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::journal;

namespace kungfu::yijinjing {
io_device_console::io_device_console(data::location_ptr home, int32_t console_width, int32_t console_height)
    : io_device(std::move(home), false, true), console_width_(console_width), console_height_(console_height) {}
} // namespace kungfu::yijinjing
