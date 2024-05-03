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

#include <glib-object.h>

#include "glib-object-additions.h"

/*
 * Signals
 */

void
g_signal_handlers_connect_object(gpointer instance, GSignalHandler *handlers,
				 gpointer gobject, GConnectFlags connect_flags)
{
	GSignalHandler *handler;

	if (handlers == NULL)
		return;

	for (handler = handlers; handler->name; handler++)
		g_signal_connect_object(instance, handler->name, handler->callback,
					gobject, connect_flags);
}

void
g_signal_handlers_block(gpointer instance, GSignalHandler *handlers, gpointer data)
{
	GSignalHandler *handler;

	if (handlers == NULL)
		return;

	for (handler = handlers; handler->name; handler++)
		g_signal_handlers_block_by_func(instance, handler->callback, data);
}

void
g_signal_handlers_unblock(gpointer instance, GSignalHandler *handlers, gpointer data)
{
	GSignalHandler *handler;

	if (handlers == NULL)
		return;

	for (handler = handlers; handler->name; handler++)
		g_signal_handlers_unblock_by_func(instance, handler->callback, data);
}
