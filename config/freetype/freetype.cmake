add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/modules/freetype ${OUT_DIR}/modules/freetype)
set_target_properties(freetype PROPERTIES FOLDER Dependencies)
SET(FREETYPE_INCLUDE_DIR ${FREETYPE_ROOT}/include)