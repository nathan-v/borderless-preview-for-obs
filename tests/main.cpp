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

#ifdef BORDERLESS_TESTS_QT
#include <QApplication>
#endif

/* Usage: <test binary> [name-substring] */
int main(int argc, char **argv)
{
#ifdef BORDERLESS_TESTS_QT
	/* Widgets need a QApplication; a headless platform gives us one with no
	 * display attached, which is what CI has. "offscreen" is the quiet one,
	 * but the obs-deps Qt bundle for Windows ships only "minimal". A caller
	 * can still pick another platform through the environment. */
	if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
#ifdef Q_OS_WIN
		qputenv("QT_QPA_PLATFORM", "minimal");
#else
		qputenv("QT_QPA_PLATFORM", "offscreen");
#endif
	}
	QApplication app(argc, argv);
#endif
	return check::RunAll(argc > 1 ? argv[1] : nullptr);
}
