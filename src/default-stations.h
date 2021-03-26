/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2020-2021 Arnaud Rebillout
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

#pragma once

/*
 * FIP <https://www.fip.fr/>
 * Just the best radios you'll ever listen to.
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
 * Nova <https://www.nova.fr/>
 * Another killer radio from France.
 */

#define DEFAULT_STATIONS_NOVA \
        "<Station>" \
        "  <name>Nova</name>" \
	"  <uri>http://novazz.ice.infomaniak.ch/novazz-128.mp3</uri>" \
        "</Station>" \
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

/*
 * More of my favorite radios.
 * - PBB <http://www2.laurentgarnier.com/PBB.html>
 * - Radio Grenouille <http://www.radiogrenouille.com/>
 */

#define DEFAULT_STATIONS_MISC \
	"<Station>" \
	"  <name>Pedro Basement Broadcast</name>" \
	"  <uri>http://pbb.laurentgarnier.com:8000/pbb128</uri> " \
	"</Station>" \
	"<Station>" \
        "  <name>Radio Grenouille</name>" \
        "  <uri>http://live.radiogrenouille.com/live</uri>" \
        "</Station>"


/*
 * SomaFM Station list
 * See https://somafm.com
 */

#define DEFAULT_STATIONS_SOMAFM \
        "<Station>" \
        "  <name>SomaFM Groove Salad</name>" \
        "  <uri>https://somafm.com/groovesalad130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Drone Zone</name>" \
        "  <uri>https://somafm.com/dronezone130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Indie Pop</name>" \
        "  <uri>https://somafm.com/indiepop130.pls</uri>" \>
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Deep Space One</name>" \
        "  <uri>https://somafm.com/deepspaceone130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Space Station</name>" \
        "  <uri>https://somafm.com/spacestation130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Lush</name>" \
        "  <uri>https://somafm.com/lush130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Secret Agent</name>" \
        "  <uri>https://somafm.com/secretagent130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Underground 80s</name>" \
        "  <uri>https://somafm.com/u80s130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM0 Groove Salad Classic</name>" \
        "  <uri>https://somafm.com/gsclassic130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Seventies</name>" \
        "  <uri>https://somafm.com/seventies130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Folk</name>" \
        "  <uri>https://somafm.com/folkfwd130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM PopTron</name>" \
        "  <uri>https://somafm.com/poptron130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Beat Blender</name>" \
        "  <uri>https://somafm.com/beatblender130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Boot Liquor</name>" \
        "  <uri>https://somafm.com/bootliquor130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM DEF CON Radio</name>" \
        "  <uri>https://somafm.com/defcon130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM The Trip</name>" \
        "  <uri>https://somafm.com/thetrip130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM BAGeL Radio</name>" \
        "  <uri>https://somafm.com/bagel130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Synphaera</name>" \
        "  <uri>https://somafm.com/synphaera130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Suburbs of Goa</name>" \
        "  <uri>https://somafm.com/suburbsofgoa130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Fluid</name>" \
        "  <uri>https://somafm.com/fluid130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Sonic Universe</name>" \
        "  <uri>https://somafm.com/sonicuniverse130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Mission Control</name>" \
        "  <uri>https://somafm.com/missioncontrol130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Seven Inch Soul</name>" \
        "  <uri>https://somafm.com/7soul130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM iIllinois Street Lounge</name>" \
        "  <uri>https://somafm.com/illstreet130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Dub Step Beyond</name>" \
        "  <uri>https://somafm.com/dubstep130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM iThistleRadio</name>" \
        "  <uri>https://somafm.com/thistle130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM ckiqhop idm</name>" \
        "  <uri>https://somafm.com/cliqhop130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM iHeavyweight Reggae</name>" \
        "  <uri>https://somafm.com/reggae130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Metal Detector</name>" \
        "  <uri>https://somafm.com/metal130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Digitalis</name>" \
        "  <uri>https://somafm.com/digitalis130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM n5MD Radio</name>" \
        "  <uri>https://somafm.com/n5md130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Vaporwaves</name>" \
        "  <uri>https://somafm.com/vaporwaves130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM SF 10-33</name>" \
        "  <uri>https://somafm.com/sf1033130.pls</uri>" \https://somafm.com
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Black Rock FM</name>" \
        "  <uri>https://somafm.com/brfm130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Covers</name>" \
        "  <uri>https://somafm.com/covers130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Live</name>" \
        "  <uri>https://somafm.com/live130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM Specials</name>" \
        "  <uri>https://somafm.com/specials130.pls</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>SomaFM SF Police Scanner</name>" \
        "  <uri>https://somafm.com/scanner130.pls</uri>" \
        "</Station>"

/*
 * Default stations
 */

#define DEFAULT_STATIONS \
        "<Stations>" \
        DEFAULT_STATIONS_FIP \
	DEFAULT_STATIONS_NOVA \
        DEFAULT_STATIONS_MISC \
	DEFAULT_STATIONS_SOMAFM \
        "</Stations>"
