# Fuzz targets, defined inside upstream's CMake project (see project_include.cmake) so each one
# links exactly the upstream object libraries the matching upstream test links. Binaries land in
# the build root, next to the assets/ directory upstream's build stages there (fuzz_pack reads the
# game-data tables from it through the default BasePath()/assets/ lookup, like upstream's tests).
set(MAYHEM_FUZZ_DIR "${PROJECT_SOURCE_DIR}/mayhem/fuzz")

add_library(mayhem_lsan_off OBJECT "${PROJECT_SOURCE_DIR}/mayhem/lsan_off.cc")
add_library(mayhem_standalone_main OBJECT "$ENV{STANDALONE_FUZZ_MAIN}")
set_source_files_properties("$ENV{STANDALONE_FUZZ_MAIN}" PROPERTIES LANGUAGE C)
separate_arguments(MAYHEM_LIB_FUZZING_ENGINE UNIX_COMMAND "$ENV{LIB_FUZZING_ENGINE}")

# mayhem_fuzzer(<name> <deps...>): builds <name> (libFuzzer) and <name>-standalone (run-once).
function(mayhem_fuzzer name)
  foreach(variant fuzzer standalone)
    if(variant STREQUAL "fuzzer")
      set(exe "${name}")
    else()
      set(exe "${name}-standalone")
    endif()
    add_executable(${exe} "${MAYHEM_FUZZ_DIR}/${name}.cpp" $<TARGET_OBJECTS:mayhem_lsan_off>)
    set_target_properties(${exe} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    target_include_directories(${exe} PRIVATE "${PROJECT_SOURCE_DIR}/Source" "${MAYHEM_FUZZ_DIR}")
    target_link_dependencies(${exe} PRIVATE ${ARGN})
    if(variant STREQUAL "fuzzer")
      target_link_options(${exe} PRIVATE ${MAYHEM_LIB_FUZZING_ENGINE})
    else()
      target_sources(${exe} PRIVATE $<TARGET_OBJECTS:mayhem_standalone_main>)
    endif()
    set_property(GLOBAL APPEND PROPERTY MAYHEM_FUZZ_TARGETS ${exe})
  endforeach()
endfunction()

mayhem_fuzzer(fuzz_pcx_to_clx
  DevilutionX::SDL app_fatal_for_testing language_for_testing
  libdevilutionx_clx_render libdevilutionx_log libdevilutionx_pcx_to_clx libdevilutionx_surface)
mayhem_fuzzer(fuzz_codec libdevilutionx_codec app_fatal_for_testing)
mayhem_fuzzer(fuzz_pkware libdevilutionx_pkware_encrypt app_fatal_for_testing)
mayhem_fuzzer(fuzz_ini libdevilutionx_ini app_fatal_for_testing)
mayhem_fuzzer(fuzz_utf8 libdevilutionx_utf8)
mayhem_fuzzer(fuzz_data_file libdevilutionx_txtdata app_fatal_for_testing language_for_testing)
mayhem_fuzzer(fuzz_dvlnet_packet libdevilutionx)
mayhem_fuzzer(fuzz_dvlnet_frame_queue libdevilutionx)
mayhem_fuzzer(fuzz_pack libdevilutionx)
mayhem_fuzzer(fuzz_level_delta libdevilutionx)

get_property(_mayhem_fuzz_targets GLOBAL PROPERTY MAYHEM_FUZZ_TARGETS)
add_custom_target(mayhem_fuzzers DEPENDS ${_mayhem_fuzz_targets})

# Upstream resolves target_link_dependencies() at the end of its CMakeLists.txt, which ran before
# this deferred file; resolve again for the targets added here (already-resolved ones are skipped).
resolve_target_link_dependencies()
