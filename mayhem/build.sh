#!/usr/bin/env bash
#
# mayhem/build.sh — build DevilutionX's fuzz harnesses and its standalone unit-test suite.
#
# Runs inside the commit image as `mayhem` in /mayhem; re-runnable and offline (every FetchContent
# dependency is pre-fetched under /opt/toolchains/deps by mayhem/Dockerfile).
#
#   build-fuzz/   upstream CMake project, sanitized ($SANITIZER_FLAGS + SanCov + $DEBUG_FLAGS), with
#                 the harnesses from mayhem/fuzz/ injected via CMAKE_PROJECT_DevilutionX_INCLUDE.
#                 Binaries: /mayhem/build-fuzz/<target> and <target>-standalone; every one links
#                 mayhem/lsan_off.cc (LeakSanitizer off at build time, see mayhem/fuzz/targets.cmake).
#   build-tests/  same project with normal flags: upstream's standalone gtest binaries (test.sh).
#
# Both build dirs match upstream's .gitignore (/build-*/), so `git clean -ffdX` removes them.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${STANDALONE_FUZZ_MAIN:=/opt/mayhem/StandaloneFuzzTargetMain.c}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE STANDALONE_FUZZ_MAIN MAYHEM_JOBS COVERAGE_FLAGS

: "${SRC:=/mayhem}"
cd "$SRC"

DEPS=/opt/toolchains/deps
# Configure options shared by both trees: no sound (SDL_audiolib is not packaged), no ZeroTier
# (libzt is a large git FetchContent with a network stack; TCP multiplayer and the shared dvlnet
# packet code stay in), assets copied rather than packed into an MPQ (no smpq tool).
COMMON=(
  -G Ninja
  -DBUILD_TESTING=ON
  -DNOSOUND=ON
  -DDISABLE_ZERO_TIER=ON
  -DDISABLE_LTO=ON
  -DDISCORD_INTEGRATION=OFF
  -DBUILD_ASSETS_MPQ=OFF
  -DBUILD_MOD_MPQS=OFF
  -DASAN=OFF -DUBSAN=OFF
  -DDEVILUTIONX_SYSTEM_SHEENBIDI=OFF
  -DDEVILUTIONX_SYSTEM_UNORDERED_DENSE=OFF
  -DDEVILUTIONX_SYSTEM_MAGIC_ENUM=OFF
  -DFETCHCONTENT_FULLY_DISCONNECTED=ON
  -DFETCHCONTENT_SOURCE_DIR_ASIO="$DEPS/asio-112f011/src"
  -DFETCHCONTENT_SOURCE_DIR_MAGIC_ENUM="$DEPS/magic_enum-0.9.7/src"
  -DFETCHCONTENT_SOURCE_DIR_MPQFS="$DEPS/mpqfs-1dc2407/src"
  -DFETCHCONTENT_SOURCE_DIR_SOL2="$DEPS/sol2-832ac77/src"
  -DFETCHCONTENT_SOURCE_DIR_SHEENBIDI="$DEPS/SheenBidi-2.9.0/src"
  -DFETCHCONTENT_SOURCE_DIR_UNORDERED_DENSE="$DEPS/unordered_dense-4.4.0/src"
  -DFETCHCONTENT_SOURCE_DIR_LIBSMACKERDEC="$DEPS/libsmackerdec-0aaaf8c/src"
)

# 1) Sanitized fuzz build. -fsanitize=fuzzer-no-link adds SanCov to the whole project (not just the
#    harness) so libFuzzer/Mayhem see coverage in the parsers; DEBUG_FLAGS last so -gdwarf-3 wins.
FUZZ_FLAGS="$SANITIZER_FLAGS -fsanitize=fuzzer-no-link $DEBUG_FLAGS"
cmake -S . -B build-fuzz "${COMMON[@]}" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_C_FLAGS="$FUZZ_FLAGS" \
  -DCMAKE_CXX_FLAGS="$FUZZ_FLAGS" \
  -DCMAKE_C_FLAGS_RELWITHDEBINFO="-O1" \
  -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O1" \
  -DCMAKE_EXE_LINKER_FLAGS="$SANITIZER_FLAGS" \
  -DCMAKE_SHARED_LINKER_FLAGS="$SANITIZER_FLAGS" \
  -DCMAKE_PROJECT_DevilutionX_INCLUDE="$SRC/mayhem/fuzz/project_include.cmake"
cmake --build build-fuzz -j"$MAYHEM_JOBS" --target mayhem_fuzzers

# 2) Upstream's standalone unit tests with normal flags (the functional oracle; test.sh runs them),
#    plus the copied test fixtures data_file_test reads.
TESTS=(cl2_to_clx_test codec_test data_file_test ini_test utf8_test parse_int_test str_cat_test
       format_int_test fixed_point_test pcx_to_clx_test
       devilutionx_copied_fixtures)
cmake -S . -B build-tests "${COMMON[@]}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_FLAGS="$COVERAGE_FLAGS" \
  -DCMAKE_CXX_FLAGS="$COVERAGE_FLAGS"
cmake --build build-tests -j"$MAYHEM_JOBS" --target "${TESTS[@]}"
