# TODO make distinction between package_name and package_binary
#      in the code, if needed.
# TODO rename these ugly ALLCAPS_NAME and so on
# TODO can we handle the uppercasing with a meson function?
# TODO check if some of these variables are unused, by any chance?

locale_dir = join_paths(get_option('prefix'), get_option('localedir'))

config = configuration_data()
config.set_quoted('GETTEXT_PACKAGE', gv_name_lowercase)
config.set_quoted('LOCALE_DIR', locale_dir)
config.set_quoted('PACKAGE_NAME', gv_name_lowercase)
config.set_quoted('PACKAGE_VERSION', gv_version)
config.set_quoted('PACKAGE_CAMEL_NAME', gv_name_camelcase)
config.set_quoted('PACKAGE_ALLCAPS_NAME', gv_name_uppercase)
config.set_quoted('PACKAGE_APPLICATION_ID', gv_application_id)
config.set_quoted('PACKAGE_APPLICATION_PATH', gv_application_path)
config.set_quoted('PACKAGE_HOMEPAGE', gv_homepage)
config.set_quoted('PACKAGE_COPYRIGHT', gv_copyright)
config.set_quoted('PACKAGE_AUTHOR_NAME', gv_author_name)
config.set_quoted('PACKAGE_AUTHOR_EMAIL', gv_author_email)

# TODO prefix feature defines with FEAT, remove ENABLED
config.set('CONSOLE_OUTOUT_ENABLED', gv_feat_console_output)
config.set('DBUS_SERVER_ENABLED', gv_feat_dbus_server)
config.set('INHIBITOR_ENABLED', gv_feat_inhibitor)
config.set('UI_ENABLED', gv_ui_enabled)
config.set('HOTKEYS_ENABLED', gv_feat_hotkeys)
config.set('NOTIFICATIONS_ENABLED', gv_feat_notifications)

configure_file(
  output: 'config.h',
  configuration: config,
)

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
