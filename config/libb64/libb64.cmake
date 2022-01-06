SET(LIBB64_INCLUDE_DIR ${LIBB64_ROOT}/include)
add_subdirectory(${LIBB64_ROOT} ${OUT_DIR}/modules/libb64)
set_target_properties(libb64 PROPERTIES FOLDER Dependencies)