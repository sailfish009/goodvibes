/*
 * Libcaphe
 *
 * Copyright (C) 2016-2017 Arnaud Rebillout
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

#include <glib-object.h>

#include "caphe-cup.h"

void
caphe_cleanup(void)
{
	g_object_add_weak_pointer(G_OBJECT(caphe_cup_default_instance),
	                          (gpointer *) &caphe_cup_default_instance);
	g_object_unref(caphe_cup_default_instance);
	g_assert_null(caphe_cup_default_instance);
}

void
caphe_init(void)
{
	/* Dummy */
}
