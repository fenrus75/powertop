ncursesw_dep = dependency('ncursesw', required: false)
if not ncursesw_dep.found()
  ncursesw_dep = dependency('ncurses')
endif

test_wakeup_base_exe = executable(
  'powertop-test-wakeup-base',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/wakeup/wakeup.cpp',
    'test_wakeup_base.cpp',
  ],
  include_directories: [
    include_directories('../..'),
    include_directories('../../src'),
    include_directories('..'),
  ],
  cpp_args: [
    '-DENABLE_TEST_FRAMEWORK',
    '-DHAVE_NO_PCI',
  ],
  dependencies: [ncursesw_dep],
)

test('base: wakeup serialize', test_wakeup_base_exe)

test_tunable_base_exe = executable(
  'powertop-test-tunable-base',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/tuning/tunable.cpp',
    'test_tunable_base.cpp',
  ],
  include_directories: [
    include_directories('../..'),
    include_directories('../../src'),
    include_directories('..'),
  ],
  cpp_args: [
    '-DENABLE_TEST_FRAMEWORK',
    '-DHAVE_NO_PCI',
  ],
  dependencies: [ncursesw_dep],
)

test('base: tunable serialize', test_tunable_base_exe)

test_device_base_exe = executable(
  'powertop-test-device-base',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/devices/device.cpp',
    'test_device_base.cpp',
  ],
  include_directories: [
    include_directories('../..'),
    include_directories('../../src'),
    include_directories('..'),
  ],
  cpp_args: [
    '-DENABLE_TEST_FRAMEWORK',
    '-DHAVE_NO_PCI',
  ],
  dependencies: [ncursesw_dep],
)

test('base: device serialize', test_device_base_exe)

test_power_meter_base_exe = executable(
  'powertop-test-power-meter-base',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/measurement/measurement.cpp',
    'test_power_meter_base.cpp',
  ],
  include_directories: [
    include_directories('../..'),
    include_directories('../../src'),
    include_directories('..'),
  ],
  cpp_args: [
    '-DENABLE_TEST_FRAMEWORK',
    '-DHAVE_NO_PCI',
  ],
  dependencies: [ncursesw_dep],
)

test('base: power_meter serialize', test_power_meter_base_exe)

test_power_consumer_base_exe = executable(
  'powertop-test-power-consumer-base',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/process/powerconsumer.cpp',
    'powerconsumer_stubs.cpp',
    'test_power_consumer_base.cpp',
  ],
  include_directories: [
    include_directories('../..'),
    include_directories('../../src'),
    include_directories('..'),
  ],
  cpp_args: [
    '-DENABLE_TEST_FRAMEWORK',
    '-DHAVE_NO_PCI',
  ],
  dependencies: [ncursesw_dep],
)

test('base: power_consumer serialize', test_power_consumer_base_exe)

test_abstract_cpu_base_exe = executable(
  'powertop-test-abstract-cpu-base',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/cpu/abstract_cpu.cpp',
    '../../src/cpu/frequency.cpp',
    'test_abstract_cpu_base.cpp',
  ],
  include_directories: [
    include_directories('../..'),
    include_directories('../../src'),
    include_directories('..'),
  ],
  cpp_args: [
    '-DENABLE_TEST_FRAMEWORK',
    '-DHAVE_NO_PCI',
  ],
  dependencies: [ncursesw_dep],
)

test('base: abstract_cpu serialize', test_abstract_cpu_base_exe)

test_process_exe = executable(
  'powertop-test-process',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/process/process.cpp',
    '../../src/process/powerconsumer.cpp',
    'powerconsumer_stubs.cpp',
    'test_process.cpp',
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

test('base: process serialize', test_process_exe)

test_usb_wakeup_exe = executable(
  'powertop-test-usb-wakeup',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/wakeup/wakeup.cpp',
    '../../src/wakeup/wakeup_usb.cpp',
    'test_usb_wakeup.cpp',
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

test('base: usb_wakeup serialize', test_usb_wakeup_exe)

test_sysfs_pm_exe = executable(
  'powertop-test-sysfs-power-meter',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/measurement/measurement.cpp',
    '../../src/measurement/sysfs.cpp',
    'test_sysfs_power_meter.cpp',
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

test('base: sysfs_power_meter serialize', test_sysfs_pm_exe)

test_interrupt_exe = executable(
  'powertop-test-interrupt-serialize',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/process/interrupt.cpp',
    '../../src/process/powerconsumer.cpp',
    'powerconsumer_stubs.cpp',
    'test_interrupt_serialize.cpp',
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

test('base: interrupt serialize', test_interrupt_exe)

test_sysfs_tunable_methods_exe = executable(
  'powertop-test-sysfs-tunable-methods',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/tuning/tunable.cpp',
    '../../src/tuning/tuningsysfs.cpp',
    'test_sysfs_tunable_methods.cpp',
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

test('base: sysfs_tunable methods', test_sysfs_tunable_methods_exe)

test_work_exe = executable(
  'powertop-test-work',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/process/work.cpp',
    '../../src/process/powerconsumer.cpp',
    'powerconsumer_stubs.cpp',
    'test_work.cpp',
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

test('base: work fire/done', test_work_exe)

test_ethernet_wakeup_exe = executable(
  'powertop-test-ethernet-wakeup',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/wakeup/wakeup.cpp',
    '../../src/wakeup/wakeup_ethernet.cpp',
    'test_ethernet_wakeup.cpp',
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

test('base: ethernet_wakeup', test_ethernet_wakeup_exe)

test_tuningusb_exe = executable(
  'powertop-test-tuningusb',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/tuning/tunable.cpp',
    '../../src/tuning/tuningusb.cpp',
    'test_tuningusb.cpp',
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

test('base: usb_tunable', test_tuningusb_exe)

test_tuningi2c_exe = executable(
  'powertop-test-tuningi2c',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/tuning/tunable.cpp',
    '../../src/tuning/tuningi2c.cpp',
    'stub_runtime_pm.cpp',
    'test_tuningi2c.cpp',
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

test('base: i2c_tunable', test_tuningi2c_exe)

test_cpu_core_package_exe = executable(
  'powertop-test-cpu-core-package',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/cpu/abstract_cpu.cpp',
    '../../src/cpu/cpu_core.cpp',
    '../../src/cpu/cpu_package.cpp',
    '../../src/cpu/frequency.cpp',
    'test_cpu_core_package.cpp',
  ],
  include_directories: [
    include_directories('../..'),
    include_directories('../../src'),
    include_directories('..'),
  ],
  cpp_args: [
    '-DENABLE_TEST_FRAMEWORK',
    '-DHAVE_NO_PCI',
  ],
  dependencies: [ncursesw_dep],
)

test('base: cpu_core and cpu_package', test_cpu_core_package_exe)

test_cpu_linux_exe = executable(
  'powertop-test-cpu-linux',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/cpu/abstract_cpu.cpp',
    '../../src/cpu/cpu_linux.cpp',
    '../../src/cpu/frequency.cpp',
    'test_cpu_linux.cpp',
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

test('base: cpu_linux cstate formatting', test_cpu_linux_exe)

test_processdevice_exe = executable(
  'powertop-test-processdevice',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/process/processdevice.cpp',
    '../../src/process/powerconsumer.cpp',
    '../../src/devices/device.cpp',
    'powerconsumer_stubs.cpp',
    '../devices/test_stubs.cpp',
    'test_processdevice.cpp',
  ],
  include_directories: [
    include_directories('../..'),
    include_directories('../../src'),
    include_directories('..'),
  ],
  cpp_args: [
    '-DENABLE_TEST_FRAMEWORK',
    '-DHAVE_NO_PCI',
  ],
  dependencies: [ncursesw_dep],
)

test('base: processdevice', test_processdevice_exe)

test_timer_exe = executable(
  'powertop-test-timer',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/process/timer.cpp',
    '../../src/process/powerconsumer.cpp',
    'powerconsumer_stubs.cpp',
    'test_timer.cpp',
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

test('base: timer', test_timer_exe)

test_runtime_exe = executable(
  'powertop-test-runtime',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/tuning/tunable.cpp',
    '../../src/tuning/runtime.cpp',
    'stub_runtime_pm.cpp',
    'test_runtime.cpp',
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

test('base: runtime_tunable', test_runtime_exe)

test_wifi_tunables_exe = executable(
  'powertop-test-wifi-tunables',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/tuning/tunable.cpp',
    '../../src/tuning/wifi.cpp',
    'stub_iw.cpp',
    'test_wifi_tunables.cpp',
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

test('base: wifi_tunables', test_wifi_tunables_exe)

test_nhm_cpu_exe = executable(
  'powertop-test-nhm-cpu',
  [
    '../../src/lib.cpp',
    '../../src/test_framework.cpp',
    '../../src/cpu/abstract_cpu.cpp',
    '../../src/cpu/cpu_linux.cpp',
    '../../src/cpu/cpu_core.cpp',
    '../../src/cpu/cpu_package.cpp',
    '../../src/cpu/intel_cpus.cpp',
    '../../src/cpu/frequency.cpp',
    'test_nhm_cpu.cpp',
  ],
  include_directories: [
    include_directories('../..'),
    include_directories('../../src'),
    include_directories('..'),
  ],
  cpp_args: [
    '-DENABLE_TEST_FRAMEWORK',
    '-DHAVE_NO_PCI',
  ],
  dependencies: [ncursesw_dep],
)

test('base: nhm_cpu', test_nhm_cpu_exe)

test_learn_parameters_exe = executable(
  'powertop-test-learn-parameters',
  [
    '../../src/test_framework.cpp',
    '../../src/parameters/parameters.cpp',
    '../../src/parameters/learn.cpp',
    'test_learn_parameters.cpp',
  ],
  include_directories: [
    include_directories('../..'),
    include_directories('../../src'),
    include_directories('..'),
  ],
  cpp_args: [
    '-DENABLE_TEST_FRAMEWORK',
    '-DHAVE_NO_PCI',
  ],
  dependencies: [ncursesw_dep],
)

test('base: learn_parameters', test_learn_parameters_exe)
