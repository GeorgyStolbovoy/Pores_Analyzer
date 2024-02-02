// binaries\UnitTest_x64_Debug.exe --run_test=suite_abstract_test --log_level=all

#define BOOST_TEST_MODULE Test
#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

BOOST_AUTO_TEST_CASE(algorithm_test)
{
	uint32_t deleted_id = 0;
	auto test = [this, &deleted_id]()
	{
		BOOST_TEST_MESSAGE(deleted_id);
	};
	deleted_id = 5;
	test();
}