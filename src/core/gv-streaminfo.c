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

#include <glib.h>
#include <glib-object.h>

#include "framework/gv-framework.h"

#include "core/gv-streaminfo.h"

/*
 * GObject definitions
 */

G_DEFINE_BOXED_TYPE(GvStreaminfo, gv_streaminfo,
		gv_streaminfo_ref, gv_streaminfo_unref);

GvStreaminfo *
gv_streaminfo_ref(GvStreaminfo *self)
{
	g_return_val_if_fail(self != NULL, NULL);
	g_return_val_if_fail(g_atomic_int_get(&self->ref_count) > 0, NULL);

	g_atomic_int_inc(&self->ref_count);

	return self;
}

void
gv_streaminfo_unref(GvStreaminfo *self)
{
	g_return_if_fail(self != NULL);
	g_return_if_fail(g_atomic_int_get(&self->ref_count) > 0);

	if (!g_atomic_int_dec_and_test(&self->ref_count))
		return;

	g_free(self->codec);
	g_free(self);
}

GvStreaminfo *
gv_streaminfo_new(void)
{
	GvStreaminfo *self;

	self = g_new0(GvStreaminfo, 1);
	self->ref_count = 1;

	return self;
}
