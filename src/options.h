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

#pragma once

struct options {
	/* Options */
	gboolean     background;
	gboolean     colorless;
	const gchar *log_level;
	const gchar *output_file;
	gboolean     print_version;
#ifdef GV_UI_ENABLED
	gboolean     without_ui;
	gboolean     status_icon;
#endif
	/* Arguments */
	const gchar *uri_to_play;
};

extern struct options options;

void options_parse(int *argc, char **argv[]);
void options_cleanup(void);
