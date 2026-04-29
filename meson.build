project('powertop', 'cpp', 'c',
  version: '2.15',
  meson_version: '>= 0.56.0',
  default_options: [
    'cpp_std=c++20',
    'warning_level=1',
    'buildtype=debugoptimized',
  ]
)

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

# Dependencies
ncursesw_dep = dependency('ncursesw', required: false)
if not ncursesw_dep.found()
  ncursesw_dep = dependency('ncurses', required: false)
endif
if not ncursesw_dep.found()
  ncursesw_dep = cc.find_library('ncursesw', required: false)
  if not ncursesw_dep.found()
    ncursesw_dep = cc.find_library('ncurses', required: true)
  endif
endif

pciutils_dep = dependency('libpci', required: false)
if not pciutils_dep.found()
  pciutils_dep = cc.find_library('pci', required: false)
endif

libtracefs_dep = dependency('libtracefs', required: true)

libnl3_dep = dependency('libnl-3.0', version: '>= 3.0', required: false)
libnlgenl3_dep = dependency('libnl-genl-3.0', version: '>= 3.0', required: false)

libnl_deps = []
has_libnl20 = false

if libnl3_dep.found()
  libnl_deps += [libnl3_dep]
  if libnlgenl3_dep.found()
    libnl_deps += [libnlgenl3_dep]
  else
    libnlgenl3_dep = cc.find_library('nl-genl-3', required: true)
    libnl_deps += [libnlgenl3_dep]
  endif
  has_libnl20 = true
else
  libnl20_dep = dependency('libnl-2.0', version: '>= 2.0', required: false)
  if libnl20_dep.found()
    libnl_deps = [libnl20_dep]
    has_libnl20 = true
  else
    libnl1_dep = dependency('libnl-1', required: true)
    libnl_deps = [libnl1_dep]
  endif
endif

pthread_dep = dependency('threads')
resolv_dep = cc.find_library('resolv', required: true)
rt_dep = cc.find_library('rt', required: false)

# Configuration
conf_data = configuration_data()
conf_data.set_quoted('PACKAGE', meson.project_name())
conf_data.set_quoted('PACKAGE_VERSION', meson.project_version())
conf_data.set_quoted('PACKAGE_NAME', meson.project_name())
conf_data.set_quoted('PACKAGE_STRING', meson.project_name() + ' ' + meson.project_version())
conf_data.set_quoted('PACKAGE_BUGREPORT', 'powertop@lists.01.org')
conf_data.set_quoted('PACKAGE_URL', 'https://github.com/fenrus75/powertop')
conf_data.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))

if has_libnl20
  conf_data.set('HAVE_LIBNL20', 1)
endif

if not pciutils_dep.found()
  conf_data.set('HAVE_NO_PCI', 1)
endif

check_headers = [
  'fcntl.h',
  'libintl.h',
  'limits.h',
  'locale.h',
  'malloc.h',
  'ncurses.h',
  'stdint.h',
  'stdlib.h',
  'string.h',
  'sys/ioctl.h',
  'sys/socket.h',
  'sys/statfs.h',
  'sys/time.h',
  'termios.h',
  'unistd.h',
]

foreach h : check_headers
  if cc.has_header(h)
    conf_data.set('HAVE_' + h.underscorify().to_upper(), 1)
  endif
endforeach

check_functions = [
  'clock_gettime',
  'fdatasync',
  'getpagesize',
  'gettimeofday',
  'memmove',
  'memset',
  'mkdir',
  'munmap',
  'pow',
  'realpath',
  'regcomp',
  'select',
  'setlocale',
  'socket',
  'sqrt',
  'strcasecmp',
  'strchr',
  'strrchr',
  'strdup',
  'strerror',
  'strncasecmp',
  'strstr',
  'strtoul',
  'strtoull',
]

foreach f : check_functions
  if cc.has_function(f)
    conf_data.set('HAVE_' + f.underscorify().to_upper(), 1)
  endif
endforeach

configure_file(output: 'config.h', configuration: conf_data)

add_project_arguments('-DHAVE_CONFIG_H', '-Wall', '-W', language: ['c', 'cpp'])

# src directory
src_inc = include_directories('src')

css_h = custom_target('css.h',
  input: 'src/powertop.css',
  output: 'css.h',
  command: [find_program('src/csstoh.sh'), '@INPUT@', '@OUTPUT@']
)

powertop_sources = [
  css_h,
  'src/devlist.cpp',
  'src/devlist.h',
  'src/display.cpp',
  'src/display.h',
  'src/lib.cpp',
  'src/lib.h',
  'src/main.cpp',
  'src/calibrate/calibrate.cpp',
  'src/calibrate/calibrate.h',
  'src/cpu/abstract_cpu.cpp',
  'src/cpu/cpu.cpp',
  'src/cpu/cpu.h',
  'src/cpu/cpu_core.cpp',
  'src/cpu/cpu_linux.cpp',
  'src/cpu/cpu_package.cpp',
  'src/cpu/cpu_rapl_device.cpp',
  'src/cpu/cpu_rapl_device.h',
  'src/cpu/cpudevice.cpp',
  'src/cpu/cpudevice.h',
  'src/cpu/dram_rapl_device.cpp',
  'src/cpu/dram_rapl_device.h',
  'src/cpu/intel_cpus.cpp',
  'src/cpu/intel_cpus.h',
  'src/cpu/intel_gpu.cpp',
  'src/cpu/rapl/rapl_interface.cpp',
  'src/cpu/rapl/rapl_interface.h',
  'src/devices/ahci.cpp',
  'src/devices/ahci.h',
  'src/devices/alsa.cpp',
  'src/devices/alsa.h',
  'src/devices/backlight.cpp',
  'src/devices/backlight.h',
  'src/devices/devfreq.cpp',
  'src/devices/devfreq.h',
  'src/devices/device.cpp',
  'src/devices/device.h',
  'src/devices/device_manager.cpp',
  'src/devices/gpu_rapl_device.cpp',
  'src/devices/gpu_rapl_device.h',
  'src/devices/i915-gpu.cpp',
  'src/devices/i915-gpu.h',
  'src/devices/network.cpp',
  'src/devices/network.h',
  'src/devices/rfkill.cpp',
  'src/devices/rfkill.h',
  'src/devices/runtime_pm.cpp',
  'src/devices/runtime_pm.h',
  'src/devices/thinkpad-fan.cpp',
  'src/devices/thinkpad-fan.h',
  'src/devices/thinkpad-light.cpp',
  'src/devices/thinkpad-light.h',
  'src/devices/usb.cpp',
  'src/devices/usb.h',
  'src/measurement/acpi.cpp',
  'src/measurement/acpi.h',
  'src/measurement/extech.cpp',
  'src/measurement/extech.h',
  'src/measurement/measurement.cpp',
  'src/measurement/measurement_manager.cpp',
  'src/measurement/measurement.h',
  'src/measurement/sysfs.cpp',
  'src/measurement/sysfs.h',
  'src/measurement/opal-sensors.cpp',
  'src/measurement/opal-sensors.h',
  'src/parameters/learn.cpp',
  'src/parameters/parameters.cpp',
  'src/parameters/parameters.h',
  'src/parameters/persistent.cpp',
  'src/perf/perf.cpp',
  'src/perf/perf.h',
  'src/perf/perf_bundle.cpp',
  'src/perf/perf_bundle.h',
  'src/perf/perf_event.h',
  'src/process/do_process.cpp',
  'src/process/interrupt.cpp',
  'src/process/interrupt.h',
  'src/process/powerconsumer.cpp',
  'src/process/powerconsumer.h',
  'src/process/process.cpp',
  'src/process/process.h',
  'src/process/processdevice.cpp',
  'src/process/processdevice.h',
  'src/process/timer.cpp',
  'src/process/timer.h',
  'src/process/work.cpp',
  'src/process/work.h',
  'src/report/report-data-html.cpp',
  'src/report/report-data-html.h',
  'src/report/report-formatter-base.cpp',
  'src/report/report-formatter-base.h',
  'src/report/report-formatter-csv.cpp',
  'src/report/report-formatter-csv.h',
  'src/report/report-formatter-html.cpp',
  'src/report/report-formatter-html.h',
  'src/report/report-formatter.h',
  'src/report/report-maker.cpp',
  'src/report/report-maker.h',
  'src/report/report.cpp',
  'src/report/report.h',
  'src/tuning/bluetooth.cpp',
  'src/tuning/bluetooth.h',
  'src/tuning/ethernet.cpp',
  'src/tuning/ethernet.h',
  'src/tuning/iw.c',
  'src/tuning/iw.h',
  'src/tuning/nl80211.h',
  'src/tuning/runtime.cpp',
  'src/tuning/runtime.h',
  'src/tuning/tunable.cpp',
  'src/tuning/tunable.h',
  'src/tuning/tuning.cpp',
  'src/tuning/tuning.h',
  'src/tuning/tuningsysfs.cpp',
  'src/tuning/tuningsysfs.h',
  'src/tuning/tuningusb.cpp',
  'src/tuning/tuningusb.h',
  'src/tuning/tuningi2c.cpp',
  'src/tuning/tuningi2c.h',
  'src/tuning/wifi.cpp',
  'src/tuning/wifi.h',
  'src/wakeup/wakeup.cpp',
  'src/wakeup/wakeup.h',
  'src/wakeup/waketab.cpp',
  'src/wakeup/wakeup_ethernet.cpp',
  'src/wakeup/wakeup_ethernet.h',
  'src/wakeup/wakeup_usb.cpp',
  'src/wakeup/wakeup_usb.h',
  'src/test_framework.cpp',
  'src/test_framework.h',
]

if get_option('test-framework')
  add_project_arguments('-DENABLE_TEST_FRAMEWORK', language: 'cpp')
endif

executable('powertop',
  powertop_sources,
  include_directories: [src_inc, include_directories('.')],
  dependencies: [
    ncursesw_dep,
    pciutils_dep,
    libtracefs_dep,
    libnl_deps,
    pthread_dep,
    resolv_dep,
    rt_dep,
  ],
  cpp_args: [
    '-Wformat',
    '-Wshadow',
    '-fno-omit-frame-pointer',
    '-fstack-protector',
    '-fpermissive',
    '-Wsuggest-override',
  ],
  install: true
)

# Documentation
install_man('doc/powertop.8')

# Bash completion
bash_completion_dep = dependency('bash-completion', required: false)
if bash_completion_dep.found()
  bash_completion_dir = bash_completion_dep.get_variable(pkgconfig: 'completionsdir', default_value: get_option('datadir') / 'bash-completion' / 'completions')
else
  bash_completion_dir = get_option('datadir') / 'bash-completion' / 'completions'
endif

install_data('scripts/bash-completion/powertop',
  install_dir: bash_completion_dir
)

# Subdirs
if get_option('nls')
  subdir('po')
endif

if get_option('enable-tests')
  subdir('tests')
endif
