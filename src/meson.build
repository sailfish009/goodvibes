# SPDX-License-Identifier: GPL-3.0-or-later

# Subdirectories

root_inc = include_directories('.')

subdir('libcaphe')
subdir('base')
subdir('core')
if gv_ui_enabled
  subdir('ui')
endif
subdir('feat')

# Source and dependencies

goodvibes_sources = [
  'main.c',
  'options.c',
]

goodvibes_dependencies = [
  gvbase_dep,
  gvcore_dep,
  gvfeat_dep,
  math_dep,
]

if gv_ui_enabled
  goodvibes_sources += 'gv-graphical-application.c'
  goodvibes_dependencies += gvui_dep
else
  goodvibes_sources += 'gv-console-application.c'
endif

# Executable definition

executable('goodvibes', goodvibes_sources,
  dependencies: goodvibes_dependencies,
  include_directories: root_inc,
  install: true
)

executable('goodvibes-client', [ 'client.c' ],
  dependencies: [ glib_dep, gio_dep ],
  install: true
)
