# SPDX-License-Identifier: GPL-3.0-or-later

ui_files = [
  'app-menu.glade',
  'main-window.glade',
  'menubar.glade',
  'prefs-window.glade',
  'station-dialog.glade',
  'status-icon-menu.glade',
]

install_data(ui_files,
  install_dir: join_paths(datadir, meson.project_name(), 'ui')
)

validator = find_program('ui-validator.sh')
foreach ui_file : ui_files
  test('Validate ' + ui_file, validator,
    args: [ files(ui_file) ]
  )
endforeach
