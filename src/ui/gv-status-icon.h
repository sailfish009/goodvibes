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

#define GV_TYPE_STATUS_ICON gv_status_icon_get_type()

G_DECLARE_FINAL_TYPE(GvStatusIcon, gv_status_icon, GV, STATUS_ICON, GObject)

/* Data types */

typedef enum {
	GV_STATUS_ICON_MIDDLE_CLICK_UNDEFINED,
	GV_STATUS_ICON_MIDDLE_CLICK_TOGGLE,
	GV_STATUS_ICON_MIDDLE_CLICK_MUTE,
} GvStatusIconMiddleClick;

typedef enum {
	GV_STATUS_ICON_SCROLL_UNDEFINED,
	GV_STATUS_ICON_SCROLL_STATION,
	GV_STATUS_ICON_SCROLL_VOLUME,
} GvStatusIconScroll;

/* Methods */

GvStatusIcon *gv_status_icon_new(GtkWindow *main_window);

/* Property accessors */

GvStatusIconMiddleClick gv_status_icon_get_middle_click_action(GvStatusIcon *self);
void                    gv_status_icon_set_middle_click_action(GvStatusIcon *self,
                                                               GvStatusIconMiddleClick action);
GvStatusIconScroll      gv_status_icon_get_scroll_action      (GvStatusIcon *self);
void                    gv_status_icon_set_scroll_action      (GvStatusIcon *self,
                                                               GvStatusIconScroll action);
