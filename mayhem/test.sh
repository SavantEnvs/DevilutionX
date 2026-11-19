#!/usr/bin/env bash
#
# mayhem/test.sh — run upstream's standalone googletest suites (built by mayhem/build.sh into
# build-tests/ with normal flags). They assert decoded values: PCX/CL2 pixels, codec round-trips,
# TSV field contents, INI parse/serialize output, UTF-8 decoding, integer/fixed-point parsing.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SRC:=/mayhem}"
cd "$SRC"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

TESTS=(cl2_to_clx_test codec_test data_file_test ini_test utf8_test parse_int_test str_cat_test
       format_int_test fixed_point_test pcx_to_clx_test)
EXPECTED_MIN=60   # the suites above define 75 tests at the integration commit

passed=0 failed=0 skipped=0
out=$(mktemp)
for t in "${TESTS[@]}"; do
  bin="build-tests/$t"
  if [ ! -x "$bin" ]; then
    echo "missing test binary $bin (mayhem/build.sh should have built it)" >&2
    failed=$((failed + 1)); continue
  fi
  "$bin" --gtest_color=no >"$out" 2>&1
  rc=$?
  p=$(sed -n 's/^\[  PASSED  \] \([0-9]*\) test.*/\1/p' "$out" | tail -1)
  f=$(sed -n 's/^\[  FAILED  \] \([0-9]*\) test.*/\1/p' "$out" | tail -1)
  s=$(sed -n 's/^\[  SKIPPED \] \([0-9]*\) test.*/\1/p' "$out" | tail -1)
  p=${p:-0} f=${f:-0} s=${s:-0}
  # A suite that exits non-zero, or reports nothing, counts as a failure.
  if [ "$rc" -ne 0 ] && [ "$f" -eq 0 ]; then f=1; fi
  if [ "$p" -eq 0 ] && [ "$f" -eq 0 ]; then f=1; fi
  echo "$t: passed=$p failed=$f skipped=$s (exit $rc)"
  [ "$f" -gt 0 ] && grep -E '^\[  FAILED  \]|Failure' "$out" | head -20
  passed=$((passed + p)) failed=$((failed + f)) skipped=$((skipped + s))
done
rm -f "$out"
if [ "$passed" -lt "$EXPECTED_MIN" ]; then
  echo "only $passed tests passed, expected at least $EXPECTED_MIN" >&2
  failed=$((failed + 1))
fi
emit_ctrf "googletest" "$passed" "$failed" "$skipped"
