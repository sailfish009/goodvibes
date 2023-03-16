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

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gio/gio.h>
#include <glib-object.h>
#include <glib-unix.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "base/gv-base.h"
#include "core/gv-core.h"
#ifdef GV_UI_ENABLED
#include "ui/gv-ui.h"
#endif

#ifdef GV_UI_ENABLED
#include "gv-graphical-application.h"
#else
#include "gv-console-application.h"
#endif
#include "options.h"

/*
 * Informations about program
 */

#define PACKAGE_INFO \
	GV_NAME_CAPITAL " " PACKAGE_VERSION

#define PACKAGE_COPYRIGHT \
	GV_COPYRIGHT " " GV_AUTHOR_NAME " <" GV_AUTHOR_EMAIL ">"

#ifdef GV_UI_ENABLED
#define VERSION_STRINGS GV_CORE_VERSION_STRINGS ", " GV_UI_VERSION_STRINGS
#else
#define VERSION_STRINGS GV CORE_VERSION_STRINGS
#endif

static const gchar *
version_strings(void)
{
	static gchar *text;

	if (text == NULL)
		text = g_strjoin(", ",
				 gv_core_glib_version_string(),
				 gv_core_soup_version_string(),
				 gv_core_gst_version_string(),
#ifdef GV_UI_ENABLED
				 gv_ui_gtk_version_string(),
#endif
				 NULL);

	return text;
}

static const gchar *
datetime_now(void)
{
	GDateTime *now;
	static gchar *text;

	g_free(text);
	now = g_date_time_new_now_local();
	text = g_date_time_format(now, "%c");
	g_date_time_unref(now);

	return text;
}

/*
 * Main - this is where everything starts...
 */

static gboolean
sigint_handler(gpointer user_data)
{
	GApplication *application = G_APPLICATION(user_data);

	/* There's probably a '^C' written on the console line by now.
	 * Let's start a new line to keep logs clean.
	 */
	putchar('\n');

	/* Stop application */
	g_application_quit(application);

	return FALSE;
}

int
main(int argc, char *argv[])
{
	GApplication *app;
	int ret;

	/* Initialize i18n */
	setlocale(LC_ALL, NULL);
	bindtextdomain(GETTEXT_PACKAGE, GV_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	/* Set application name */
	g_set_prgname(PACKAGE_NAME);
	g_set_application_name(_("Goodvibes"));

#ifdef GV_UI_ENABLED
	/* Set application icon */
	gtk_window_set_default_icon_name(GV_ICON_NAME);
#endif

	/* Parse command-line, and run some init code at the same time */
	options_parse(&argc, &argv);

	/* We might just want to print the version and exit */
	if (options.print_version) {
		g_print("%s\n", PACKAGE_INFO);
		g_print("%s\n", PACKAGE_COPYRIGHT);
		g_print("Compiled with  : %s\n", VERSION_STRINGS);
		g_print("Running against: %s\n", version_strings());
		return EXIT_SUCCESS;
	}

	/* Run in background.
	 * This option may be used along with the 'logfile' option, therefore the
	 * arguments for daemon() must be as follow:
	 * - nochdir = 1: we MUST NOT change the working directory
	 *   (otherwise a relative path for logfile won't work)
	 * - noclose = 0: we MUST close std{in/out/err} BEFORE initializing the logs
	 *   (and let the log system re-open std{out/err} afterward if needed).
	 */
	if (options.background) {
		if (daemon(1, 0) == -1) {
			g_printerr("Failed to daemonize: %s\n", strerror(errno));
			return EXIT_FAILURE;
		}
	}

	/* Initialize log system, warm it up with a few logs */
	log_init(options.log_level, options.colorless, options.output_file);
	INFO("%s", PACKAGE_INFO);
	INFO("%s", PACKAGE_COPYRIGHT);
	INFO("Started at: %s [pid: %ld]", datetime_now(), (long) getpid());
	INFO("Compiled with  : %s", VERSION_STRINGS);
	INFO("Running against: %s", version_strings());
	INFO("Gettext locale dir: %s", GV_LOCALEDIR);

	/* Create the application */
#ifdef GV_UI_ENABLED
	app = gv_graphical_application_new(GV_APPLICATION_ID);
#else
	app = gv_console_application_new(GV_APPLICATION_ID);
#endif

	/* Quit on SIGINT */
	g_unix_signal_add(SIGINT, sigint_handler, app);

	/* Run the application */
	ret = g_application_run(app, 0, NULL);

	/* Cleanup */
	log_cleanup();
	options_cleanup();

	return ret;
}
