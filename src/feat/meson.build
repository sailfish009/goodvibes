feat_sources = [
  'gv-feat.c'
]

feat_dependencies = [
  glib_dep,
  gobject_dep,
  gvframework_dep,
]

#
# Core features
#

if get_option('feat-console-output')
  feat_sources += 'gv-console-output.c'
endif

if get_option('feat-dbus-server')
  feat_sources += [
    'gv-dbus-server.c',
    'gv-dbus-server-native.c',
    'gv-dbus-server-mpris2.c'
  ]
endif

if get_option('feat-inhibitor')
  feat_sources += 'gv-inhibitor.c'
  feat_dependencies += libcaphe_dep
endif

#
# UI features
#

if get_option('ui')
  if get_option('feat-hotkeys')
    feat_sources += 'gv-hotkeys.c'
    feat_dependencies += keybinder_dep
  endif

  if get_option('feat-notifications')
    feat_sources += 'gv-notifications.c'
  endif
endif

#
# Dependency
#

gvfeat = static_library('gvfeat',
  sources: [ feat_sources ],
  dependencies: feat_dependencies,
  include_directories: root_inc,
)

gvfeat_dep = declare_dependency(
  dependencies: feat_dependencies,
  link_with: gvfeat,
)
