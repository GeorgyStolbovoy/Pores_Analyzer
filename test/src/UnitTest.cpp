// binaries\UnitTest_x64_Debug.exe --run_test=suite_abstract_test --log_level=all

#define BOOST_TEST_MODULE Test
#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

BOOST_AUTO_TEST_CASE(algorithm_test)
{
	double x = 35.85, y = 29.888, e = 0.001, c = -0.1, a = std::pow(10, c);
	for (double xk = x, prev = xk + 1; std::abs(xk - prev) > e; prev = xk, xk = a*y*std::pow(xk, a-1) - a*std::pow(xk, 2*a-1) + x)
		BOOST_TEST_MESSAGE(xk << " ; y = " << std::pow(xk, a) << " ; X = " << a*y*(a - 1)*std::pow(xk, a - 2) - a*(2*a - 1)*std::pow(xk, 2*a - 2));
}