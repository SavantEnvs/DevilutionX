# Passed to upstream's configure as CMAKE_PROJECT_DevilutionX_INCLUDE (mayhem/build.sh): runs
# right after upstream's project(DevilutionX) call and defers the fuzz-target definitions to the
# end of the top-level CMakeLists.txt, when every libdevilutionx_* target exists. No upstream file
# is modified.
# Deferred-call arguments are expanded when the call runs, so keep the path in a variable.
set(MAYHEM_FUZZ_TARGETS_FILE "${CMAKE_CURRENT_LIST_DIR}/targets.cmake")
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL include "${MAYHEM_FUZZ_TARGETS_FILE}")
