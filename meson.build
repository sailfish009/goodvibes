# SPDX-License-Identifier: GPL-3.0-or-later

project('goodvibes', 'c',
  version: '0.4.3',
  license: 'GPLv3+',
  meson_version: '>= 0.44.0',
  default_options: [
    'buildtype=debugoptimized',
    'c_args=-Wshadow',
    'c_std=gnu99',
    'warning_level=2', # -Wextra
  ]
)

# Paths

prefix = get_option('prefix')
assert(prefix.startswith('/'), 'Prefix is not absolute: "@0@"'.format(prefix))

bindir = join_paths(prefix, get_option('bindir'))
datadir = join_paths(prefix, get_option('datadir'))
localedir = join_paths(prefix, get_option('localedir'))
mandir = join_paths(prefix, get_option('mandir'))

# Modules

gnome = import('gnome')
i18n  = import('i18n')

# Project settings

gv_binary = meson.project_name()
gv_version = meson.project_version()
gv_name_lowercase = 'goodvibes'
gv_name_camelcase = 'Goodvibes'
gv_name_uppercase = 'GOODVIBES'
gv_application_id = 'io.gitlab.Goodvibes'
gv_application_path = '/io/gitlab/Goodvibes'
gv_old_application_id = 'com.elboulangero.Goodvibes'
gv_old_application_path = '/com/elboulangero/Goodvibes'
gv_icon_name = gv_application_id
gv_homepage = 'https://gitlab.com/goodvibes/goodvibes'
gv_online_help = 'https://goodvibes.readthedocs.io'
gv_copyright = 'Copyright (C) 2015-2020'
gv_author_name = 'Arnaud Rebillout'
gv_author_email = 'elboulangero@gmail.com'

# Goodvibes Core

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

gv_feat_console_output = get_option('feat-console-output')
gv_feat_dbus_server = get_option('feat-dbus-server')
gv_feat_inhibitor = get_option('feat-inhibitor')

# Goodvibes UI

gv_ui_enabled = get_option('ui-enabled')
if gv_ui_enabled
  gv_feat_hotkeys = get_option('feat-hotkeys')
  gv_feat_notifications = get_option('feat-notifications')
else
  gv_feat_hotkeys = false
  gv_feat_notifications = false
endif

if gv_ui_enabled
  gtk_req = '>= 3.12.0'
  gtk_dep = dependency('gtk+-3.0', version: gtk_req)
endif

if gv_feat_hotkeys
  keybinder_req = '>= 0.3'
  keybinder_dep = dependency('keybinder-3.0', version: keybinder_req)
endif

# Test suite

# Make tests conditional on having mutest-1 installed system-wide,
# or available as a subproject. Note that starting with meson 0.49,
# we can simplify the snippet below using 'disabler: true'. See:
# https://ebassi.github.io/mutest/mutest.md.html#/adding%C2%B5testtoyourproject

mutest_dep = dependency('mutest-1', required: false)
if not mutest_dep.found()
  mutest = subproject('mutest',
    default_options: [ 'static=true' ],
    required: false,
  )
  if mutest.found()
    mutest_dep = mutest.get_variable('mutest_dep')
  else
    mutest_dep = disabler()
  endif
endif

# Subdirectories

subdir('data')
subdir('src')
subdir('po')

# Post-install

meson.add_install_script('scripts/meson/post-install', datadir)

# Summary

summary = [
  '',
  '--------------------------------',
  '',
  '    @0@ - @1@'.format(meson.project_name(), meson.project_version()),
  '',
  '    Core',
  '      Console output: @0@'.format(gv_feat_console_output),
  '      D-Bus server  : @0@'.format(gv_feat_dbus_server),
  '      Inhibitor     : @0@'.format(gv_feat_inhibitor),
  '',
  '    UI',
  '      Enabled       : @0@'.format(gv_ui_enabled),
  '      Hotkeys       : @0@'.format(gv_feat_hotkeys),
  '      Notifications : @0@'.format(gv_feat_notifications),
  '',
  '--------------------------------',
  '',
]

message('\n'.join(summary))

# Install the git pre-commit hook

git_hook = run_command(join_paths(meson.source_root(), 'scripts/install-git-hook.sh'))
if git_hook.returncode() == 0
  message(git_hook.stdout().strip())
endif

# Targets to generate tag files

git = find_program('git', required: false)
if git.found()
  git_dir = join_paths(meson.source_root(), '.git')
  all_files = run_command(git, ['--git-dir', git_dir, 'ls-files', ':/*.[ch]'])
  all_files = files(all_files.stdout().split())

  etags_output = join_paths(meson.source_root(), 'TAGS')
  run_target('etags', command: ['env', 'etags', '-o', etags_output] + all_files)

  ctags_output = join_paths(meson.source_root(), 'tags')
  run_target('ctags', command: ['env', 'ctags', '-o', ctags_output] + all_files)
endif
