unit_tests = [
  'utils',
]

if mutest_dep.found()
  foreach unit: unit_tests
    test('base / ' + unit,
      executable(unit, unit + '.c',
        dependencies: [ gvbase_dep, mutest_dep ],
        include_directories: root_inc,
      ),
      workdir: meson.current_source_dir(),
    )
  endforeach
endif
