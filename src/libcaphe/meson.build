# SPDX-License-Identifier: GPL-3.0-or-later

caphe_sources = [
  'caphe.c',
  'caphe-cup.c',
  'caphe-dbus-inhibitor.c',
  'caphe-dbus-invokator.c',
  'caphe-dbus-proxy.c',
  'caphe-inhibitor.c',
  'caphe-inhibitor-list.c',
  'caphe-login-dbus-invokator.c',
  'caphe-portal-dbus-invokator.c',
  'caphe-power-dbus-invokator.c',
  'caphe-session-dbus-invokator.c',
]

caphe_dependencies = [
  glib_dep,
  gobject_dep,
  gio_dep,
  gio_unix_dep
]

libcaphe = static_library('caphe',
  sources: [ caphe_sources ],
  dependencies: caphe_dependencies,
  c_args: [
    '-DG_LOG_DOMAIN="Caphe"',
    #'-DENABLE_TRACE',
  ],
)

libcaphe_dep = declare_dependency(
  dependencies: caphe_dependencies,
  link_with: libcaphe,
)

subdir('tests')
