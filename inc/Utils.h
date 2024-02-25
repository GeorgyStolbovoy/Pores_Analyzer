#pragma once

template <class A, class B>
A nearest_integer(B x)
{
	if (A r = x; std::abs(x - r) < 0.5)
		return r;
	else
		return x > 0 ? r + 1 : r - 1;
}

template <bool Cond, class A, class B>
constexpr decltype(auto) conditional(A&& a, B&& b)
{
	if constexpr (Cond)
		return std::forward<A>(a);
	else
		return std::forward<B>(b);
}