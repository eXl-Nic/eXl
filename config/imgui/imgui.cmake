if(${EXL_BUILD_OGL})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/config/imgui ${OUT_DIR}/modules/imgui)
SET(IMGUI_INCLUDE_DIR ${IMGUI_ROOT})

set_target_properties(imgui PROPERTIES FOLDER Dependencies)

endif()