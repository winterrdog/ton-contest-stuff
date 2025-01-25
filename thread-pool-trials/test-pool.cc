#include <gtest/gtest.h>

#include "pool.hpp"

using dp::thread_pool;

TEST(ThreadPoolTest, BasicFunctionality)
{
    const unsigned limit = 200'000;
    int num_threads = std::thread::hardware_concurrency();

    std::cout << "Number of threads: " << num_threads << '\n';
    thread_pool pool(num_threads);
    std::vector<std::future<unsigned>> results{};

    for (unsigned i = 0; i < limit; i++)
    {
        results.emplace_back(pool.enqueue([i] { return i * i; }));
    }

    // verify output
    for (unsigned i = 0; i < limit; i++)
    {
        ASSERT_EQ(results[i].get(), i * i);
    }
}
