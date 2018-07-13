/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2018 Arnaud Rebillout
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

#ifndef __GOODVIBES_UI_GV_UI_HELPERS_H__
#define __GOODVIBES_UI_GV_UI_HELPERS_H__

#include <glib-object.h>
#include <gtk/gtk.h>

/*
 * GValue transform functions
 */

void gv_value_transform_enum_string(const GValue *src_value, GValue *dest_value);
void gv_value_transform_string_enum(const GValue *src_value, GValue *dest_value);

/*
 * Gtk builder helpers
 */

void gv_builder_load(const char *filename, GtkBuilder **builder, gchar **uifile);

#endif /* __GOODVIBES_UI_GV_UI_HELPERS_H__ */
