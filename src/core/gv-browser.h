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

#ifndef __GOODVIBES_CORE_GV_BROWSER_H__
#define __GOODVIBES_CORE_GV_BROWSER_H__

#include <glib-object.h>
#include <gio/gio.h>

/* GObject declarations */

#define GV_TYPE_BROWSER gv_browser_get_type()

G_DECLARE_FINAL_TYPE(GvBrowser, gv_browser, GV, BROWSER, GObject)

/* Data types */

/* Methods */

GvBrowser *gv_browser_new(void);

void gv_browser_search_async(GvBrowser *self, const gchar *station_name,
                             GAsyncReadyCallback callback, gpointer user_data);

// TODO: fix proto, no more gpointer
gpointer gv_browser_search_finish(GvBrowser *self, GAsyncResult *result, GError **error);

/* Property accessors */

#endif /* __GOODVIBES_CORE_GV_BROWSER_H__ */
