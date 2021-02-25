/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2021 Arnaud Rebillout
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

#pragma once

#include <glib-object.h>
#include <gtk/gtk.h>

/* GObject declarations */

#define GV_TYPE_MAIN_WINDOW gv_main_window_get_type()

G_DECLARE_FINAL_TYPE(GvMainWindow, gv_main_window, GV, MAIN_WINDOW, GtkApplicationWindow)

/* Data types */

typedef enum {
	GV_MAIN_WINDOW_CLOSE_QUIT,
	GV_MAIN_WINDOW_CLOSE_CLOSE,
} GvMainWindowCloseAction;

typedef enum {
	GV_MAIN_WINDOW_THEME_DEFAULT,
	GV_MAIN_WINDOW_THEME_DARK,
	GV_MAIN_WINDOW_THEME_LIGHT,
} GvMainWindowThemeVariant;

/* Methods */

GvMainWindow *gv_main_window_new(GApplication *application, GMenuModel *primary_menu, gboolean status_icon_mode);

void gv_main_window_play_stop(GvMainWindow *self);
void gv_main_window_resize_height(GvMainWindow *self, gint height);

/* Property accessors */

GMenuModel *             gv_main_window_get_primary_menu   (GvMainWindow *self);
gint                     gv_main_window_get_natural_height (GvMainWindow *self);
GvMainWindowCloseAction  gv_main_window_get_close_action   (GvMainWindow *self);
void                     gv_main_window_set_close_action   (GvMainWindow *self,
                                                            GvMainWindowCloseAction action);
GvMainWindowThemeVariant gv_main_window_get_theme_variant  (GvMainWindow *self);
void                     gv_main_window_set_theme_variant  (GvMainWindow *self,
                                                            GvMainWindowThemeVariant variant);
