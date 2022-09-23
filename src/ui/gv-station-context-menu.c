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

#include <glib-object.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "base/glib-object-additions.h"
#include "base/gv-base.h"
#include "core/gv-core.h"
#include "ui/gtk-additions.h"
#include "ui/gv-station-dialog.h"
#include "ui/gv-ui-internal.h"

#include "ui/gv-station-context-menu.h"

#define ADD_STATION_LABEL    _("Add Station")
#define EDIT_STATION_LABEL   _("Edit Station")
#define REMOVE_STATION_LABEL _("Remove Station")

/*
 * Properties
 */

enum {
	/* Reserved */
	PROP_0,
	/* Properties */
	PROP_STATION,
	/* Number of properties */
	PROP_N
};

static GParamSpec *properties[PROP_N];

/*
 * GObject definitions
 */

struct _GvStationContextMenuPrivate {
	/* Widgets */
	GtkWidget *add_station_menu_item;
	GtkWidget *edit_station_menu_item;
	GtkWidget *remove_station_menu_item;
	/* Selected station if any */
	GvStation *station;
};

typedef struct _GvStationContextMenuPrivate GvStationContextMenuPrivate;

struct _GvStationContextMenu {
	/* Parent instance structure */
	GtkMenu parent_instance;
	/* Private data */
	GvStationContextMenuPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(GvStationContextMenu, gv_station_context_menu, GTK_TYPE_MENU)

/*
 * Signal handlers & callbacks
 */

static void
on_menu_item_activate(GtkMenuItem *item, GvStationContextMenu *self)
{
	GvStationList *station_list = gv_core_station_list;
	GvStationContextMenuPrivate *priv = self->priv;
	GvStation *selected_station = priv->station;
	GtkWidget *widget = GTK_WIDGET(item);

	if (widget == priv->add_station_menu_item) {
		GvStation *station;

		station = gv_show_add_station_dialog(GTK_WINDOW(gv_ui_main_window));
		if (station) {
			if (selected_station)
				gv_station_list_insert_after(station_list, station, selected_station);
			else
				gv_station_list_append(station_list, station);
		}

	} else if (widget == priv->edit_station_menu_item && selected_station) {
		gv_show_edit_station_dialog(GTK_WINDOW(gv_ui_main_window), selected_station);

	} else if (widget == priv->remove_station_menu_item && selected_station) {
		gv_station_list_remove(station_list, selected_station);

	} else {
		CRITICAL("Unhandled menu item %p", item);
	}
}

/*
 * Private methods
 */

static void
gv_station_context_menu_populate(GvStationContextMenu *self)
{
	GvStationContextMenuPrivate *priv = self->priv;
	GtkWidget *widget;

	/* This is supposed to be called once only */
	g_assert_null(gtk_container_get_children(GTK_CONTAINER(self)));

	/* Add station */
	widget = gtk_menu_item_new_with_label(ADD_STATION_LABEL);
	gtk_menu_shell_append(GTK_MENU_SHELL(self), widget);
	g_signal_connect_object(widget, "activate", G_CALLBACK(on_menu_item_activate), self, 0);
	priv->add_station_menu_item = widget;

	/* Edit and Remove only apply if a station is selected */
	if (priv->station) {
		/* Edit station */
		widget = gtk_menu_item_new_with_label(EDIT_STATION_LABEL);
		gtk_menu_shell_append(GTK_MENU_SHELL(self), widget);
		g_signal_connect_object(widget, "activate", G_CALLBACK(on_menu_item_activate), self, 0);
		priv->edit_station_menu_item = widget;

		/* Remove station */
		widget = gtk_menu_item_new_with_label(REMOVE_STATION_LABEL);
		gtk_menu_shell_append(GTK_MENU_SHELL(self), widget);
		g_signal_connect_object(widget, "activate", G_CALLBACK(on_menu_item_activate), self, 0);
		priv->remove_station_menu_item = widget;
	}

	/* Showtime */
	gtk_widget_show_all(GTK_WIDGET(self));
}

/*
 * Property accessors
 */

static void
gv_station_context_menu_set_station(GvStationContextMenu *self, GvStation *station)
{
	GvStationContextMenuPrivate *priv = self->priv;

	/* This is a construct-only property - NULL is allowed */
	g_assert_null(priv->station);
	g_set_object(&priv->station, station);
}

static void
gv_station_context_menu_get_property(GObject *object,
				     guint property_id,
				     GValue *value,
				     GParamSpec *pspec)
{
	TRACE_GET_PROPERTY(object, property_id, value, pspec);

	G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
}

static void
gv_station_context_menu_set_property(GObject *object,
				     guint property_id,
				     const GValue *value,
				     GParamSpec *pspec)
{
	GvStationContextMenu *self = GV_STATION_CONTEXT_MENU(object);

	TRACE_SET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_STATION:
		gv_station_context_menu_set_station(self, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/*
 * Public methods
 */

GtkWidget *
gv_station_context_menu_new(void)
{
	return g_object_new(GV_TYPE_STATION_CONTEXT_MENU, NULL);
}

GtkWidget *
gv_station_context_menu_new_with_station(GvStation *station)
{
	return g_object_new(GV_TYPE_STATION_CONTEXT_MENU,
			    "station", station,
			    NULL);
}

/*
 * GObject methods
 */

static void
gv_station_context_menu_finalize(GObject *object)
{
	GvStationContextMenu *self = GV_STATION_CONTEXT_MENU(object);
	GvStationContextMenuPrivate *priv = self->priv;

	TRACE("%p", object);

	/* Unref station */
	if (priv->station)
		g_object_unref(priv->station);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_station_context_menu, object);
}

static void
gv_station_context_menu_constructed(GObject *object)
{
	GvStationContextMenu *self = GV_STATION_CONTEXT_MENU(object);

	TRACE("%p", object);

	/* Populate */
	gv_station_context_menu_populate(self);

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_station_context_menu, object);
}

static void
gv_station_context_menu_init(GvStationContextMenu *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_station_context_menu_get_instance_private(self);
}

static void
gv_station_context_menu_class_init(GvStationContextMenuClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_station_context_menu_finalize;
	object_class->constructed = gv_station_context_menu_constructed;

	/* Properties */
	object_class->get_property = gv_station_context_menu_get_property;
	object_class->set_property = gv_station_context_menu_set_property;

	properties[PROP_STATION] =
		g_param_spec_object("station", "Station", NULL, GV_TYPE_STATION,
				    GV_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_properties(object_class, PROP_N, properties);
}
