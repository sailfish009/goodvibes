/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2021 Arnaud Rebillout
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * This header contains definitions to be used by ui users
 */

#pragma once

#include <gtk/gtk.h>

/* Macros */

#define GV_UI_GTK_VERSION_STRING "GTK " \
	GV_STRINGIFY_DEPAREN(GTK_MAJOR_VERSION) "." \
	GV_STRINGIFY_DEPAREN(GTK_MINOR_VERSION) "." \
	GV_STRINGIFY_DEPAREN(GTK_MICRO_VERSION)

#define GV_UI_VERSION_STRINGS \
	GV_UI_GTK_VERSION_STRING

/* Global variables */

extern GtkWindow *gv_ui_main_window;

/* Functions */

void gv_ui_init     (GApplication *app, gboolean status_icon_mode);
void gv_ui_cleanup  (void);
void gv_ui_configure(void);

void gv_ui_play_stop(void);
void gv_ui_mute(void);
void gv_ui_present_add_station(void);
void gv_ui_present_main       (void);
void gv_ui_present_preferences(void);
void gv_ui_present_keyboard_shortcuts(void);
void gv_ui_present_about      (void);
void gv_ui_hide               (void);

const gchar *gv_ui_gtk_version_string(void);

/* Graphical toolkit */

GOptionGroup *gv_ui_toolkit_init_get_option_group (void);
