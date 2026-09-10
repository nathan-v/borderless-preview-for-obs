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

#include "borderless-preview.hpp"
#include "canvas-fit.hpp"

#include <obs-module.h>
#include <graphics/graphics.h>
#include <graphics/matrix4.h>
#include <graphics/vec4.h>
#include <util/platform.h>

#if !defined(_WIN32) && !defined(__APPLE__)
#include <obs-nix-platform.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <QAction>
#include <QMainWindow>
#include <QMouseEvent>
#include <QWindow>

#include <mutex>

#define BACKGROUND_COLOR 0xFF000000 /* letterbox colour: black, so bars vanish */

std::atomic<bool> BorderlessDisplay::studioMode{false};
obs_weak_source_t *BorderlessDisplay::previewScene = nullptr;
bool BorderlessDisplay::replacing = false;
bool BorderlessDisplay::ready = false;
QPointer<QAction> BorderlessDisplay::toolsAction;
BorderlessDisplay *BorderlessDisplay::centralInstance = nullptr;
CentralSwap BorderlessDisplay::swap;

static std::mutex previewSceneMutex;

/* ------------------------------------------------------------------------- */
/* helpers                                                                   */

static QString PluginStr(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

static QMainWindow *MainWindow()
{
	return static_cast<QMainWindow *>(obs_frontend_get_main_window());
}

/* A scene with transparent areas needs something behind it (OBS draws the
 * same backdrop for its Studio Mode preview). */
static void DrawBackdrop(float cx, float cy)
{
	gs_effect_t *solid = obs_get_base_effect(OBS_EFFECT_SOLID);
	gs_eparam_t *color = gs_effect_get_param_by_name(solid, "color");
	gs_technique_t *tech = gs_effect_get_technique(solid, "Solid");

	struct vec4 black;
	vec4_set(&black, 0.0f, 0.0f, 0.0f, 1.0f);
	gs_effect_set_vec4(color, &black);

	gs_technique_begin(tech);
	gs_technique_begin_pass(tech, 0);
	gs_matrix_push();
	gs_matrix_identity();
	gs_matrix_scale3f(cx, cy, 1.0f);
	gs_render_start(false);
	gs_vertex2f(0.0f, 0.0f);
	gs_vertex2f(0.0f, 1.0f);
	gs_vertex2f(1.0f, 0.0f);
	gs_vertex2f(1.0f, 1.0f);
	gs_render_stop(GS_TRISTRIP);
	gs_matrix_pop();
	gs_technique_end_pass(tech);
	gs_technique_end(tech);
	gs_load_vertexbuffer(nullptr);
}

static bool QTToGSWindow(QWindow *window, gs_window &gswindow)
{
#ifdef _WIN32
	gswindow.hwnd = (HWND)window->winId();
	return true;
#elif __APPLE__
	gswindow.view = (id)window->winId();
	return true;
#else
	switch (obs_get_nix_platform()) {
	case OBS_NIX_PLATFORM_X11_EGL:
		gswindow.id = window->winId();
		gswindow.display = obs_get_nix_platform_display();
		return true;
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
	case OBS_NIX_PLATFORM_WAYLAND:
		gswindow.display = (void *)window->winId();
		return gswindow.display != nullptr;
#endif
	default:
		blog(LOG_WARNING, "[borderless-preview] unsupported display platform for this Qt build");
		return false;
	}
#endif
}

/* ------------------------------------------------------------------------- */
/* widget                                                                    */

BorderlessDisplay::BorderlessDisplay(QWidget *parent) : QWidget(parent)
{
	setAttribute(Qt::WA_PaintOnScreen);
	setAttribute(Qt::WA_StaticContents);
	setAttribute(Qt::WA_NoSystemBackground);
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAttribute(Qt::WA_DontCreateNativeAncestors);
	setAttribute(Qt::WA_NativeWindow);
	setMinimumSize(32, 18);
	setToolTip(PluginStr("BorderlessPreview.Tip"));

	auto windowVisible = [this](bool visible) {
		if (!visible) {
#if !defined(_WIN32) && !defined(__APPLE__)
			DestroyDisplay(); /* the surface goes away with the window on Linux */
			destroying = false;
#endif
			return;
		}
		if (!display) {
			CreateDisplay();
		} else {
			QSize s = PixelSize();
			obs_display_resize(display, s.width(), s.height());
		}
	};
	auto screenChanged = [this](QScreen *) {
		CreateDisplay();
		if (display) {
			QSize s = PixelSize();
			obs_display_resize(display, s.width(), s.height());
		}
	};
	connect(windowHandle(), &QWindow::visibleChanged, windowVisible);
	connect(windowHandle(), &QWindow::screenChanged, screenChanged);
}

BorderlessDisplay::~BorderlessDisplay()
{
	DestroyDisplay();
	if (centralInstance == this)
		centralInstance = nullptr;
}

QPaintEngine *BorderlessDisplay::paintEngine() const
{
	return nullptr;
}

QSize BorderlessDisplay::PixelSize() const
{
	qreal dpr = devicePixelRatioF();
	return QSize(int(width() * dpr), int(height() * dpr));
}

void BorderlessDisplay::CreateDisplay()
{
	if (display || destroying || !ready)
		return;
	if (!windowHandle() || !windowHandle()->isExposed())
		return;

	QSize s = PixelSize();
	gs_init_data info = {};
	info.cx = s.width();
	info.cy = s.height();
	info.format = GS_BGRA;
	info.zsformat = GS_ZS_NONE;
	if (!QTToGSWindow(windowHandle(), info.window))
		return;

	display = obs_display_create(&info, BACKGROUND_COLOR);
	if (display)
		obs_display_add_draw_callback(display, Draw, this);
}

void BorderlessDisplay::DestroyDisplay()
{
	if (display) {
		obs_display_remove_draw_callback(display, Draw, this);
		obs_display_destroy(display);
		display = nullptr;
	}
	destroying = true;
}

void BorderlessDisplay::paintEvent(QPaintEvent *event)
{
	CreateDisplay();
	QWidget::paintEvent(event);
}

void BorderlessDisplay::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
	CreateDisplay();
	if (isVisible() && display) {
		QSize s = PixelSize();
		obs_display_resize(display, s.width(), s.height());
	}
}

void BorderlessDisplay::moveEvent(QMoveEvent *event)
{
	QWidget::moveEvent(event);
	if (display)
		obs_display_update_color_space(display);
}

bool BorderlessDisplay::nativeEvent(const QByteArray &, void *message, qintptr *)
{
#ifdef _WIN32
	const MSG &msg = *static_cast<MSG *>(message);
	if (msg.message == WM_DISPLAYCHANGE && display)
		obs_display_update_color_space(display);
#else
	UNUSED_PARAMETER(message);
#endif
	return false;
}

void BorderlessDisplay::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
		ToggleReplacing();
	QWidget::mouseDoubleClickEvent(event);
}

/* ------------------------------------------------------------------------- */
/* rendering (graphics thread)                                               */

void BorderlessDisplay::Draw(void *data, uint32_t cx, uint32_t cy)
{
	BorderlessDisplay *d = static_cast<BorderlessDisplay *>(data);
	if (!cx || !cy)
		return;

	obs_video_info ovi;
	if (!obs_get_video_info(&ovi) || !ovi.base_width || !ovi.base_height)
		return;

	int x, y;
	float scale;
	GetScaleAndCenterPos(int(ovi.base_width), int(ovi.base_height), int(cx), int(cy), x, y, scale);
	int newCX = int(scale * float(ovi.base_width));
	int newCY = int(scale * float(ovi.base_height));

	gs_viewport_push();
	gs_projection_push();
	gs_ortho(0.0f, float(ovi.base_width), 0.0f, float(ovi.base_height), -100.0f, 100.0f);
	gs_set_viewport(x, y, newCX, newCY);

	UNUSED_PARAMETER(d);
	bool showPreview = studioMode.load();
	if (showPreview) {
		DrawBackdrop(float(ovi.base_width), float(ovi.base_height));
		obs_source_t *scene = nullptr;
		{
			std::lock_guard<std::mutex> lock(previewSceneMutex);
			if (previewScene)
				scene = obs_weak_source_get_source(previewScene);
		}
		if (scene) {
			obs_source_video_render(scene);
			obs_source_release(scene);
		}
	} else {
		obs_render_main_texture_src_color_only();
	}
	gs_load_vertexbuffer(nullptr);

	gs_projection_pop();
	gs_viewport_pop();
}

/* ------------------------------------------------------------------------- */
/* frontend state (UI thread)                                                */

void BorderlessDisplay::OnStudioMode(bool enabled)
{
	studioMode.store(enabled);
	OnSceneChanged();
}

void BorderlessDisplay::OnSceneChanged()
{
	obs_source_t *scene = obs_frontend_get_current_preview_scene();
	obs_weak_source_t *weak = scene ? obs_source_get_weak_source(scene) : nullptr;
	{
		std::lock_guard<std::mutex> lock(previewSceneMutex);
		obs_weak_source_release(previewScene);
		previewScene = weak;
	}
	obs_source_release(scene);
}

void BorderlessDisplay::OnFrontendReady()
{
	ready = true;
	LoadSettings();
	studioMode.store(obs_frontend_preview_program_mode_active());
	OnSceneChanged();
	SetToolsAction(toolsAction);
	ApplyReplacement();
}

void BorderlessDisplay::OnExit()
{
	/* Put OBS's own preview back before it saves its window state and tears
	 * libobs down. The Qt widgets themselves are deleted later by OBS. */
	bool wanted = replacing;
	replacing = false;
	ApplyReplacement();
	replacing = wanted; /* keep the saved preference for next launch */
	ready = false;
	{
		std::lock_guard<std::mutex> lock(previewSceneMutex);
		obs_weak_source_release(previewScene);
		previewScene = nullptr;
	}
}

/* ------------------------------------------------------------------------- */
/* in-place replacement of the stock preview                                 */

void BorderlessDisplay::SetCentralInstance(BorderlessDisplay *instance)
{
	centralInstance = instance;
}

void BorderlessDisplay::SetToolsAction(QAction *action)
{
	toolsAction = action;
	if (toolsAction) {
		toolsAction->blockSignals(true);
		toolsAction->setChecked(replacing);
		toolsAction->blockSignals(false);
	}
}

bool BorderlessDisplay::Replacing()
{
	return replacing;
}

void BorderlessDisplay::SetReplacing(bool on)
{
	if (replacing != on) {
		replacing = on;
		SaveSettings();
	}
	if (toolsAction && toolsAction->isChecked() != on) {
		toolsAction->blockSignals(true);
		toolsAction->setChecked(on);
		toolsAction->blockSignals(false);
	}
	ApplyReplacement();
}

void BorderlessDisplay::ToggleReplacing()
{
	SetReplacing(!replacing);
}

/* OBS's preview, its zoom toolbar and the Studio Mode program view all live
 * in the main window's central widget. Swap that widget for ours and the
 * picture takes the whole central area; swap back and everything is exactly
 * as it was. Dock layout is untouched either way. The swap itself is in
 * CentralSwap so it can be tested without OBS. */
void BorderlessDisplay::ApplyReplacement()
{
	QMainWindow *main = MainWindow();
	if (!main || !centralInstance)
		return;

	if (ready && replacing)
		swap.Engage(main, centralInstance);
	else
		swap.Release(main);
}

/* ------------------------------------------------------------------------- */
/* settings                                                                  */

void BorderlessDisplay::LoadSettings()
{
	char *path = obs_module_config_path("config.json");
	if (!path)
		return;
	obs_data_t *data = obs_data_create_from_json_file(path);
	if (data) {
		replacing = obs_data_get_bool(data, "replace_main_preview");
		obs_data_release(data);
	}
	bfree(path);
}

void BorderlessDisplay::SaveSettings()
{
	char *dir = obs_module_config_path(nullptr);
	if (dir) {
		os_mkdirs(dir);
		bfree(dir);
	}
	char *path = obs_module_config_path("config.json");
	if (!path)
		return;
	obs_data_t *data = obs_data_create();
	obs_data_set_bool(data, "replace_main_preview", replacing);
	obs_data_save_json_safe(data, path, "tmp", "bak");
	obs_data_release(data);
	bfree(path);
}
