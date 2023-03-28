unit_tests = [
  'metadata',
  'playlist-utils',
  'station-list',
]

if mutest_dep.found()
  foreach unit: unit_tests
    test(unit,
      executable(unit, unit + '.c',
        dependencies: [ gvcore_dep, mutest_dep ],
        include_directories: root_inc,
      ),
      workdir: meson.current_source_dir(),
    )
  endforeach
endif
