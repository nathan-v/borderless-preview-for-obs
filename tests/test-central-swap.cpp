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

#include "central-swap.hpp"

#include <QDockWidget>
#include <QLabel>
#include <QMainWindow>
#include <QPointer>

namespace {

/* A main window the way OBS hands it to the plugin: a central widget of its
 * own, plus our replacement parked as a hidden child (plugin-main does the
 * same before the frontend is ready). */
struct Fixture {
	QMainWindow main;
	QPointer<QWidget> stock;
	QPointer<QWidget> ours;
	CentralSwap swap;

	Fixture() : stock(new QLabel("stock")), ours(new QLabel("ours"))
	{
		main.setCentralWidget(stock);
		ours->setParent(&main);
		ours->hide();
		main.show();
	}
};

} // namespace

TEST_CASE(engage_puts_the_replacement_in_the_central_slot)
{
	Fixture f;
	CHECK(f.swap.Engage(&f.main, f.ours));
	CHECK(f.swap.Engaged());
	CHECK(f.main.centralWidget() == f.ours);
	CHECK(f.ours->isVisible());
}

TEST_CASE(engage_keeps_the_stock_widget_alive_and_hidden)
{
	Fixture f;
	f.swap.Engage(&f.main, f.ours);
	CHECK(!f.stock.isNull());
	CHECK(f.stock->isHidden());
	CHECK(f.stock->parent() == &f.main);
}

TEST_CASE(engage_twice_is_a_no_op)
{
	Fixture f;
	CHECK(f.swap.Engage(&f.main, f.ours));
	CHECK(!f.swap.Engage(&f.main, f.ours));
	CHECK(f.main.centralWidget() == f.ours);
	CHECK(!f.stock.isNull());
}

TEST_CASE(engage_needs_a_central_widget_to_take)
{
	QMainWindow empty;
	QWidget ours;
	CentralSwap swap;
	CHECK(!swap.Engage(&empty, &ours));
	CHECK(!swap.Engaged());
	CHECK(empty.centralWidget() == nullptr);
}

TEST_CASE(engage_rejects_null_arguments)
{
	Fixture f;
	CHECK(!f.swap.Engage(&f.main, nullptr));
	CHECK(!f.swap.Engage(nullptr, f.ours));
	CHECK(!f.swap.Engaged());
	CHECK(f.main.centralWidget() == f.stock);
}

TEST_CASE(release_puts_the_stock_widget_back)
{
	Fixture f;
	f.swap.Engage(&f.main, f.ours);
	CHECK(f.swap.Release(&f.main));
	CHECK(!f.swap.Engaged());
	CHECK(f.main.centralWidget() == f.stock);
	CHECK(f.stock->isVisible());
}

TEST_CASE(release_keeps_the_replacement_alive_and_hidden)
{
	Fixture f;
	f.swap.Engage(&f.main, f.ours);
	f.swap.Release(&f.main);
	CHECK(!f.ours.isNull());
	CHECK(f.ours->isHidden());
	CHECK(f.ours->parent() == &f.main);
}

TEST_CASE(release_when_not_engaged_is_a_no_op)
{
	Fixture f;
	CHECK(!f.swap.Release(&f.main));
	CHECK(f.main.centralWidget() == f.stock);
	CHECK(f.stock->isVisible());
}

TEST_CASE(repeated_round_trips_leave_the_window_as_it_was)
{
	Fixture f;
	for (int i = 0; i < 3; i++) {
		CHECK(f.swap.Engage(&f.main, f.ours));
		CHECK(f.swap.Release(&f.main));
	}
	CHECK(f.main.centralWidget() == f.stock);
	CHECK(f.stock->isVisible());
	CHECK(!f.ours.isNull());
	CHECK(f.ours->isHidden());
}

TEST_CASE(docks_are_untouched)
{
	Fixture f;
	QDockWidget *dock = new QDockWidget("Scenes", &f.main);
	dock->setWidget(new QLabel("scene list"));
	f.main.addDockWidget(Qt::LeftDockWidgetArea, dock);
	dock->show(); /* children added to an already visible window are not shown automatically */

	f.swap.Engage(&f.main, f.ours);
	CHECK_EQ(int(f.main.dockWidgetArea(dock)), int(Qt::LeftDockWidgetArea));
	CHECK(dock->isVisible());

	f.swap.Release(&f.main);
	CHECK_EQ(int(f.main.dockWidgetArea(dock)), int(Qt::LeftDockWidgetArea));
	CHECK(dock->isVisible());
}

TEST_CASE(release_after_the_stock_widget_was_deleted_does_nothing)
{
	Fixture f;
	f.swap.Engage(&f.main, f.ours);
	delete f.stock.data();
	CHECK(f.stock.isNull());
	CHECK(!f.swap.Engaged());
	CHECK(!f.swap.Release(&f.main));
	CHECK(f.main.centralWidget() == f.ours);
}

TEST_CASE(engage_does_not_swallow_its_own_replacement)
{
	/* If the stock widget vanished while engaged, a second Engage must not
	 * take our own widget as the "stock" one and hide it. */
	Fixture f;
	f.swap.Engage(&f.main, f.ours);
	delete f.stock.data();
	CHECK(!f.swap.Engage(&f.main, f.ours));
	CHECK(f.main.centralWidget() == f.ours);
	CHECK(f.ours->isVisible());
}
