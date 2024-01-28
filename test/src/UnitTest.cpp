// binaries\UnitTest_x64_Debug.exe --run_test=suite_abstract_test --log_level=all

#define BOOST_TEST_MODULE Test
#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

BOOST_AUTO_TEST_CASE(algorithm_test)
{
	constexpr std::initializer_list<std::ptrdiff_t> ofs{0, 1, 1, 1, 1, 0, 1, -1, 0, -1, -1, -1, -1, 0, -1, 1};
	for (auto ofs_it = ofs.begin(), ofs_end = ofs.end(); ofs_it != ofs_end; ++ofs_it)
		BOOST_TEST_MESSAGE(*ofs_it);
}