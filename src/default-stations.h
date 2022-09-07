/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2020-2021 Arnaud Rebillout
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

/*
 * FIP <https://www.fip.fr/>
 */

#define DEFAULT_STATIONS_FIP \
        "<Station>" \
        "  <name>FIP</name>" \
	"  <uri>https://stream.radiofrance.fr/fip/fip_hifi.m3u8</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>FIP Electro</name>" \
	"  <uri>https://stream.radiofrance.fr/fipelectro/fipelectro_hifi.m3u8</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>FIP Groove</name>" \
	"  <uri>https://stream.radiofrance.fr/fipgroove/fipgroove_hifi.m3u8</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>FIP Jazz</name>" \
	"  <uri>https://stream.radiofrance.fr/fipjazz/fipjazz_hifi.m3u8</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>FIP Monde</name>" \
	"  <uri>https://stream.radiofrance.fr/fipworld/fipworld_hifi.m3u8</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>FIP Nouveautés</name>" \
	"  <uri>https://stream.radiofrance.fr/fipnouveautes/fipnouveautes_hifi.m3u8</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>FIP Pop</name>" \
        "  <uri>https://stream.radiofrance.fr/fippop/fippop_hifi.m3u8</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>FIP Reggae</name>" \
	"  <uri>https://stream.radiofrance.fr/fipreggae/fipreggae_hifi.m3u8</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>FIP Rock</name>" \
	"  <uri>https://stream.radiofrance.fr/fiprock/fiprock_hifi.m3u8</uri>" \
        "</Station>"

/*
 * Radio Nova <https://www.nova.fr/>
 */

#define DEFAULT_STATIONS_NOVA_WITH_ADS \
        "<Station>" \
        "  <name>Nova</name>" \
	"  <uri>http://novazz.ice.infomaniak.ch/novazz-128.mp3</uri>" \
        "</Station>"

#define DEFAULT_STATIONS_NOVA_NO_ADS \
        "<Station>" \
        "  <name>Nova Classics</name>" \
        "  <uri>http://nova-vnt.ice.infomaniak.ch/nova-vnt-128.mp3</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>Nova Danse</name>" \
	"  <uri>http://nova-dance.ice.infomaniak.ch/nova-dance-128.mp3</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>Nova la Nuit</name>" \
	"  <uri>http://nova-ln.ice.infomaniak.ch/nova-ln-128.mp3</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>Nova Nouvo</name>" \
        "  <uri>http://nova-nouvo.ice.infomaniak.ch/nova-nouvo-128.mp3</uri>" \
        "</Station>"

#define DEFAULT_STATIONS_NOVA DEFAULT_STATIONS_NOVA_NO_ADS

/*
 * Misc
 * - Pedro Broadcasting Basement <https://www.laurentgarnier.com/radio.html>
 * - Radio Grenouille <http://www.radiogrenouille.com/>
 * - Radio Meuh <https://www.radiomeuh.com/>
 */

#define DEFAULT_STATIONS_MISC_NO_ADS \
	"<Station>" \
	"  <name>Pedro Broadcasting Basement</name>" \
	"  <uri>https://pbbradio.com:8443/pbb128</uri>" \
	"</Station>" \
	"<Station>" \
	"  <name>Radio Meuh</name>" \
	"  <uri>https://radiomeuh2.ice.infomaniak.ch/radiomeuh2-128.mp3</uri>" \
	"</Station>"

#define DEFAULT_STATIONS_MISC_WITH_ADS \
	"<Station>" \
        "  <name>Radio Grenouille</name>" \
        "  <uri>http://live.radiogrenouille.com/live</uri>" \
        "</Station>"

#define DEFAULT_STATIONS_MISC DEFAULT_STATIONS_MISC_NO_ADS

/*
 * Default stations
 */

#define DEFAULT_STATIONS \
        "<Stations>" \
        DEFAULT_STATIONS_FIP \
	DEFAULT_STATIONS_NOVA \
        DEFAULT_STATIONS_MISC \
        "</Stations>"
