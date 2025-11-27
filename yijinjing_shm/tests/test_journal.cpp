#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <kungfu/yijinjing/journal/journal.h>
#include <kungfu/yijinjing/io.h>
#include <kungfu/yijinjing/journal/nop.h>

TEST_CASE("Journal", "[journal]") {
    auto home = std::make_shared<kungfu::yijinjing::data::location>(kungfu::yijinjing::data::mode::LIVE, kungfu::yijinjing::data::category::SYSTEM, "test", "test", std::make_shared<kungfu::yijinjing::data::locator>());
    auto writer = std::make_shared<kungfu::yijinjing::journal::writer>(home, 0, false, std::make_shared<kungfu::yijinjing::noop_publisher>());
    REQUIRE(writer->current_frame_uid() > 0);
    auto frame = writer->open_frame(0, 1, 1024);
    REQUIRE(frame->msg_type() == 1);
    writer->close_frame(1024, kungfu::yijinjing::time::now_in_nano());
}
