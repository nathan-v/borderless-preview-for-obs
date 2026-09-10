/*
Borderless Preview for OBS
Copyright (C) 2026 Nathan V

Display surface modelled on OBS Studio's OBSQTDisplay and the preview render
path in OBSBasic_Preview.cpp, Copyright (C) 2023 by Lain Bailey, GPL v2+.

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

#include "central-swap.hpp"

#include <obs.h>
#include <obs-frontend-api.h>

#include <QPointer>
#include <QWidget>

#include <atomic>

class QAction;

/*
 * Draws the OBS canvas with no edge, no label row and no zoom toolbar: the
 * picture fills the widget right up to the letterbox. Shows the preview
 * scene while Studio Mode is on, program otherwise.
 *
 * Lives in place of OBS's own preview: when "Borderless preview" is on
 * (Tools menu, hotkey, or a double-click on the view) the stock preview
 * area is lifted out of the main window's central slot and this widget
 * goes in; toggling it off swaps them back. Docks never move.
 *
 * View-only: selecting and moving sources still happens in the stock
 * preview, which is one toggle away.
 */
class BorderlessDisplay : public QWidget {
	Q_OBJECT

public:
	explicit BorderlessDisplay(QWidget *parent = nullptr);
	~BorderlessDisplay() override;

	QPaintEngine *paintEngine() const override;

	/* called from the frontend event callback in plugin-main */
	static void OnStudioMode(bool enabled);
	static void OnSceneChanged();
	static void OnFrontendReady();
	static void OnExit();

	/* the in-place replacement of the stock preview */
	static void SetCentralInstance(BorderlessDisplay *instance);
	static void SetToolsAction(QAction *action);
	static bool Replacing();
	static void SetReplacing(bool on);
	static void ToggleReplacing();

protected:
	void paintEvent(QPaintEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void moveEvent(QMoveEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

private:
	void CreateDisplay();
	void DestroyDisplay();
	QSize PixelSize() const;

	static void Draw(void *data, uint32_t cx, uint32_t cy);
	static void ApplyReplacement();
	static void LoadSettings();
	static void SaveSettings();

	obs_display_t *display = nullptr;
	bool destroying = false;

	static std::atomic<bool> studioMode;
	static obs_weak_source_t *previewScene; /* updated on the UI thread, read in Draw */
	static bool replacing;
	static bool ready;
	static QPointer<QAction> toolsAction;
	static BorderlessDisplay *centralInstance; /* the widget that takes the preview's slot */
	static CentralSwap swap;                   /* holds OBS's own central widget while swapped out */
};
