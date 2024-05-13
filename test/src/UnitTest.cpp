// binaries\UnitTest_x64_Debug.exe --run_test=suite_abstract_test --log_level=all

#define BOOST_TEST_MODULE Test
#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/logic/tribool.hpp>

BOOST_AUTO_TEST_CASE(algorithm_test)
{
	constexpr auto s1 = sizeof(std::pair<bool, bool>);
	constexpr auto s2 = sizeof(boost::tribool);

	BOOST_TEST_MESSAGE(f.foo.operator()<true>());
	BOOST_TEST_MESSAGE(f.foo.operator()<false>());
}