project('libcaphe', 'c',
        version: '0.1',
        license: 'GPLv3',
        default_options: [
          'c_std=c99',
          'warning_level=1'
        ])

glib_dep = dependency('glib-2.0', version: '>= 2.44')
gio_dep  = dependency('gio-2.0', version: '>= 2.44')
gio_unix_dep = dependency('gio-unix-2.0', version: '>= 2.44')

subdir('caphe')
