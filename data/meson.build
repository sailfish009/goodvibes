# SPDX-License-Identifier: GPL-3.0-or-later

# My understanding of meson (a beginner guide)
#
#     'configure step' and 'build step'
#
# *Configure files* are generated early when creating the build directory with
# 'meson 'builddir', and afterward Meson automatically update these files if
# the source changes. Therefore, my understanding is that there's no need to
# declare a dependency to ensure configure files are up-to-date: they're always
# up-to-date.
#
# However, custom_target() has a 'depend_files:' keyword for declaring
# dependencies on files. It's unclear if we need to use it for configure files.
# I'd say no need.
#
# *Build targets* are, on the other hand, part of a dependency chain, and the
# 'depends:' keyword must be used to indicate a dependency. This is mandatory.
#
#     'source dir' and 'build dir'
#
# Meson has a clear concept of 'source dir' and 'build dir'.
#
# When you give a relative path to 'input:', meson assumes it's relative to
# 'source dir'. Beware when you need to use configure files as inputs, as they
# live in 'build dir'.
#
#     References
#
# <http://mesonbuild.com/Overview.html>
#
# Credit for this file goes to <https://github.com/pithos/pithos>.

# ----------------------------------------------------- #
#         D-Bus Service File                            #
# ----------------------------------------------------- #
# <https://dbus.freedesktop.org/doc/system-activation.txt>

services_dir = join_paths(datadir, 'dbus-1/services')

service_conf = configuration_data()
service_conf.set('bindir', bindir)
service_conf.set('binary', gv_binary)
service_conf.set('dbus_name', gv_application_id)

configure_file(
  input: gv_application_id + '.service.in',
  output: gv_application_id + '.service',
  configuration: service_conf,
  install_dir: services_dir,
)

# XXX It would be nice to have a tool to validate the service file.

# ----------------------------------------------------- #
#         Desktop File                                  #
# ----------------------------------------------------- #
# <https://specifications.freedesktop.org/desktop-entry-spec/latest/>

# XXX It would be nice to have desktop.in created with configure_file(),
# so that we can do some variable substitution, like for the other data
# files here. However, it means that the POTFILES should list the built
# file, which lives in the build dir. Right now it doesn't work.
# https://github.com/mesonbuild/meson/issues/1733

applications_dir = join_paths(datadir, 'applications')

desktop_file = i18n.merge_file(
  input: gv_application_id + '.desktop.in',
  output: gv_application_id + '.desktop',
  po_dir: '../po',
  type: 'desktop',
  install: true,
  install_dir: applications_dir,
)

desktop_utils = find_program('desktop-file-validate', required: false)
if desktop_utils.found()
  test('Validate desktop file', desktop_utils,
    args: [ desktop_file ],
  )
endif

# ----------------------------------------------------- #
#         Appstream File                                #
# ----------------------------------------------------- #
# <https://www.freedesktop.org/software/appstream/docs/>

appstream_dir = join_paths(datadir, 'metainfo')

appstream_file = i18n.merge_file(
  input: gv_application_id + '.appdata.xml.in',
  output: gv_application_id + '.appdata.xml',
  po_dir: '../po',
  type: 'xml',
  install: true,
  install_dir: appstream_dir,
)

appstream_util = find_program('appstream-util', required: false)
if appstream_util.found()
  test('Validate appstream file', appstream_util,
    # Relax validation because the image width is too small
    args: [ 'validate-relax', appstream_file ]
  )
endif

# ----------------------------------------------------- #
#         GSettings Schemas                             #
# ----------------------------------------------------- #
# <https://developer.gnome.org/gio/stable/GSettings.html>

schemas_dir = join_paths(datadir, 'glib-2.0', 'schemas')

schema_conf = configuration_data()
schema_conf.set('id', gv_application_id)
schema_conf.set('path', gv_application_path)

schema_file = configure_file(
  input: gv_application_id + '.gschema.xml.in',
  output: gv_application_id + '.gschema.xml',
  configuration: schema_conf,
  install_dir: schemas_dir,
)

#gv_settings = gnome.compile_schemas(
#  depend_files: schema_file
#)

# Generate enums.xml file.
# XXX gnome.compile_schemas() can't generate the enum.xml file automatically,
# like it was done with the autotools. We need to do it explicitly.
# https://github.com/mesonbuild/meson/issues/1687
# XXX It's unfortunate that we have to list the headers here,
# maybe that would be better done in src/ ?
enum_headers = [
  '../src/ui/gv-main-window.h',
  '../src/ui/gv-status-icon.h',
]

generate_enums = gnome.mkenums(gv_application_id + '.enums.xml',
  sources: enum_headers,
  comments: '<!-- @comment@ -->',
  fhead: '<schemalist>',
  vhead: '  <@type@ id="' + gv_application_id + '.@EnumName@">',
  vprod: '    <value nick="@valuenick@" value="@valuenum@"/>',
  vtail: '  </@type@>',
  ftail: '</schemalist>',
  install_header: true,
  install_dir: schemas_dir
)

# Compile schemas for running the application in-tree
# XXX We use a custom target, because gnome.compile_schemas()
# would run in the source dir rather than the build dir, hence
# missing the generated 'enums.xml' file.
# https://github.com/mesonbuild/meson/issues/1687
compile_schemas = custom_target('glib-compile-schemas',
  output: 'gschemas.compiled',
  build_by_default: true,
  install: false,
  command: [
    find_program('glib-compile-schemas'),
    meson.current_build_dir(),
  ],
  depends: [ generate_enums ],
)

compile_schemas = find_program('glib-compile-schemas', required: false)
if compile_schemas.found()
  test('Validate schema file', compile_schemas,
     args: [ '--strict', '--dry-run', meson.current_build_dir() ],
# Requires meson >= 0.46, commenting for now
#    depends: [ generate_enums ],
  )
endif

# ----------------------------------------------------- #
#         Old GSettings Schemas                         #
# ----------------------------------------------------- #

# In version 4.1, I changed application ID to io.gitlab.Goodvibes.
# This means changing the path and names of all the data files,
# including gsettings schema files. And it turns out that the only
# way to access the settings with the old app id (for migrating it
# to the new app id) is to actually keep on shipping gschema and enum
# files with the old name. So that's what we're doing here.

old_application_id = 'com.elboulangero.Goodvibes'
old_application_path = '/com/elboulangero/Goodvibes'

old_schema_conf = configuration_data()
old_schema_conf.set('id', old_application_id)
old_schema_conf.set('path', old_application_path)

old_schema_file = configure_file(
  input: gv_application_id + '.gschema.xml.in',
  output: old_application_id + '.gschema.xml',
  configuration: old_schema_conf,
  install_dir: schemas_dir,
)

old_generate_enums = gnome.mkenums(old_application_id + '.enums.xml',
  sources: enum_headers,
  comments: '<!-- @comment@ -->',
  fhead: '<schemalist>',
  vhead: '  <@type@ id="' + old_application_id + '.@EnumName@">',
  vprod: '    <value nick="@valuenick@" value="@valuenum@"/>',
  vtail: '  </@type@>',
  ftail: '</schemalist>',
  install_header: true,
  install_dir: schemas_dir
)

# ----------------------------------------------------- #
#         Manual Pages                                  #
# ----------------------------------------------------- #

# XXX That would be nice to use install_man(), however right now
# it's not possible to use it with generated files. So we have
# to use a custom target and zip the files manually.
# https://github.com/mesonbuild/meson/issues/1550

manuals_dir = join_paths(mandir, 'man1')

manual_conf = configuration_data()
manual_conf.set('package', gv_name_lowercase)
manual_conf.set('Package', gv_name_camelcase)
manual_conf.set('PACKAGE', gv_name_uppercase)
manual_conf.set('version', gv_version)
manual_conf.set('homepage', gv_homepage)
manual_conf.set('author', gv_author_name)

gzip = find_program('gzip', required: true)

foreach exe: [ 'goodvibes', 'goodvibes-client' ]
  tmp_man_file = configure_file(
    input: exe + '.1.in',
    output: exe + '.1',
    configuration: manual_conf,
  )
  custom_target('man-' + exe,
    command: [ gzip, '--keep', '--force', tmp_man_file ],
    output: exe + '.1.gz',
    install: true,
    install_dir: manuals_dir,
  )
endforeach

# ------------------------------------------------------- #
#         Subdirectories                                  #
# ------------------------------------------------------- #

subdir('icons')
