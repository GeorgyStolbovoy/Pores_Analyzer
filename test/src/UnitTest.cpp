// binaries\UnitTest_x64_Debug.exe --run_test=suite_abstract_test --log_level=all

#define BOOST_TEST_MODULE Test
#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include "Utils.h"

struct F
{
	int operator()(int i) const {return 42;}
};

auto foo()
{
	auto l = []<bool b>(int){return 42;};
	return &l.operator()<true>;
}

BOOST_AUTO_TEST_CASE(algorithm_test)
{
	Invoker<sizeof(F), int, int> exe1{F{}};
	BOOST_TEST_MESSAGE(exe1(1));

	auto lam = [f = F{}](double a, double b, double c){return a*b*c*f(100);};
	Invoker<sizeof(lam), double, double, double, double> exe2{std::move(lam)};
	BOOST_TEST_MESSAGE(exe2(10.0, 20.0, 5.0));
}