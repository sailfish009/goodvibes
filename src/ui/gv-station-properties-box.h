/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2020 Arnaud Rebillout
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

#define GV_TYPE_STATION_PROPERTIES_BOX gv_station_properties_box_get_type()

G_DECLARE_FINAL_TYPE(GvStationPropertiesBox, gv_station_properties_box, GV, STATION_PROPERTIES_BOX, GtkBox)

/* Methods */

GtkWidget *gv_station_properties_box_new(void);
