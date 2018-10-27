icon_name = meson.project_name()

themes = [
  [ 'hicolor', [
    [ '16x16', [
      icon_name + '.png'
    ]],
    [ '22x22', [
      icon_name + '.png'
    ]],
    [ '24x24', [
      icon_name + '.png'
    ]],
    [ '32x32', [
      icon_name + '.png'
    ]],
    [ '48x48', [
      icon_name + '.png'
    ]],
    [ '256x256', [
      icon_name + '.png'
    ]],
    [ '512x512', [
      icon_name + '.png'
    ]],
    [ 'symbolic', [
      icon_name + '-symbolic.svg'
    ]]
  ]]
]

foreach i : themes
  theme = i[0]
  sizes = i[1]
  foreach i : sizes
    size = i[0]
    icons = i[1]
    foreach icon : icons
      install_data(join_paths(theme, size, 'apps', icon),
        install_dir : join_paths(datadir, 'icons', theme, size, 'apps')
      )
    endforeach
  endforeach
endforeach
