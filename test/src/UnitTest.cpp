// binaries\UnitTest_x64_Debug.exe --run_test=suite_abstract_test --log_level=all

#define BOOST_TEST_MODULE Test
#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <thread>
#include <mutex>

BOOST_AUTO_TEST_CASE(algorithm_test)
{
	std::mutex mx;
    std::thread t{[lock = std::unique_lock{mx}]() mutable {std::this_thread::sleep_for(std::chrono::seconds(1)); BOOST_TEST_MESSAGE("1"); lock.unlock();}};
    std::lock_guard{mx};
    t.join();
    BOOST_TEST_MESSAGE("2");
}