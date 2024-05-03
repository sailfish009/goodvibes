/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2023-2024 Arnaud Rebillout
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

#include <glib-object.h>
#include <gtk/gtk.h>

#include "core/gv-playback.h"

/* GObject declarations */

#define GV_TYPE_CERTIFICATE_DIALOG gv_certificate_dialog_get_type()

G_DECLARE_FINAL_TYPE(GvCertificateDialog, gv_certificate_dialog, GV, CERTIFICATE_DIALOG, GObject)

/* Methods */

GvCertificateDialog *gv_make_certificate_dialog(GtkWindow *parent, GvPlayback *playback);
//GvCertificateDialog *gv_certificate_dialog_new   (void);
//void                 gv_certificate_dialog_init  (GvCertificateDialog *self, GtkWindow *parent, GvPlayback *playback);
void                 gv_certificate_dialog_update(GvCertificateDialog *self, GvPlayback *playback, GTlsCertificateFlags tls_errors);
void                 gv_certificate_dialog_show  (GvCertificateDialog *self);
