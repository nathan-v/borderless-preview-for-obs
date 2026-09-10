/*
Borderless Preview for OBS
Copyright (C) 2026 Nathan V

Fit math modelled on OBS Studio's OBSBasic_Preview.cpp, Copyright (C) 2023 by
Lain Bailey, GPL v2+.

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

/*
 * Same fit as OBS's own preview, minus the 10 px edge it insists on: scale
 * the base canvas to fill the window on one axis and centre it on the other,
 * so the only bars left are the letterbox. Pure arithmetic with no OBS or Qt
 * involvement, which is why it lives in its own header; the unit tests cover
 * it directly.
 */
inline void GetScaleAndCenterPos(int baseCX, int baseCY, int windowCX, int windowCY, int &x, int &y, float &scale)
{
	double windowAspect = double(windowCX) / double(windowCY);
	double baseAspect = double(baseCX) / double(baseCY);
	int newCX, newCY;

	if (windowAspect > baseAspect) {
		scale = float(windowCY) / float(baseCY);
		newCX = int(double(windowCY) * baseAspect);
		newCY = windowCY;
	} else {
		scale = float(windowCX) / float(baseCX);
		newCX = windowCX;
		newCY = int(float(windowCX) / baseAspect);
	}

	x = windowCX / 2 - newCX / 2;
	y = windowCY / 2 - newCY / 2;
}
