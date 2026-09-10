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

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>

#include <QAction>
#include <QMainWindow>
#include <QMetaObject>

#include "borderless-preview.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

MODULE_EXPORT const char *obs_module_name(void)
{
	return "Borderless Preview";
}

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Replaces the main preview, in place, with an edge-to-edge view. Toggle from the Tools menu.";
}

static BorderlessDisplay *central_view = nullptr;
static obs_hotkey_id toggle_hotkey = OBS_INVALID_HOTKEY_ID;

/* Hotkey callbacks arrive off the UI thread; hop over before touching Qt. */
static void toggle_hotkey_cb(void *, obs_hotkey_id, obs_hotkey_t *, bool pressed)
{
	if (!pressed)
		return;
	QMainWindow *main = static_cast<QMainWindow *>(obs_frontend_get_main_window());
	if (!main)
		return;
	QMetaObject::invokeMethod(main, []() { BorderlessDisplay::ToggleReplacing(); }, Qt::QueuedConnection);
}

static void frontend_event(enum obs_frontend_event event, void *)
{
	switch (event) {
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
		BorderlessDisplay::OnFrontendReady();
		break;
	case OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED:
		BorderlessDisplay::OnStudioMode(true);
		break;
	case OBS_FRONTEND_EVENT_STUDIO_MODE_DISABLED:
		BorderlessDisplay::OnStudioMode(false);
		break;
	case OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED:
	case OBS_FRONTEND_EVENT_SCENE_CHANGED:
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:
		BorderlessDisplay::OnSceneChanged();
		break;
	case OBS_FRONTEND_EVENT_SCRIPTING_SHUTDOWN:
	case OBS_FRONTEND_EVENT_EXIT:
		BorderlessDisplay::OnExit();
		break;
	default:
		break;
	}
}

bool obs_module_load(void)
{
	QMainWindow *main = static_cast<QMainWindow *>(obs_frontend_get_main_window());
	if (!main) {
		obs_log(LOG_WARNING, "no main window; not loading UI");
		return true;
	}

	/* the view that replaces the stock preview in place */
	central_view = new BorderlessDisplay(main);
	central_view->hide();
	BorderlessDisplay::SetCentralInstance(central_view);

	/* Tools menu: checkable "Borderless preview" for quick switching */
	QAction *action = static_cast<QAction *>(
		obs_frontend_add_tools_menu_qaction(obs_module_text("BorderlessPreview.Toggle")));
	if (action) {
		action->setCheckable(true);
		QObject::connect(action, &QAction::toggled, [](bool on) { BorderlessDisplay::SetReplacing(on); });
		BorderlessDisplay::SetToolsAction(action);
	}

	toggle_hotkey = obs_hotkey_register_frontend(
		"BorderlessPreview.Toggle", obs_module_text("BorderlessPreview.Hotkey"), toggle_hotkey_cb, nullptr);

	obs_frontend_add_event_callback(frontend_event, nullptr);

	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_frontend_remove_event_callback(frontend_event, nullptr);
	if (toggle_hotkey != OBS_INVALID_HOTKEY_ID)
		obs_hotkey_unregister(toggle_hotkey);
	central_view = nullptr;
	obs_log(LOG_INFO, "plugin unloaded");
}
