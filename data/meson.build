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

# ----------------------------------------------------- #
#         D-Bus Service File                            #
# ----------------------------------------------------- #

services_dir = join_paths(get_option('datadir'), 'dbus-1/services')

# We need an additional configuration object
abs_bin_dir = join_paths(get_option('prefix'), get_option('bindir'))
service_conf = configuration_data()
service_conf.merge_from(gv_conf)
service_conf.set('bindir', abs_bin_dir)

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

applications_dir = join_paths(get_option('datadir'), 'applications')

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
#         GSettings Schemas                             #
# ----------------------------------------------------- #

schemas_dir = join_paths(get_option('datadir'), 'glib-2.0', 'schemas')

schema_file = configure_file(
  input: gv_application_id + '.gschema.xml.in',
  output: gv_application_id + '.gschema.xml',
  configuration: gv_conf,
  install_dir: schemas_dir,
)

#gv_settings = gnome.compile_schemas(
#  depend_files: schema_file
#)

# General enums.xml file.
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
#         Manual Pages                                  #
# ----------------------------------------------------- #

# XXX That would be nice to use install_man(), however right now
# it's not possible to use it with generated files. So we have
# to use a custom target and zip the files manually.
# https://github.com/mesonbuild/meson/issues/1550

manuals_dir = join_paths(get_option('mandir'), 'man1')

gzip = find_program('gzip', required: true)

foreach exe: [ 'goodvibes', 'goodvibes-client' ]
  tmp_man_file = configure_file(
    input: exe + '.1.in',
    output: exe + '.1',
    configuration: gv_conf,
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
subdir('ui')
