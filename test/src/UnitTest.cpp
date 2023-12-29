// binaries\UnitTest_x64_Debug.exe --run_test=suite_abstract_test --log_level=all

#define BOOST_TEST_MODULE Test
#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

BOOST_AUTO_TEST_CASE(algorithm_test)
{
	std::list<uint8_t> vec;
    auto it = --vec.end();
    vec.push_back(10);
	BOOST_TEST_MESSAGE(*it);
}