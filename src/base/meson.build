# SPDX-License-Identifier: GPL-3.0-only

# Configuration file

config = configuration_data()
config.set_quoted('GETTEXT_PACKAGE', gv_name_lowercase)
config.set_quoted('GV_LOCALEDIR', localedir)
config.set_quoted('PACKAGE_NAME', gv_name_lowercase)
config.set_quoted('PACKAGE_VERSION', gv_version)
config.set_quoted('GV_NAME_CAPITAL', gv_name_camelcase)
config.set_quoted('GV_APPLICATION_ID', gv_application_id)
config.set_quoted('GV_APPLICATION_PATH', gv_application_path)
config.set_quoted('GV_ICON_NAME', gv_icon_name)
config.set_quoted('GV_HOMEPAGE', gv_homepage)
config.set_quoted('GV_ONLINE_HELP', gv_online_help)
config.set_quoted('GV_COPYRIGHT', gv_copyright)
config.set_quoted('GV_AUTHOR_NAME', gv_author_name)
config.set_quoted('GV_AUTHOR_EMAIL', gv_author_email)

config.set('GV_FEAT_CONSOLE_OUTPUT', gv_feat_console_output)
config.set('GV_FEAT_DBUS_SERVER', gv_feat_dbus_server)
config.set('GV_UI_ENABLED', gv_ui_enabled)
config.set('GV_FEAT_HOTKEYS', gv_feat_hotkeys)
config.set('GV_FEAT_INHIBITOR', gv_feat_inhibitor)
config.set('GV_FEAT_NOTIFICATIONS', gv_feat_notifications)

configure_file(
  output: 'config.h',
  configuration: config,
)

# Sources and dependencies

base_sources = [
  'glib-additions.c',
  'glib-object-additions.c',
  'gv-configurable.c',
  'gv-errorable.c',
  'gv-feature.c',
  'gv-base.c',
  'log.c',
  'uri-schemes.c',
  'utils.c',
]

base_dependencies = [
  glib_dep,
  gobject_dep,
  gio_dep,
]

base_enum_headers = [ 'gv-feature.h' ]
base_enums = gnome.mkenums_simple('gv-base-enum-types',
  sources: base_enum_headers
)
base_enum_h = base_enums[1]

# Library definition

gvbase = static_library('gvbase',
  sources: [ base_sources, base_enums ],
  dependencies: base_dependencies,
  include_directories: root_inc,
)

gvbase_dep = declare_dependency(
  dependencies: base_dependencies,
  sources: [ base_enum_h ],
  link_with: gvbase,
)
