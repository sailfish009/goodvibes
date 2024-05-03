/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2017-2024 Arnaud Rebillout
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

#include <gio/gio.h>
#include <glib-object.h>
#include <glib.h>

#include "base/gv-base.h"
#include "core/gv-core.h"
#include "feat/gv-feat.h"

#include "default-stations.h"
#include "gv-console-application.h"
#include "options.h"

#if !GLIB_CHECK_VERSION(2, 74, 0)
#define G_APPLICATION_DEFAULT_FLAGS G_APPLICATION_FLAGS_NONE
#endif

/*
 * GObject definitions
 */

struct _GvConsoleApplication {
	/* Parent instance structure */
	GApplication parent_instance;
};

G_DEFINE_TYPE(GvConsoleApplication, gv_console_application, G_TYPE_APPLICATION)

/*
 * Public methods
 */

GApplication *
gv_console_application_new(const gchar *application_id)
{
	GvConsoleApplication *self;

	self = g_object_new(GV_TYPE_CONSOLE_APPLICATION,
			    "application-id", application_id,
			    "flags", G_APPLICATION_DEFAULT_FLAGS,
			    NULL);

	return G_APPLICATION(self);
}

/*
 * GApplication methods
 */

static void
gv_console_application_shutdown(GApplication *app)
{
	DEBUG_NO_CONTEXT(">>>> Main loop terminated <<<<");

	/* Cleanup */
	DEBUG_NO_CONTEXT("---- Cleaning up ----");
	gv_feat_cleanup();
	gv_core_cleanup();
	gv_base_cleanup();

	/* Mandatory chain-up */
	G_APPLICATION_CLASS(gv_console_application_parent_class)->shutdown(app);
}

static void
gv_console_application_startup(GApplication *app)
{
	DEBUG_NO_CONTEXT("---- Starting application ----");

	/* Mandatory chain-up, see:
	 * https://developer.gnome.org/gtk3/stable/GtkApplication.html#gtk-application-new
	 */
	G_APPLICATION_CLASS(gv_console_application_parent_class)->startup(app);

	/* Initialization */
	DEBUG_NO_CONTEXT("---- Initializing ----");
	gv_base_init();
	gv_core_init(app, DEFAULT_STATIONS);
	gv_feat_init();
	gv_base_init_completed();

	/* Configuration */
	DEBUG_NO_CONTEXT("---- Configuring ----");
	gv_feat_configure_early();
	gv_core_configure();
	gv_feat_configure_late();

	/* Hold application */
	g_application_hold(app);
}

static gboolean
when_idle_go_player(gpointer user_data)
{
	const gchar *uri_to_play = user_data;

	gv_player_go(gv_core_player, uri_to_play);

	return G_SOURCE_REMOVE;
}

static void
gv_console_application_activate(GApplication *app G_GNUC_UNUSED)
{
	static gboolean first_invocation = TRUE;

	if (first_invocation) {
		first_invocation = FALSE;

		DEBUG_NO_CONTEXT(">>>> Main loop started <<<<");

		/* Schedule a callback to play music.
		 * DO NOT start playing now ! It's too early !
		 * There's still some init code pending, and we want to ensure
		 * (as much as possible) that this init code is run before we
		 * start the playback. Therefore we schedule with a low priority.
		 */
		g_idle_add_full(G_PRIORITY_LOW, when_idle_go_player,
				(void *) options.uri_to_play, NULL);
	}
}

/*
 * GObject methods
 */

static void
gv_console_application_init(GvConsoleApplication *self)
{
	TRACE("%p", self);
}

static void
gv_console_application_class_init(GvConsoleApplicationClass *class)
{
	GApplicationClass *application_class = G_APPLICATION_CLASS(class);

	TRACE("%p", class);

	/* Override GApplication methods */
	application_class->startup = gv_console_application_startup;
	application_class->shutdown = gv_console_application_shutdown;
	application_class->activate = gv_console_application_activate;
}
