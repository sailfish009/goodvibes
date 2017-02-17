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

#ifndef __GOODVIBES_UI_GV_MAIN_WINDOW_MANAGER_H__
#define __GOODVIBES_UI_GV_MAIN_WINDOW_MANAGER_H__

#include <glib-object.h>
#include <gtk/gtk.h>

#include "ui/gv-main-window.h"

/* GObject declarations */

#define GV_TYPE_MAIN_WINDOW_MANAGER gv_main_window_manager_get_type()

G_DECLARE_FINAL_TYPE(GvMainWindowManager, gv_main_window_manager,
                     GV, MAIN_WINDOW_MANAGER, GObject)

/* Methods */

GvMainWindowManager *gv_main_window_manager_new(GvMainWindow *main_window,
                                                gboolean status_icon_mode);

/* Property accessors */

gboolean gv_main_window_manager_get_autoset_height(GvMainWindowManager *self);
void     gv_main_window_manager_set_autoset_height(GvMainWindowManager *self,
                                                   gboolean autoset_height);

#endif /* __GOODVIBES_UI_GV_MAIN_WINDOW_MANAGER_H__ */
