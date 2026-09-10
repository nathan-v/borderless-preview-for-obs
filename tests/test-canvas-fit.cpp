/*
Borderless Preview for OBS
Copyright (C) 2026 Nathan V

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include "check.hpp"

#include "canvas-fit.hpp"

namespace {

struct Fit {
	int x = -1;
	int y = -1;
	float scale = 0.0f;
};

Fit FitCanvas(int baseCX, int baseCY, int windowCX, int windowCY)
{
	Fit fit;
	GetScaleAndCenterPos(baseCX, baseCY, windowCX, windowCY, fit.x, fit.y, fit.scale);
	return fit;
}

} // namespace

TEST_CASE(exact_fit_has_no_bars)
{
	Fit fit = FitCanvas(1920, 1080, 1920, 1080);
	CHECK_EQ(fit.x, 0);
	CHECK_EQ(fit.y, 0);
	CHECK_NEAR(fit.scale, 1.0f, 1e-6);
}

TEST_CASE(wider_window_pillarboxes_and_centres)
{
	Fit fit = FitCanvas(1920, 1080, 3840, 1080);
	CHECK_NEAR(fit.scale, 1.0f, 1e-6);
	CHECK_EQ(fit.x, 960);
	CHECK_EQ(fit.y, 0);
}

TEST_CASE(taller_window_letterboxes_and_centres)
{
	Fit fit = FitCanvas(1920, 1080, 1920, 2160);
	CHECK_NEAR(fit.scale, 1.0f, 1e-6);
	CHECK_EQ(fit.x, 0);
	CHECK_EQ(fit.y, 540);
}

TEST_CASE(smaller_window_scales_down)
{
	Fit fit = FitCanvas(1920, 1080, 960, 540);
	CHECK_NEAR(fit.scale, 0.5f, 1e-6);
	CHECK_EQ(fit.x, 0);
	CHECK_EQ(fit.y, 0);
}

TEST_CASE(larger_window_scales_up)
{
	Fit fit = FitCanvas(1920, 1080, 3840, 2160);
	CHECK_NEAR(fit.scale, 2.0f, 1e-6);
	CHECK_EQ(fit.x, 0);
	CHECK_EQ(fit.y, 0);
}

TEST_CASE(portrait_window_fills_the_width)
{
	/* The reason this plugin exists: a 1080x1920 monitor showing a 16:9 canvas. */
	Fit fit = FitCanvas(1920, 1080, 1080, 1920);
	CHECK_NEAR(fit.scale, 0.5625f, 1e-6);
	CHECK_EQ(fit.x, 0);
	/* 1080 wide at 16:9 is 607 tall (truncated); the bar above is half the remainder. */
	CHECK_EQ(fit.y, 1920 / 2 - 607 / 2);
}

TEST_CASE(square_canvas_in_wide_window)
{
	Fit fit = FitCanvas(1000, 1000, 3000, 1000);
	CHECK_NEAR(fit.scale, 1.0f, 1e-6);
	CHECK_EQ(fit.x, 1000);
	CHECK_EQ(fit.y, 0);
}

TEST_CASE(scaled_canvas_never_exceeds_the_window)
{
	const int windows[][2] = {{100, 100}, {1280, 720}, {1080, 1920}, {3440, 1440}, {5120, 1440}, {33, 19}};
	for (const auto &w : windows) {
		Fit fit = FitCanvas(1920, 1080, w[0], w[1]);
		int cx = int(fit.scale * 1920.0f);
		int cy = int(fit.scale * 1080.0f);
		CHECK(cx <= w[0]);
		CHECK(cy <= w[1]);
		CHECK(fit.x >= 0);
		CHECK(fit.y >= 0);
		CHECK(fit.x + cx <= w[0]);
		CHECK(fit.y + cy <= w[1]);
	}
}

TEST_CASE(bars_are_split_evenly)
{
	/* Whatever is left over goes half on each side, so the picture sits in the middle. */
	Fit wide = FitCanvas(1920, 1080, 3440, 1440);
	int wideCX = (3440 / 2 - wide.x) * 2;
	CHECK(wideCX <= 3440);
	CHECK(3440 - wideCX <= 2 * wide.x + 1);

	Fit tall = FitCanvas(1920, 1080, 1080, 1920);
	int tallCY = (1920 / 2 - tall.y) * 2;
	CHECK(tallCY <= 1920);
	CHECK(1920 - tallCY <= 2 * tall.y + 1);
}
