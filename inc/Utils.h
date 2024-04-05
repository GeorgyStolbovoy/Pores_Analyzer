#pragma once

template <class A, class B>
A nearest_integer(B x)
{
	if (A r = x; std::abs(x - r) < 0.5)
		return r;
	else
		return x > 0 ? r + 1 : r - 1;
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

#define SWITCHER(body, macro_cond, case_1, case_2) [=](arg_t arg)->ret_t {body(arg, macro_cond, case_1, case_2)}
#define SWITCHERS_SIZES(r, prod) (sizeof(decltype(SWITCHER(BOOST_PP_SEQ_ELEM(0, prod), BOOST_PP_SEQ_ELEM(1, prod), BOOST_PP_SEQ_ELEM(2, prod), BOOST_PP_SEQ_ELEM(3, prod)))))
#define RETURN_SWITCHERS(macro_cond, body) \
		using return_t = Invoker<std::max({BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH_PRODUCT(SWITCHERS_SIZES, ((body))((macro_cond))((0)(1))((0)(1))))}), ret_t, arg_t>; \
		if (case_1 && case_2) \
			return return_t{SWITCHER(body, macro_cond, 1, 1)}; \
		else if (case_1) \
			return return_t{SWITCHER(body, macro_cond, 1, 0)}; \
		else if (case_2) \
			return return_t{SWITCHER(body, macro_cond, 0, 1)}; \
		else \
			return return_t{SWITCHER(body, macro_cond, 0, 0)};
#define STATIC_SWITCHER(ret_type, arg_type, body, constexpr_cond, case_1_cond, case_2_cond) \
	using ret_t = BOOST_PP_REMOVE_PARENS(ret_type); \
	using arg_t = BOOST_PP_REMOVE_PARENS(arg_type); \
	bool case_1 = BOOST_PP_REMOVE_PARENS(case_1_cond), \
		case_2 = BOOST_PP_REMOVE_PARENS(case_2_cond); \
	if constexpr (BOOST_PP_REMOVE_PARENS(constexpr_cond)) { \
		RETURN_SWITCHERS(1, body) \
	} else { \
		RETURN_SWITCHERS(0, body) \
	}

#define GET_STATIC_SWITCHER(ret_type, arg_type, body, constexpr_cond, case_1_cond, case_2_cond) [](){STATIC_SWITCHER(ret_type, arg_type, body, constexpr_cond, case_1_cond, case_2_cond)}()
#define CONDITIONAL(constexpr_cond, true_branch, false_branch, ...) [__VA_ARGS__](){if constexpr (constexpr_cond) {return BOOST_PP_REMOVE_PARENS(true_branch);} else {return BOOST_PP_REMOVE_PARENS(false_branch);}}()