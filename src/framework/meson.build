framework_sources = [
  'glib-additions.c',
  'glib-object-additions.c',
  'gst-additions.c',
  'gv-configurable.c',
  'gv-errorable.c',
  'gv-feature.c',
  'gv-file-helpers.c',
  'gv-framework.c',
  'log.c',
  'uri-schemes.c',
]

framework_dependencies = [
  glib_dep,
  gobject_dep,
  gio_dep,
  gst_dep,
]

if get_option('ui')
  framework_sources += 'gtk-additions.c'
  framework_dependencies += gtk_dep
endif

framework_enum_headers = [ 'gv-feature.h' ]
framework_enums = gnome.mkenums_simple('gv-framework-enum-types',
  sources: framework_enum_headers
)
framework_enum_h = framework_enums[1]

gvframework = static_library('gvframework',
  sources: [ framework_sources, framework_enums ],
  dependencies: framework_dependencies,
  include_directories: root_inc,
)

gvframework_dep = declare_dependency(
  dependencies: framework_dependencies,
  sources: [ framework_enum_h ],
  link_with: gvframework,
)
