# SPDX-License-Identifier: GPL-3.0-or-later

# Sources and dependencies

feat_sources = [
  'gv-feat.c'
]

feat_dependencies = [
  glib_dep,
  gobject_dep,
  gvbase_dep,
  gvcore_dep,
]

if gv_feat_console_output
  feat_sources += 'gv-console-output.c'
endif

if gv_feat_dbus_server
  feat_sources += [
    'gv-dbus-server.c',
    'gv-dbus-server-native.c',
    'gv-dbus-server-mpris2.c'
  ]
  if gv_ui_enabled
    feat_dependencies += gvui_dep
  endif
endif

if gv_feat_inhibitor
  feat_sources += [
    'gv-inhibitor.c',
    'gv-inhibitor-impl.c',
  ]
  feat_dependencies += gvui_dep
endif

if gv_feat_hotkeys
  feat_sources += 'gv-hotkeys.c'
  feat_dependencies += keybinder_dep
endif

if gv_feat_notifications
  feat_sources += 'gv-notifications.c'
endif

# Library definition

gvfeat = static_library('gvfeat',
  sources: [ feat_sources ],
  dependencies: feat_dependencies,
  include_directories: root_inc,
)

gvfeat_dep = declare_dependency(
  dependencies: feat_dependencies,
  link_with: gvfeat,
)
