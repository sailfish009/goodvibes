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

#include <stdio.h>
#include <stdlib.h>

#include <glib-object.h>
#include <glib.h>

#include "log.h"

static gboolean initialized = FALSE;

/*
 * Global object list
 *
 * This list contains all the global objects, ie. objects which lifetime
 * last for the whole program duration. It must be populated during
 * initialization, and accessed only after.
 */

static GList *object_list;

/* Return the list of global objects. This list must be treated as read-only by
 * the caller, it's not supposed to be modified.
 */
GList *
gv_base_get_objects(void)
{
	/* This should happen only after initialization is complete */
	g_assert(initialized == TRUE);

	return object_list;
}

/* Register a global object. */
void
gv_base_register_object(gpointer data)
{
	GObject *object = G_OBJECT(data);

	/* This should happen only during initialization, never after */
	g_assert(initialized == FALSE);

	/* Add to the object list (we don't take ownership) */
	object_list = g_list_prepend(object_list, object);

	/* Add a weak pointer, for cleanup checks */
	g_object_add_weak_pointer(object, (gpointer *) &(object_list->data));
}

void
gv_base_cleanup(void)
{
	GList *item;

	g_assert(initialized == TRUE);

	/* Objects in list should be empty, thanks to the magic of weak pointers */
	for (item = object_list; item; item = item->next) {
		GObject *object = G_OBJECT(item->data);

		if (object == NULL)
			continue;

		WARNING("Object of type '%s' has not been finalized!",
			G_OBJECT_TYPE_NAME(object));
	}

	/* Free list */
	g_list_free(object_list);
}

void
gv_base_init_completed(void)
{
	g_assert(initialized == FALSE);
	initialized = TRUE;
}

void
gv_base_init(void)
{
	/* Dummy */
}
