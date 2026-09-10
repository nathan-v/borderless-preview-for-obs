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

#pragma once

#include <QPointer>

class QMainWindow;
class QWidget;

/*
 * Swaps a QMainWindow's central widget for another one and back, keeping the
 * original alive and hidden in between. This is the whole mechanism behind
 * "Borderless preview": OBS's stock preview, its zoom toolbar and the Studio
 * Mode program view all live in the central widget, so lifting that one
 * widget out and dropping ours in gives the picture the entire central area.
 *
 * Nothing here touches OBS or libobs, so the unit tests exercise it against
 * a plain QMainWindow. Docks are never involved; QMainWindow keeps them
 * independent of the central slot.
 */
class CentralSwap {
public:
	/* Puts replacement in the central slot and hides the widget that was
	 * there. Returns false and changes nothing when already engaged, when
	 * replacement is null or already central, or when main has no central
	 * widget to take. */
	bool Engage(QMainWindow *main, QWidget *replacement);

	/* Puts the original widget back and hides whatever was in the slot.
	 * Returns false and changes nothing when not engaged, including when the
	 * original has since been deleted. */
	bool Release(QMainWindow *main);

	bool Engaged() const { return !stock.isNull(); }

private:
	QPointer<QWidget> stock; /* the window's own central widget while swapped out */
};
