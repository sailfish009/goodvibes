# SPDX-License-Identifier: GPL-3.0-or-later

themes = [
  [ 'hicolor', [
    [ '16x16', gv_icon_name + '.png' ],
    [ '22x22', gv_icon_name + '.png' ],
    [ '24x24', gv_icon_name + '.png' ],
    [ '32x32', gv_icon_name + '.png' ],
    [ '48x48', gv_icon_name + '.png' ],
    [ '256x256', gv_icon_name + '.png' ],
    [ '512x512', gv_icon_name + '.png' ],
    [ 'symbolic', gv_icon_name + '-symbolic.svg' ],
  ]]
]

foreach i : themes
  theme = i[0]
  sizes = i[1]
  foreach i : sizes
    size = i[0]
    icon = i[1]
    install_data(join_paths(theme, size, 'apps', icon),
      install_dir : join_paths(datadir, 'icons', theme, size, 'apps')
    )
  endforeach
endforeach
