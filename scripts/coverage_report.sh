#!/bin/bash
# Coverage snapshot helper for PowerTOP tests.
#
# Usage:
#   scripts/coverage_report.sh [snapshot_name] [build_dir]
#
# Arguments:
#   snapshot_name  Label for the output files (default: "cov")
#                  Output files are written to /tmp/pt_<snapshot_name>_src.info
#                  and the HTML report to /tmp/pt_<snapshot_name>_html/
#   build_dir      Meson build directory configured with -Db_coverage=true
#                  (default: build_cov)
#
# The script captures coverage data from <build_dir>, filters to src/ files
# only, prints a per-file summary to stdout, and generates an HTML report.
#
# Typical "before/after" workflow:
#
#   # 1. One-time build setup (if not already done):
#   meson setup build_cov -Db_coverage=true -Denable-tests=true
#
#   # 2. Run tests to populate coverage data:
#   ninja -C build_cov test
#
#   # 3. Capture baseline snapshot:
#   scripts/coverage_report.sh before
#
#   # 4. Add your new test, rebuild, run tests:
#   ninja -C build_cov test
#
#   # 5. Capture after snapshot:
#   scripts/coverage_report.sh after
#
#   # 6. Compare the two summaries (last line of each):
#   grep "^Total" /tmp/pt_before_src.info.summary /tmp/pt_after_src.info.summary

set -euo pipefail

LABEL="${1:-cov}"
BUILDDIR="${2:-build_cov}"

INITIAL="/tmp/pt_${LABEL}_initial.info"
RUN="/tmp/pt_${LABEL}_run.info"
COMBINED="/tmp/pt_${LABEL}_combined.info"
SRC="/tmp/pt_${LABEL}_src.info"
HTML="/tmp/pt_${LABEL}_html"

if [[ ! -d "$BUILDDIR" ]]; then
    echo "ERROR: build directory '$BUILDDIR' not found." >&2
    echo "       Run: meson setup $BUILDDIR -Db_coverage=true -Denable-tests=true" >&2
    exit 1
fi

echo "=== Capturing coverage from $BUILDDIR (label: $LABEL) ==="

# Baseline: all instrumented lines at zero (ensures unexecuted files appear)
lcov --initial --capture \
     --directory "$BUILDDIR" \
     --output-file "$INITIAL" \
     --ignore-errors inconsistent \
     --quiet

# Actual execution data
lcov --capture \
     --directory "$BUILDDIR" \
     --output-file "$RUN" \
     --ignore-errors inconsistent \
     --quiet

# Merge baseline + execution
lcov --add-tracefile "$INITIAL" \
     --add-tracefile "$RUN" \
     --output-file "$COMBINED" \
     --ignore-errors inconsistent \
     --quiet

# Filter to src/ only (exclude test files, system headers, build-generated files)
lcov --extract "$COMBINED" "*/src/*" \
     --output-file "$SRC" \
     --ignore-errors inconsistent \
     --quiet

echo ""
echo "=== Per-file coverage summary ==="
lcov --list "$SRC"

echo ""
echo "=== HTML report: $HTML/index.html ==="
genhtml "$SRC" --output-directory "$HTML" --quiet
echo "    Open with: xdg-open $HTML/index.html"
