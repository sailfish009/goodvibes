unit_tests = [
  'metadata',
  'station-list',
]

if mutest_dep.found()
  foreach unit: unit_tests
    test(unit,
      executable(unit, unit + '.c',
        dependencies: [ gvcore_dep, mutest_dep ],
        include_directories: root_inc,
      ),
    )
  endforeach
endif
