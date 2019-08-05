# SPDX-License-Identifier: GPL-3.0-or-later

# Configuration file

config = configuration_data()
config.set_quoted('GETTEXT_PACKAGE', gv_name_lowercase)
config.set_quoted('GV_LOCALEDIR', localedir)
config.set_quoted('PACKAGE_NAME', gv_name_lowercase)
config.set_quoted('PACKAGE_VERSION', gv_version)
config.set_quoted('GV_NAME_CAPITAL', gv_name_camelcase)
config.set_quoted('GV_APPLICATION_ID', gv_application_id)
config.set_quoted('GV_APPLICATION_PATH', gv_application_path)
config.set_quoted('GV_OLD_APPLICATION_ID', gv_old_application_id)
config.set_quoted('GV_ICON_NAME', gv_icon_name)
config.set_quoted('GV_HOMEPAGE', gv_homepage)
config.set_quoted('GV_COPYRIGHT', gv_copyright)
config.set_quoted('GV_AUTHOR_NAME', gv_author_name)
config.set_quoted('GV_AUTHOR_EMAIL', gv_author_email)

config.set('GV_FEAT_CONSOLE_OUTPUT', gv_feat_console_output)
config.set('GV_FEAT_DBUS_SERVER', gv_feat_dbus_server)
config.set('GV_FEAT_INHIBITOR', gv_feat_inhibitor)
config.set('GV_UI_ENABLED', gv_ui_enabled)
config.set('GV_FEAT_HOTKEYS', gv_feat_hotkeys)
config.set('GV_FEAT_NOTIFICATIONS', gv_feat_notifications)

configure_file(
  output: 'config.h',
  configuration: config,
)

# Sources and dependencies

framework_sources = [
  'glib-additions.c',
  'glib-object-additions.c',
  'gv-configurable.c',
  'gv-errorable.c',
  'gv-feature.c',
  'gv-framework.c',
  'log.c',
  'uri-schemes.c',
  'utils.c',
]

framework_dependencies = [
  glib_dep,
  gobject_dep,
  gio_dep,
]

framework_enum_headers = [ 'gv-feature.h' ]
framework_enums = gnome.mkenums_simple('gv-framework-enum-types',
  sources: framework_enum_headers
)
framework_enum_h = framework_enums[1]

# Library definition

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
