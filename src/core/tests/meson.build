unit_tests = [
  'station-list',
]

# Make tests conditional on having mutest-1 installed system-wide,
# or available as a subproject.

mutest_dep = dependency('mutest-1', required: false)
if not mutest_dep.found()
  mutest = subproject('mutest',
    default_options: [ 'static=true' ],
    required: false,
  )
  if mutest.found()
    mutest_dep = mutest.get_variable('mutest_dep')
  else
    mutest_dep = disabler()
  endif
endif

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
