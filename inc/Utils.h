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

template <std::size_t S, class R, class... As>
struct Invoker
{
	uint8_t bytes_[S];
	R(*invoker)(uint8_t*, As...);

	template <class F>
	Invoker(F&& f) : invoker{[](uint8_t* bytes, As... args){return (*reinterpret_cast<F*>(bytes))(args...);}}
	{
		new(bytes_) F(f);
	}
	R operator()(As... args) {return invoker(bytes_, args...);}
};

template <class R, class Tuple, uint8_t index = 0>
R get_tuple_element(const Tuple& tuple, int runtime_index)
{
	if constexpr (std::is_same_v<std::tuple_element_t<index, Tuple>, R>)
		if (index == runtime_index)
			return std::get<index>(tuple);
	if constexpr (index + 1 < std::tuple_size_v<Tuple>)
		return get_tuple_element<R, Tuple, index + 1>(tuple, runtime_index);
	else
		throw std::out_of_range{"runtime tuple index out of range"};
}