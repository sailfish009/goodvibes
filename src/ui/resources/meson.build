# SPDX-License-Identifier: GPL-3.0-or-later

# Resources

ui_resources_conf = configuration_data()
ui_resources_conf.set('path', gv_application_path)

ui_resources_file = configure_file(
  input: gv_application_id + '.Ui.gresource.xml.in',
  output: gv_application_id + '.Ui.gresource.xml',
  configuration: ui_resources_conf,
)

ui_resources = gnome.compile_resources('gv-ui-resources',
  ui_resources_file,
  c_name: 'gv_ui',
  export: true,
)

# Validation

ui_glade_files = [
  'main-window.glade',
  'prefs-window.glade',
  'station-dialog.glade',
]

validator = find_program(join_paths(meson.source_root(), 'scripts/meson/validate-ui-file'))
foreach file : ui_glade_files
  test('Validate ' + file, validator,
    args: [ files(file) ],
  )
endforeach
