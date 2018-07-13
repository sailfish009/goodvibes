/*
 * Libcaphe
 *
 * Copyright (C) 2016-2018 Arnaud Rebillout
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

/*
 * Compilation:
 *   packages="glib-2.0 gio-2.0 gio-unix-2.0"
 *   gcc $(pkg-config --cflags --libs $packages) *.c -o main
 *   gcc $(pkg-config --cflags --libs $packages) -DENABLE_TRACE *.c -o main
 *
 * Execution:
 *   G_MESSAGES_DEBUG=all ./main
 */

#include <stdlib.h>
#include <stdio.h>

#include <glib.h>
#include <gio/gio.h>
#include <glib-unix.h>

#include "libcaphe/caphe.h"

#define INHIBIT_REASON   "Ice-coffee with milk"

#define print(fmt, ...) g_print(fmt "\n", ##__VA_ARGS__)

static GMainLoop *main_loop;

static void
on_caphe_notify_inhibitor(CapheCup *caphe,
                          GParamSpec  *pspec G_GNUC_UNUSED,
                          gpointer user_data G_GNUC_UNUSED)
{
	CapheInhibitor *inhibitor = caphe_cup_get_inhibitor(caphe);

	if (inhibitor)
		print("Inhibited with inhibitor: %s", caphe_inhibitor_get_name(inhibitor));
	else
		print("Uninhibited");
}

static gboolean
on_stress_step_4(CapheCup *caphe)
{
	if (!caphe_cup_is_inhibited(caphe))
		g_error("Should be inhibited !");

	return G_SOURCE_REMOVE;
}

static gboolean
on_stress_step_3(CapheCup *caphe)
{
	if (!caphe_cup_is_inhibited(caphe))
		g_error("Should be inhibited !");

	print("Sending inhibit request for a different reason");
	caphe_cup_inhibit(caphe, "Expresso");

	print("Waiting 1 sec. Expect inhibited.");
	g_timeout_add_seconds(1, (GSourceFunc) on_stress_step_4, caphe);

	return G_SOURCE_REMOVE;
}

static gboolean
on_stress_step_2(CapheCup *caphe)
{
	if (caphe_cup_is_inhibited(caphe))
		g_error("Shouldn't be inhibited !");

	print("Sending batch of requests.");
	caphe_cup_uninhibit(caphe);
	caphe_cup_inhibit(caphe, INHIBIT_REASON);
	caphe_cup_inhibit(caphe, INHIBIT_REASON "dfdf");
	caphe_cup_uninhibit(caphe);
	caphe_cup_inhibit(caphe, INHIBIT_REASON);

	print("Waiting 1 sec. Expect inhibited.");
	g_timeout_add_seconds(1, (GSourceFunc) on_stress_step_3, caphe);

	return G_SOURCE_REMOVE;
}

static gboolean
when_idle_stress(CapheCup *caphe)
{
	print("Sending 1st batch of requests.");
	caphe_cup_inhibit(caphe, INHIBIT_REASON);
	caphe_cup_inhibit(caphe, INHIBIT_REASON);
	caphe_cup_uninhibit(caphe);
	caphe_cup_inhibit(caphe, INHIBIT_REASON);
	caphe_cup_uninhibit(caphe);

	print("Waiting 1 second. Expect uninhibited.");
	g_timeout_add_seconds(1, (GSourceFunc) on_stress_step_2, caphe);

	return G_SOURCE_REMOVE;
}

static gboolean
when_idle_inhibit(CapheCup *caphe)
{
	print("Inhibiting...");
	caphe_cup_inhibit(caphe, INHIBIT_REASON);

	return G_SOURCE_REMOVE;
}

static gboolean
sigint_handler(gpointer user_data G_GNUC_UNUSED)
{
	g_main_loop_quit(main_loop);

	return FALSE;
}

static void
print_usage(const char *progname)
{
	print("Usage: %s <inhibit/stress>", progname);
}

int
main(int argc, char *argv[])
{
	CapheCup *caphe;
	GSourceFunc idle_func;

	/* Process input arguments */
	if (argc == 1 || argc > 2) {
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	if (!g_strcmp0(argv[1], "inhibit"))
		idle_func = (GSourceFunc) when_idle_inhibit;
	else if (!g_strcmp0(argv[1], "stress"))
		idle_func = (GSourceFunc) when_idle_stress;
	else {
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Init */
	caphe_init();
	caphe = caphe_cup_get_default();
	g_signal_connect(caphe, "notify::inhibitor", G_CALLBACK(on_caphe_notify_inhibitor), NULL);

	/* Add idle function. Priorities to try:
	 * - G_PRIORITY_LOW
	 * - G_PRIORITY_HIGH
	 */
	g_idle_add_full(G_PRIORITY_LOW, idle_func, caphe, NULL);

	/* Be ready to catch interruptions */
	g_unix_signal_add(SIGINT, sigint_handler, NULL);

	/* Main loop */
	print("-- Running the main loop --");
	main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(main_loop);

	/* Cleanup */
	print("-- Main loop exited --");
	g_main_loop_unref(main_loop);
	caphe_cleanup();

	return EXIT_SUCCESS;
}
