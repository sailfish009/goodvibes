# SPDX-License-Identifier: GPL-3.0-only

# Subdirectories

subdir('resources')

# Sources and dependencies

ui_sources = [
  'gv-about-dialog.c',
  'gv-keyboard-shortcuts-window.c',
  'gv-main-window.c',
  'gv-main-window-manager.c',
  'gv-main-window-standalone.c',
  'gv-main-window-status-icon.c',
  'gv-playlist-view.c',
  'gv-prefs-window.c',
  'gv-station-context-menu.c',
  'gv-station-dialog.c',
  'gv-station-view.c',
  'gv-stations-tree-view.c',
  'gv-status-icon.c',
  'gv-ui.c',
  'gv-ui-helpers.c',
]

ui_dependencies = [
  glib_dep,
  gobject_dep,
  gio_dep,
  gtk_dep,
  gvbase_dep,
  gvcore_dep,
]

ui_enum_headers = [
  'gv-main-window.h',
  'gv-main-window-standalone.h',
  'gv-status-icon.h',
]
ui_enums = gnome.mkenums_simple('gv-ui-enum-types',
  sources: ui_enum_headers
)
ui_enum_h = ui_enums[1]

# Library definition

gvui = static_library('gvui',
  sources: [ ui_enums, ui_resources, ui_sources ],
  dependencies: ui_dependencies,
  include_directories: root_inc,
)

gvui_dep = declare_dependency(
  dependencies: ui_dependencies,
  sources: [ ui_enum_h ],
  link_with: gvui,
)
