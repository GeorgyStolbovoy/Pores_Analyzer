#pragma once

#include <wx/window.h>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/container_hash/hash.hpp>

namespace mi = boost::multi_index;

class MeasureWindow : public wxWindow
{
	using pore_coord_t = std::pair<uint16_t, std::pair<std::ptrdiff_t, std::ptrdiff_t>>;

public:
	struct tag_for_insert {}; struct tag_for_search {};
	boost::multi_index_container<
		pore_coord_t,
		mi::indexed_by<
			mi::ordered_non_unique<mi::tag<tag_for_insert>, mi::member<pore_coord_t, pore_coord_t::first_type, &pore_coord_t::first>>,
			mi::hashed_unique<
				mi::tag<tag_for_search>,
				mi::member<pore_coord_t, pore_coord_t::second_type, &pore_coord_t::second>,
				boost::hash<pore_coord_t::second_type>,
				decltype([](const pore_coord_t::second_type& lhs, const pore_coord_t::second_type& rhs){return lhs.first == rhs.first && lhs.second == rhs.second;})
			>
		>
	> m_pores;

	MeasureWindow();
	void Measure();
};