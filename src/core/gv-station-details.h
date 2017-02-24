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

#ifndef __GOODVIBES_CORE_GV_STATION_DETAILS_H__
#define __GOODVIBES_CORE_GV_STATION_DETAILS_H__

#include <glib.h>

/* Data types */

struct _GvStationDetails {
	gchar *id;
	gchar *name;
	gchar *homepage;
	gchar *tags;
	gchar *country;
	gchar *state;
	gchar *language;
	guint  click_count;
};

typedef struct _GvStationDetails GvStationDetails;

/* Functions */

GvStationDetails *gv_station_details_new(void);
void              gv_station_details_free(GvStationDetails *details);
void              gv_station_details_list_free(GList *list);

#endif /* __GOODVIBES_CORE_GV_STATION_DETAILS_H__ */
