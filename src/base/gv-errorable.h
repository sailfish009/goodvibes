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

/* GObject declarations */

#define GV_TYPE_ERRORABLE gv_errorable_get_type()

G_DECLARE_INTERFACE(GvErrorable, gv_errorable, GV, ERRORABLE, GObject)

struct _GvErrorableInterface {
	/* Parent interface */
	GTypeInterface parent_iface;
	/* Signals */
	void (*error) (GvErrorable *self);
};

/* Methods */

void gv_errorable_emit_error(GvErrorable *self, const gchar *fmt, ...);
