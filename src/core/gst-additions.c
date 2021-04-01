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

#include <gst/gst.h>

/*
 * Version Information
 */

const gchar *
gst_get_runtime_version_string(void)
{
	static gchar *version_string;

	if (version_string == NULL) {
		guint major;
		guint minor;
		guint micro;
		guint nano;

		gst_version(&major, &minor, &micro, &nano);

		version_string = g_strdup_printf("GStreamer %u.%u.%u.%u",
		                                 major, minor, micro, nano);
	}

	return version_string;
}

const gchar *
gst_get_compile_version_string(void)
{
	static gchar *version_string;

	if (version_string == NULL) {
		version_string = g_strdup_printf("GStreamer %u.%u.%u.%u",
		                                 GST_VERSION_MAJOR,
		                                 GST_VERSION_MINOR,
		                                 GST_VERSION_MICRO,
		                                 GST_VERSION_NANO);
	}

	return version_string;
}
