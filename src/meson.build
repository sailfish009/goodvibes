root_inc = include_directories('.')

subdir('libcaphe')
subdir('framework')
subdir('core')
if get_option('ui')
  subdir('ui')
endif
subdir('feat')

goodvibes_sources = [
  'main.c',
  'options.c',
]

goodvibes_dependencies = [
  gvframework_dep,
  gvcore_dep,
  gvfeat_dep,
  math_dep,
]

if get_option('ui')
  goodvibes_sources += 'gv-graphical-application.c'
  goodvibes_dependencies += gvui_dep
else
  goodvibes_sources += 'gv-console-application.c'
endif

executable('goodvibes', goodvibes_sources,
  dependencies: goodvibes_dependencies,
  include_directories: root_inc,
  install: true
)

executable('goodvibes-client', [ 'client.c' ],
  dependencies: [ glib_dep, gio_dep ],
  install: true
)
