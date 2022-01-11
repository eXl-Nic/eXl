add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/config/yojimbo ${OUT_DIR}/modules/yojimbo)
set_target_properties(yojimbo PROPERTIES FOLDER Dependencies)
SET(YOJIMBO_INCLUDE_DIR ${YOJIMBO_ROOT})