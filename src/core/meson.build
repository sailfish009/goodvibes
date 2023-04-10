# SPDX-License-Identifier: GPL-3.0-only

# Sources and dependencies

core_sources = [
  'gv-core.c',
  'gv-engine.c',
  'gv-metadata.c',
  'gv-player.c',
  'gv-playlist.c',
  'gv-station.c',
  'gv-station-list.c',
  'gv-streaminfo.c',
  'playlist-utils.c',
]

core_dependencies = [
  glib_dep,
  gobject_dep,
  gio_dep,
  gst_dep,
  gst_audio_dep,
  gst_base_dep,
  libsoup_dep,
  gvbase_dep,
]

core_enum_headers = [ 'gv-engine.h', 'gv-player.h', 'gv-station.h' ]
core_enums = gnome.mkenums_simple('gv-core-enum-types',
  sources: core_enum_headers
)
core_enum_h = core_enums[1]

# Library definition

gvcore = static_library('gvcore',
  sources: [ core_enums, core_sources ],
  dependencies: core_dependencies,
  include_directories: root_inc,
)

gvcore_dep = declare_dependency(
  dependencies: core_dependencies,
  sources: [ core_enum_h ],
  link_with: gvcore,
)

# Tests

if get_option('tests')
  subdir('tests')
endif
