ui_files = [
  'app-menu.glade',
  'main-window.glade',
  'menubar.glade',
  'prefs-window.glade',
  'station-dialog.glade',
  'status-icon-menu.glade',
]

install_data(ui_files,
  install_dir: join_paths(get_option('datadir'), meson.project_name(), 'ui')
)

gtk_builder_tool = find_program('gtk-builder-tool', required: false)
if gtk_builder_tool.found()
  foreach ui_file : ui_files
    test('Validate ' + ui_file, gtk_builder_tool,
      args: [ 'validate', join_paths(meson.current_source_dir(), ui_file) ]
    )
  endforeach
endif
