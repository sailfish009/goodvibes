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

GvStationDetails *
gv_station_details_new(void)
{
	GvStationDetails *details;

	details = g_new0(GvStationDetails, 1);

	return details;
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

void
gv_station_details_list_free(GList *list)
{
	g_list_free_full(list, (GDestroyNotify) gv_station_details_free);
}
