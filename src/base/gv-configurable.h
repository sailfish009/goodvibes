/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2024 Arnaud Rebillout
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

/* GObject declarations */

#define GV_TYPE_CONFIGURABLE gv_configurable_get_type()

G_DECLARE_INTERFACE(GvConfigurable, gv_configurable, GV, CONFIGURABLE, GObject)

struct _GvConfigurableInterface {
	/* Parent interface */
	GTypeInterface parent_iface;
	/* Virtual methods */
	void (*configure) (GvConfigurable *self);
};

/* Methods */

void gv_configurable_configure(GvConfigurable *self);
