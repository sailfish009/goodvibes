project('goodvibes', 'c',
  version: '0.3.7',
  license: 'GPLv3+',
  default_options: [
    'buildtype=debugoptimized',
    'c_args=-Wshadow',
    'c_std=gnu99',
    'warning_level=2', # -Wextra
  ]
)

#
# Modules
#

gnome = import('gnome')
i18n  = import('i18n')

#
# Project arguments
#

gv_application_id = 'com.elboulangero.Goodvibes'
gv_localedir = join_paths(get_option('prefix'), get_option('localedir'))

goodvibes_args = [
  '-DPACKAGE_NAME="' + meson.project_name() + '"',
  '-DPACKAGE_VERSION="' + meson.project_version() + '"',
  '-DPACKAGE_CAMEL_NAME="Goodvibes"',
  '-DPACKAGE_ALLCAPS_NAME="GOODVIBES"',
  '-DPACKAGE_LONG_NAME="Goodvibes Radio Player"',
  '-DPACKAGE_APPLICATION_ID="' + gv_application_id + '"',
  '-DPACKAGE_APPLICATION_PATH="/com/elboulangero/Goodvibes"',
  '-DPACKAGE_AUTHOR_NAME="Arnaud Rebillout"',
  '-DPACKAGE_AUTHOR_EMAIL="<elboulangero@gmail.com>"',
  '-DPACKAGE_COPYRIGHT="Copyright (C) 2015-2017"',
  '-DPACKAGE_HOMEPAGE="http://goodvibes.elboulangero.com"',
  '-DGV_LOCALE_DIR="' + gv_localedir + '"',
]

#
# Goodvibes Core
#

cc = meson.get_compiler('c')
math_dep = cc.find_library('m')

glib_req      = '>= 2.44'
glib_dep      = dependency('glib-2.0', version: glib_req)
gobject_dep   = dependency('gobject-2.0', version: glib_req)
gio_dep       = dependency('gio-2.0', version: glib_req)
gio_unix_dep  = dependency('gio-unix-2.0', version: glib_req)

gst_req       = '>= 1.4.4'
gst_dep       = dependency('gstreamer-1.0', version: gst_req)
gst_base_dep  = dependency('gstreamer-base-1.0', version: gst_req)
gst_audio_dep = dependency('gstreamer-audio-1.0', version: gst_req)

libsoup_req   = '>= 2.42'
libsoup_dep   = dependency('libsoup-2.4', version: libsoup_req)

if get_option('feat-console-output')
  goodvibes_args += '-DCONSOLE_OUTPUT_ENABLED'
endif

if get_option('feat-dbus-server')
  goodvibes_args += '-DDBUS_SERVER_ENABLED'
endif

if get_option('feat-inhibitor')
  goodvibes_args += '-DINHIBITOR_ENABLED'
endif

#
# Goodvibes UI
#

if get_option('ui')
  goodvibes_args += '-DUI_ENABLED'
  gtk_req = '>= 3.12.0'
  gtk_dep = dependency('gtk+-3.0', version: gtk_req)

  if get_option('feat-hotkeys')
    goodvibes_args += '-DHOTKEYS_ENABLED'
    keybinder_req = '>= 0.3'
    keybinder_dep = dependency('keybinder-3.0', version: keybinder_req)
  endif

  if get_option('feat-notifications')
    goodvibes_args += '-DNOTIFICATIONS_ENABLED'
  endif
endif

# Finish with arguments

add_project_arguments(goodvibes_args, language: 'c')

#
# Configuration
#

# TODO config.h should live in 'framework' I guess

gv_conf = configuration_data()
gv_conf.set('PACKAGE_NAME',             meson.project_name())
gv_conf.set('PACKAGE_VERSION',          meson.project_version())
gv_conf.set('PACKAGE_CAMEL_NAME',       'Goodvibes')
gv_conf.set('PACKAGE_ALLCAPS_NAME',     'GOODVIBES')
gv_conf.set('PACKAGE_LONG_NAME',        'Goodvibes Radio Player')
gv_conf.set('PACKAGE_APPLICATION_ID',   gv_application_id)
gv_conf.set('PACKAGE_APPLICATION_PATH', '/com/elboulangero/Goodvibes')
gv_conf.set('PACKAGE_AUTHOR_NAME',      'Arnaud Rebillout')
gv_conf.set('PACKAGE_AUTHOR_EMAIL',     '<elboulangero@gmail.com>')
gv_conf.set('PACKAGE_COPYRIGHT',        'Copyright (C) 2015-2017')
gv_conf.set('PACKAGE_HOMEPAGE',         'http://goodvibes.elboulangero.com')

#
# Subdirs
#

subdir('data')
subdir('src')
subdir('po')

#
# Summary
#

summary = [
  '',
  '--------------------------------',
  '',
  '    @0@ - @1@'.format(meson.project_name(), meson.project_version()),
  '',
  '    Core',
  '      Console output: @0@'.format(get_option('feat-console-output')),
  '      D-Bus server  : @0@'.format(get_option('feat-dbus-server')),
  '      Inhibitor     : @0@'.format(get_option('feat-inhibitor')),
  '',
  '    UI',
  '      Enabled       : @0@'.format(get_option('ui')),
  '      Hotkeys       : @0@'.format(get_option('feat-hotkeys')),
  '      Notifications : @0@'.format(get_option('feat-notifications')),
  '',
  '--------------------------------',
  '',
]

message('\n'.join(summary))
