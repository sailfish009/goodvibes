/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2020 Arnaud Rebillout
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

#include <gtk/gtk.h>

#include "base/gv-base.h"

static const gchar *authors[] = {
	GV_AUTHOR_NAME " <" GV_AUTHOR_EMAIL ">",
	NULL
};

static const gchar *artists[] = {
	"Lahminèwski Lab https://lahminewski-lab.net",
	NULL
};

static const gchar *translators =
	"Weblate https://hosted.weblate.org/projects/goodvibes/translations\n" \
	"Adolfo Jayme Barrientos <fitojb@ubuntu.com> - Catalan (ca)\n" \
	"Lukáš Linhart <lukighostmail@gmail.com> - Czech (cs)\n" \
	"Michal Čihař <michal@cihar.com> - Czech (cs)\n" \
	"Andreas Kleinert <Andy.Kleinert@gmail.com> - German (de)\n" \
	"Vinz <vinz@vinzv.de> - German (de)\n" \
	"Daniel Bageac <danielbageac@gmail.com> - English (United States) (en_US)\n" \
	"Adolfo Jayme Barrientos <fitojb@ubuntu.com> - Spanish (es)\n" \
	"Benages <juanjo@benages.eu> - Spanish (es)\n" \
	"Holman Calderón <halecalderon@gmail.com> - Spanish (es)\n" \
	"Étienne Deparis <etienne@depar.is> - French (fr)\n" \
	"Jeannette L <j.lavoie@net-c.ca> - French (fr)\n" \
	"Milo Ivir <mail@milotype.de> - Croatian (hr)\n" \
	"Notramo <notramo@vipmail.hu> -  (hu)\n" \
	"Silvyaamalia <silvyaamalia@student.uns.ac.id> - Indonesian (id)\n" \
	"Dallenog <dallenog@gmail.com> - Italian (it)\n" \
	"Luca De Filippo <luca.defilippo@translationcommons.org> - Italian (it)\n" \
	"Prachi Joshi <josprachi@yahoo.com> - Marathi (mr)\n" \
	"Allan Nordhøy <epost@anotheragency.no> - Norwegian Bokmål (nb_NO)\n" \
	"Heimen Stoffels <vistausss@outlook.com> - Dutch (nl)\n" \
	"Michal Biesiada <blade-14@o2.pl> - Polish (pl)\n" \
	"Fúlvio Alves <fga.fulvio@gmail.com> - Portuguese (Brazil) (pt_BR)\n" \
	"Ssantos <ssantos@web.de> - Portuguese (pt)\n" \
	"Manuela Silva <mmsrs@sky.com> - Portuguese (Portugal) (pt_PT)\n" \
	"Ssantos <ssantos@web.de> - Portuguese (Portugal) (pt_PT)\n" \
	"Алексей Выскубов <viskubov@gmail.com> - Russian (ru)\n" \
	"Иван <nkvdno@mail.ru> - Russian (ru)\n" \
	"Andrej Shadura <andrew@shadura.me> - Slovak (sk)\n" \
	"Akash Rao <akash.rao.ind@gmail.com> - Telugu (te)\n" \
	"Oğuz Ersen <oguzersen@protonmail.com> - Turkish (tr)";

void
gv_show_about_dialog(GtkWindow *parent, const gchar *audio_backend_string,
                     const gchar *ui_toolkit_string)
{
	// WISHED "license-type" shouldn't be hardcoded

	gchar *comments;

	comments = g_strdup_printf("Audio Backend: %s\n"
	                           "GUI Toolkit: %s",
	                           audio_backend_string,
	                           ui_toolkit_string);

	gtk_show_about_dialog(parent,
	                      "artists", artists,
	                      "authors", authors,
	                      "comments", comments,
	                      "copyright", GV_COPYRIGHT " " GV_AUTHOR_NAME,
	                      "license-type", GTK_LICENSE_GPL_3_0,
	                      "logo-icon-name", GV_ICON_NAME,
			      "translator-credits", translators,
	                      "version", PACKAGE_VERSION,
	                      "website", GV_HOMEPAGE,
	                      NULL);

	g_free(comments);
}
