/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2020-2021 Arnaud Rebillout
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

#include <gtk/gtk.h>
#include <amtk/amtk.h>

void
gv_show_keyboard_shortcuts_window(GtkWindow *parent)
{
        GtkShortcutsWindow *window;
        GtkContainer *section;
        GtkContainer *group;
	GtkWidget *shortcut;
        AmtkFactory *factory;

        factory = amtk_factory_new(NULL);
        amtk_factory_set_default_flags(factory, AMTK_FACTORY_IGNORE_GACTION);

        group = amtk_shortcuts_group_new(NULL);
	shortcut = amtk_factory_create_shortcut(factory, "app.play-stop");
	gtk_container_add(group, shortcut);
	shortcut = amtk_factory_create_shortcut(factory, "app.add-station");
	gtk_container_add(group, shortcut);
	shortcut = amtk_factory_create_shortcut(factory, "app.help");
	gtk_container_add(group, shortcut);
	shortcut = amtk_factory_create_shortcut(factory, "app.close-ui");
	gtk_container_add(group, shortcut);
	shortcut = amtk_factory_create_shortcut(factory, "app.quit");
	gtk_container_add(group, shortcut);

        g_object_unref(factory);

	section = amtk_shortcuts_section_new(NULL);
        gtk_container_add(section, GTK_WIDGET(group));

	window = amtk_shortcuts_window_new(parent);
        gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(section));
        gtk_widget_show_all(GTK_WIDGET(window));
}
