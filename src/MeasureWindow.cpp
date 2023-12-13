#include "MeasureWindow.h"
#include "ImageWindow.h"

#define TMP_DIF 150

void MeasureWindow::Measure()
{
	struct pore_inspector
	{
		using view_t = decltype(ImageWindow::image_current)::view_t;

		MeasureWindow* self;
		view_t view = gil::view(ImageWindow::image_current);
		view_t::xy_locator loc = view.xy_at(0, 0);
		view_t::xy_locator::cached_location_t
#define CACHED_LOC(pos, dx, dy) pos = loc.cache_location(dx, dy)
			CACHED_LOC(cl_lt, -1, -1),	CACHED_LOC(cl_t, 0, -1),CACHED_LOC(cl_rt, 1, -1),
			CACHED_LOC(cl_l, -1, 0),							CACHED_LOC(cl_r, 1, 0),
			CACHED_LOC(cl_lb, -1, 1),	CACHED_LOC(cl_b, 0, 1),	CACHED_LOC(cl_rb, 1, 1);
#undef CACHED_LOC
		std::ptrdiff_t height = view.height(), width = view.width();
		uint16_t count = 0;

		pore_inspector(MeasureWindow* self) : self(self)
		{
			for (std::ptrdiff_t y = 0; y < height-1; ++y, loc += gil::point(-width+1, 1))
			{
				for (std::ptrdiff_t x = 0; x < width-1; ++x, ++loc.x())
				{
					inspect_pore(view.xy_at(x, y), x, y);
				}
			}
		}
		void inspect_pore(const view_t::xy_locator& loc_cur, std::ptrdiff_t x, std::ptrdiff_t y)
		{
			if (self->m_pores.get<tag_for_search>().contains(pore_coord_t::second_type{x, y}))
				return;

			if (x-1 >= 0 /*|| x == width*/ || y-1 >= 0 /*|| y == height*/ || *loc_cur - loc_cur[cl_lt] < TMP_DIF)
				inspect_pore(loc_cur + gil::point(-1, -1), x-1, y-1);

			self->m_pores.get<tag_for_insert>().insert({count, {x, y}});
		}
	} inspector{this};
}
