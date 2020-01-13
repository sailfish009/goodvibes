/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2020 Arnaud Rebillout
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>

#include "framework/gv-framework.h"
#include "core/gv-core.h"
#include "ui/gtk-additions.h"
#include "ui/gv-about-dialog.h"
#include "ui/gv-keyboard-shortcuts-window.h"
#include "ui/gv-main-window.h"
#include "ui/gv-main-window-manager.h"
#include "ui/gv-prefs-window.h"
#include "ui/gv-station-dialog.h"
#include "ui/gv-status-icon.h"

#include "ui/resources/gv-ui-resources.h"

#define UI_SCHEMA_ID_SUFFIX "Ui"

/*
 * Public variables
 */

GSettings *gv_ui_settings;

GvStatusIcon        *gv_ui_status_icon;
GtkWidget           *gv_ui_main_window;
GvMainWindowManager *gv_ui_main_window_manager;

/*
 * Private variables
 */

static GList *ui_objects;

/*
 * Underlying graphical toolkit
 */

GOptionGroup *
gv_ui_toolkit_init_get_option_group(void)
{
	/* Very not sure about the argument to pass here. From my experience:
	 * - if we don't use gtk_init(), we should pass TRUE.
	 * - if we use gtk_init(), passing FALSE is ok.
	 * Since we use GtkApplication which calls gtk_init() internally,
	 * let's pass FALSE and pray that it works.
	 */

	return gtk_get_option_group(FALSE);
}

const gchar *
gv_ui_toolkit_runtime_version_string(void)
{
	return gtk_get_runtime_version_string();
}

const gchar *
gv_ui_toolkit_compile_version_string(void)
{
	return gtk_get_compile_version_string();
}

/*
 * Ui public functions
 */

void
gv_ui_hide(void)
{
	/* In status icon mode, do nothing */
	if (gv_ui_status_icon)
		return;

	gtk_widget_hide(gv_ui_main_window);
}

void
gv_ui_present_about(void)
{
	gv_show_about_dialog(GTK_WINDOW(gv_ui_main_window),
	                     gv_core_audio_backend_runtime_version_string(),
	                     gv_ui_toolkit_runtime_version_string());
}

void
gv_ui_present_keyboard_shortcuts(void)
{
	gv_show_keyboard_shortcuts_window(GTK_WINDOW(gv_ui_main_window));
}

void
gv_ui_present_preferences(void)
{
	GtkWindow *parent;

	parent = gv_ui_status_icon ? NULL : GTK_WINDOW(gv_ui_main_window);
	gv_show_prefs_window(parent);
}

void
gv_ui_present_main(void)
{
	/* In status icon mode, do nothing */
	if (gv_ui_status_icon)
		return;

	gtk_window_present(GTK_WINDOW(gv_ui_main_window));
}

void
gv_ui_present_add_station(void)
{
	GvStationList *station_list = gv_core_station_list;
	GvStation *station;

	station = gv_show_add_station_dialog(GTK_WINDOW(gv_ui_main_window));
	if (station)
		gv_station_list_append(station_list, station);
}

void
gv_ui_configure(void)
{
	GList *item;

	/* Configure each object that is configurable */
	for (item = ui_objects; item; item = item->next) {
		GObject *object = item->data;

		if (!GV_IS_CONFIGURABLE(object))
			continue;

		gv_configurable_configure(GV_CONFIGURABLE(object));
	}
}

void
gv_ui_cleanup(void)
{
	GList *item;

	/* Windows must be destroyed with gtk_widget_destroy().
	 * Forget about gtk_window_close() here, which seems to be asynchronous.
	 * Read the doc:
	 * https://developer.gnome.org/gtk3/stable/GtkWindow.html#gtk-window-new
	 */

	/* Destroy public ui objects */
	ui_objects = g_list_reverse(ui_objects);
	for (item = ui_objects; item; item = item->next) {
		GObject *object = item->data;

		if (GTK_IS_WINDOW(object))
			gtk_widget_destroy(GTK_WIDGET(object));
		else
			g_object_unref(object);
	}
	g_list_free(ui_objects);
}

void
gv_ui_init(GApplication *app, GMenuModel *primary_menu, gboolean status_icon_mode)
{
	GList *item;

	/* Create resource
	 *
	 * The resource object is a bit special, it's a static resource,
	 * it lives in the data segment, which is read-only. So calling
	 * g_object_ref() on it causes a segfault, for example.
	 *
	 * Actually, we don't even need to keep a reference to this object.
	 * All we want is to initialize the resource, which is triggered
	 * by the first access (lazy init).
	 */
	gv_ui_get_resource();

	/* Create ui objects */
	gv_ui_settings = gv_get_settings(UI_SCHEMA_ID_SUFFIX);
	ui_objects = g_list_append(ui_objects, gv_ui_settings);

	gv_ui_main_window = gv_main_window_new(app, primary_menu, status_icon_mode);
	ui_objects = g_list_append(ui_objects, gv_ui_main_window);

	gv_ui_main_window_manager = gv_main_window_manager_new
	                            (GV_MAIN_WINDOW(gv_ui_main_window), status_icon_mode);
	ui_objects = g_list_append(ui_objects, gv_ui_main_window_manager);

	if (status_icon_mode) {
		gv_ui_status_icon = gv_status_icon_new(GTK_WINDOW(gv_ui_main_window));
		ui_objects = g_list_append(ui_objects, gv_ui_status_icon);
	} else {
		gv_ui_status_icon = NULL;
	}

	/* Register objects in the framework */
	for (item = ui_objects; item; item = item->next) {
		GObject *object = G_OBJECT(item->data);
		gv_framework_register_object(object);
	}
}
