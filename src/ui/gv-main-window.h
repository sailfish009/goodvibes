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

#pragma once

#include <glib-object.h>
#include <gtk/gtk.h>

/* GObject declarations */

#define GV_TYPE_MAIN_WINDOW gv_main_window_get_type()

G_DECLARE_DERIVABLE_TYPE(GvMainWindow, gv_main_window, GV, MAIN_WINDOW, GtkApplicationWindow)

struct _GvMainWindowClass {
	/* Parent class */
	GtkApplicationWindowClass parent_class;
};

/* Data types */

typedef enum {
	GV_MAIN_WINDOW_THEME_DEFAULT,
	GV_MAIN_WINDOW_THEME_DARK,
	GV_MAIN_WINDOW_THEME_LIGHT,
} GvMainWindowThemeVariant;

/* Methods */

void gv_main_window_play_stop(GvMainWindow *self);
void gv_main_window_resize_height(GvMainWindow *self, gint height);

/* Exposed so that gv-main-widow-standalone can call it */
void gv_main_window_configure(GvMainWindow *self);

/* Property accessors */

gint                     gv_main_window_get_natural_height (GvMainWindow *self);
GvMainWindowThemeVariant gv_main_window_get_theme_variant  (GvMainWindow *self);
void                     gv_main_window_set_theme_variant  (GvMainWindow *self,
							    GvMainWindowThemeVariant variant);
