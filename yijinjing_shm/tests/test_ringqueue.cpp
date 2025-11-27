#include <catch2/catch.hpp>
#include <kungfu/yijinjing/cache/ringqueue.h>
#include <thread>
#include <vector>

TEST_CASE("RingQueue", "[cache]") {
    kungfu::yijinjing::cache::ringqueue<int> queue(1024);

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 100; ++j) {
                queue.push(j);
            }
        });
    }

    for (auto &t : threads) {
        t.join();
    }

    REQUIRE(queue.size() == 1000);

    int *result;
    for (int i = 0; i < 1000; ++i) {
        REQUIRE(queue.pop(result));
    }
}
