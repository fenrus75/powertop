tidy = find_program('tidy', required: false)

tidy_args = []
if tidy.found()
  tidy_args = [
    '-DHAVE_TIDY',
    '-DTIDY_BIN="' + tidy.full_path() + '"',
  ]
endif

report_sources = [
  css_h,
  '../../src/lib.cpp',
  '../../src/test_framework.cpp',
  '../../src/report/report.cpp',
  '../../src/report/report-maker.cpp',
  '../../src/report/report-formatter-html.cpp',
  '../../src/report/report-formatter-csv.cpp',
  '../../src/report/report-formatter-base.cpp',
  '../../src/report/report-data-html.cpp',
]

report_incdirs = [
  include_directories('../..'),
  include_directories('../../src'),
  include_directories('..'),
]

report_cpp_args = [
  '-DENABLE_TEST_FRAMEWORK',
  '-DHAVE_NO_PCI',
]

test_report_exe = executable(
  'powertop-test-report',
  report_sources + ['test_html_report.cpp'],
  include_directories: report_incdirs,
  cpp_args: report_cpp_args + [
    '-DTEST_DATA_DIR="' + meson.current_source_dir() / 'data' + '"',
  ] + tidy_args,
  dependencies: [ncursesw_dep],
)

test_csv_report_exe = executable(
  'powertop-test-csv-report',
  report_sources + ['test_csv_report.cpp'],
  include_directories: report_incdirs,
  cpp_args: report_cpp_args,
  dependencies: [ncursesw_dep],
)

test('report: html generation', test_report_exe)
test('report: csv generation',  test_csv_report_exe)

