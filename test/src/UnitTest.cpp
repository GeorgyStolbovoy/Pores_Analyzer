// binaries\UnitTest_x64_Debug.exe --run_test=suite_abstract_test --log_level=all

#define BOOST_TEST_MODULE Test
#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

BOOST_AUTO_TEST_CASE(algorithm_test)
{
	unsigned char* bm_buf = new unsigned char[300'000];
    memset(bm_buf, 0, 300'000);
    auto ptr = bm_buf;
    for (int i = 0, end = 300'000; i < end; ++i)
    {
        ++ptr;
        auto jjj = ptr - bm_buf;
        auto ddd = ptr[2];
    }
    delete bm_buf;
}