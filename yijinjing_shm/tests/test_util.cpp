#include <catch2/catch.hpp>
#include <kungfu/yijinjing/time.h>

TEST_CASE("Time", "[util]") {
    auto t = kungfu::yijinjing::time::now_in_nano();
    auto s = kungfu::yijinjing::time::strftime(t);
    REQUIRE(s.length() > 0);
}
