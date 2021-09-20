/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2020-2021 Arnaud Rebillout
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

#include <gtk/gtk.h>

#include "base/gv-base.h"

#define UI_RESOURCE_PATH GV_APPLICATION_PATH "/Ui/shortcuts-window.ui"

static GtkWindow *
make_shortcuts_window(GtkWindow *parent)
{
	GtkBuilder *builder;
	GtkWindow *window;

	builder = gtk_builder_new_from_resource(UI_RESOURCE_PATH);
	window = GTK_WINDOW(gtk_builder_get_object(builder, "shortcuts-window"));
	gtk_window_set_transient_for(window, parent);
	gtk_window_set_destroy_with_parent(window, TRUE);

	g_object_ref_sink(G_OBJECT(window));
	g_object_unref(builder);

	return window;
}

void
gv_show_keyboard_shortcuts_window(GtkWindow *parent)
{
	static GtkWindow *window;

	if (window == NULL) {
		window = make_shortcuts_window(parent);
		g_object_add_weak_pointer(G_OBJECT(window), (gpointer *) &window);
	}

	gtk_window_present(window);
}
