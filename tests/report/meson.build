tidy = find_program('tidy', required: false)
html2text = find_program('html2text', required: false)
mdl = find_program('mdl', required: false)
mdformat = find_program('mdformat', required: false)
python3 = find_program('python3', required: false)

tidy_args = []
if tidy.found()
  tidy_args = [
    '-DHAVE_TIDY',
    '-DTIDY_BIN="' + tidy.full_path() + '"',
  ]
endif

html2text_args = []
if html2text.found()
  html2text_args = [
    '-DHAVE_HTML2TEXT',
    '-DHTML2TEXT_BIN="' + html2text.full_path() + '"',
  ]
endif

md_args = []
if mdl.found()
  md_args += [
    '-DHAVE_MDL',
    '-DMDL_BIN="' + mdl.full_path() + '"',
  ]
endif
if mdformat.found()
  md_args += [
    '-DHAVE_MDFORMAT',
    '-DMDFORMAT_BIN="' + mdformat.full_path() + '"',
  ]
endif

csv_args = []
if python3.found()
  csv_args = [
    '-DHAVE_PYTHON3',
    '-DPYTHON3_BIN="' + python3.full_path() + '"',
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
  '../../src/report/report-formatter-md.cpp',
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
  ] + tidy_args + html2text_args,
  dependencies: [ncursesw_dep],
)

test_csv_report_exe = executable(
  'powertop-test-csv-report',
  report_sources + ['test_csv_report.cpp'],
  include_directories: report_incdirs,
  cpp_args: report_cpp_args + csv_args,
  dependencies: [ncursesw_dep],
)

test('report: html generation', test_report_exe)
test('report: csv generation',  test_csv_report_exe)


test_md_report_exe = executable(
  'powertop-test-md-report',
  report_sources + ['test_md_report.cpp'],
  include_directories: report_incdirs,
  cpp_args: report_cpp_args + md_args,
  dependencies: [ncursesw_dep],
)

test('report: md generation', test_md_report_exe)
