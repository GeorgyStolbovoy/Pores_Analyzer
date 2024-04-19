// binaries\UnitTest_x64_Debug.exe --run_test=suite_abstract_test --log_level=all

#define BOOST_TEST_MODULE Test
#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/hana/all.hpp>

struct S
{
	int a;
	boost::hana::optional<int> b;
	int c = 0;
};

int foo(const S& s)
{
	return s.a + s.b.value() + s.c;
}

BOOST_AUTO_TEST_CASE(algorithm_test)
{
	BOOST_TEST_MESSAGE(foo({.a = 1, .c = 3}));
}