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

#include "central-swap.hpp"

#include <QMainWindow>
#include <QWidget>

bool CentralSwap::Engage(QMainWindow *main, QWidget *replacement)
{
	if (!main || !replacement || Engaged())
		return false;
	if (main->centralWidget() == replacement)
		return false;

	QWidget *current = main->takeCentralWidget(); /* ownership comes to us; kept alive, hidden */
	if (!current)
		return false;

	stock = current;
	current->setParent(main);
	current->hide();
	main->setCentralWidget(replacement);
	replacement->show();
	return true;
}

bool CentralSwap::Release(QMainWindow *main)
{
	if (!main || !Engaged())
		return false;

	QWidget *ours = main->takeCentralWidget();
	if (ours) {
		ours->setParent(main);
		ours->hide();
	}
	main->setCentralWidget(stock);
	stock->show();
	stock = nullptr;
	return true;
}
