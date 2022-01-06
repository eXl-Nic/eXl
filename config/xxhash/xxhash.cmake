set(XXHASH_BUILD_XXHSUM OFF CACHE BOOL "Build the xxhsum binary" FORCE)
add_subdirectory(${XXHASH_ROOT}/cmake_unofficial ${OUT_DIR}/modules/xxhash)
set_target_properties(xxhash PROPERTIES FOLDER Dependencies)
SET(XXHASH_INCLUDE_DIR ${XXHASH_ROOT})