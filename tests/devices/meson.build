test_alsa_exe = executable(
  'powertop-test-alsa-serialize',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/parameters/parameters.cpp',
    '../../src/devices/device.cpp',
    '../../src/devices/alsa.cpp',
    'test_stubs.cpp',
    'test_alsa_serialize.cpp',
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

test('devices: alsa serialize', test_alsa_exe)

test_sysfs_tunable_exe = executable(
  'powertop-test-sysfs-tunable',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/tuning/tunable.cpp',
    '../../src/tuning/tuningsysfs.cpp',
    'test_sysfs_tunable.cpp',
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

test('devices: sysfs_tunable serialize', test_sysfs_tunable_exe)
