#include <catch2/catch.hpp>
#include <kungfu/yijinjing/io.h>

TEST_CASE("Locator", "[io]") {
    auto locator = std::make_shared<kungfu::yijinjing::data::locator>();
    auto home = std::make_shared<kungfu::yijinjing::data::location>(kungfu::yijinjing::data::mode::LIVE, kungfu::yijinjing::data::category::SYSTEM, "test", "test", locator);

    auto path = locator->layout_file(home, kungfu::yijinjing::data::layout::JOURNAL, "test");
    REQUIRE(path.find("test") != std::string::npos);
}
