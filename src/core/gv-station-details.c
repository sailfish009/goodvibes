/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2017 Arnaud Rebillout
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

#include <glib.h>

#include "core/gv-station-details.h"

G_DEFINE_BOXED_TYPE(GvStationDetails, gv_station_details,
                    (GBoxedCopyFunc) gv_station_details_copy,
                    (GBoxedFreeFunc) gv_station_details_free);

GvStationDetails *
gv_station_details_copy(GvStationDetails *details)
{
	GvStationDetails *new_details = g_new0(GvStationDetails, 1);

	new_details->id       = g_strdup(details->id);
	new_details->name     = g_strdup(details->name);
	new_details->homepage = g_strdup(details->homepage);
	new_details->tags     = g_strdup(details->tags);
	new_details->country  = g_strdup(details->country);
	new_details->state    = g_strdup(details->state);
	new_details->language = g_strdup(details->language);

	return new_details;
}

void
gv_station_details_free(GvStationDetails *details)
{
	if (details == NULL)
		return;

	g_free(details->id);
	g_free(details->name);
	g_free(details->homepage);
	g_free(details->tags);
	g_free(details->country);
	g_free(details->state);
	g_free(details->language);

	g_free(details);
}

GvStationDetails *
gv_station_details_new(void)
{
	GvStationDetails *details;

	details = g_new0(GvStationDetails, 1);

	return details;
}

void
gv_station_details_list_free(GList *list)
{
	g_list_free_full(list, (GDestroyNotify) gv_station_details_free);
}
