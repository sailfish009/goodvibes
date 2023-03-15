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

/*
 * This header contains definitions to be used by core users
 */

#pragma once

#include <glib.h>
#include <gio/gio.h>
#include <libsoup/soup.h>

#include "core/gv-metadata.h"
#include "core/gv-player.h"
#include "core/gv-station.h"
#include "core/gv-station-list.h"
#include "core/gv-streaminfo.h"

/* Macros */

#define GV_CORE_GLIB_VERSION_STRING "GLib " \
	G_STRINGIFY(GLIB_MAJOR_VERSION) "." \
	G_STRINGIFY(GLIB_MINOR_VERSION) "." \
	G_STRINGIFY(GLIB_MICRO_VERSION)

#define GV_CORE_SOUP_VERSION_STRING "Soup " \
	G_STRINGIFY(SOUP_MAJOR_VERSION) "." \
	G_STRINGIFY(SOUP_MINOR_VERSION) "." \
	G_STRINGIFY(SOUP_MICRO_VERSION)

/* Global variables */

extern GApplication  *gv_core_application;

extern GvPlayer      *gv_core_player;
extern GvStationList *gv_core_station_list;

/* Functions */

void gv_core_init     (GApplication *app, const gchar *default_stations);
void gv_core_cleanup  (void);
void gv_core_configure(void);

void gv_core_quit     (void);

const gchar *gv_core_glib_version_string(void);
const gchar* gv_core_soup_version_string(void);

/*
 * Underlying audio backend
 */

GOptionGroup *gv_core_audio_backend_init_get_option_group (void);
void          gv_core_audio_backend_cleanup               (void);
const gchar  *gv_core_audio_backend_runtime_version_string(void);
const gchar  *gv_core_audio_backend_compile_version_string(void);
