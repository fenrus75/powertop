test_lib_sources = [
  '../../src/lib.cpp',
  '../../src/test_framework.cpp',
  'test_read_file_content.cpp',
]

test_lib_exe = executable(
  'powertop-test-lib',
  test_lib_sources,
  include_directories: [
    include_directories('../..'),
    include_directories('../../src'),
    include_directories('..'),
  ],
  cpp_args: [
    '-DENABLE_TEST_FRAMEWORK',
    '-DHAVE_NO_PCI',
    '-DTEST_DATA_DIR="' + meson.current_source_dir() / 'data' + '"',
  ],
  dependencies: [ncursesw_dep],
)

test('lib: read_file_content', test_lib_exe)

test_lib_misc_exe = executable(
  'powertop-test-lib-misc',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    'test_lib_misc.cpp',
  ],
  include_directories: [
    include_directories('../..'),
    include_directories('../../src'),
    include_directories('..'),
  ],
  cpp_args: [
    '-DENABLE_TEST_FRAMEWORK',
    '-DHAVE_NO_PCI',
    '-DTEST_DATA_DIR="' + meson.current_source_dir() / 'data' + '"',
  ],
  dependencies: [ncursesw_dep],
)

test('lib: pt_gettime and align_string', test_lib_misc_exe)

_lib_incdirs = [
  include_directories('../..'),
  include_directories('../../src'),
  include_directories('..'),
]
_lib_cpp_args = [
  '-DENABLE_TEST_FRAMEWORK',
  '-DHAVE_NO_PCI',
  '-DTEST_DATA_DIR="' + meson.current_source_dir() / 'data' + '"',
]
_lib_sources = ['../../src/lib.cpp', '../../src/test_framework.cpp']

test_lib_pure_exe = executable(
  'powertop-test-lib-pure',
  _lib_sources + ['test_lib_pure.cpp'],
  include_directories: _lib_incdirs,
  cpp_args: _lib_cpp_args,
  dependencies: [ncursesw_dep],
)
test('lib: pure functions', test_lib_pure_exe)

test_lib_sysfs_exe = executable(
  'powertop-test-lib-sysfs',
  _lib_sources + ['test_lib_sysfs.cpp'],
  include_directories: _lib_incdirs,
  cpp_args: _lib_cpp_args,
  dependencies: [ncursesw_dep],
)
test('lib: sysfs and msr functions', test_lib_sysfs_exe)

test_lib_system_exe = executable(
  'powertop-test-lib-system',
  _lib_sources + ['test_lib_system.cpp'],
  include_directories: _lib_incdirs,
  cpp_args: _lib_cpp_args,
  dependencies: [ncursesw_dep],
)
test('lib: system functions', test_lib_system_exe)

test_lib_kallsyms_empty_exe = executable(
  'powertop-test-lib-kallsyms-empty',
  _lib_sources + ['test_lib_kallsyms_empty.cpp'],
  include_directories: _lib_incdirs,
  cpp_args: _lib_cpp_args,
  dependencies: [ncursesw_dep],
)
test('lib: kallsyms empty content', test_lib_kallsyms_empty_exe)

if pciutils_dep.found()
  test_lib_pci_exe = executable(
    'powertop-test-lib-pci',
    _lib_sources + ['test_lib_pci.cpp'],
    include_directories: _lib_incdirs,
    cpp_args: [
      '-DENABLE_TEST_FRAMEWORK',
      # deliberately NO -DHAVE_NO_PCI so real pci_* functions are compiled
      '-DTEST_DATA_DIR="' + meson.current_source_dir() / 'data' + '"',
    ],
    dependencies: [ncursesw_dep, pciutils_dep],
  )
  test('lib: pci id lookup', test_lib_pci_exe)
endif
