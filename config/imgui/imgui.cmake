if(${EXL_BUILD_OGL})

set(EXL_SUBMODULE_PROJECTS ${EXL_SUBMODULE_PROJECTS} ${EXL_ROOT}/config/imgui)
set(EXL_SUBMODULE_TARGETS ${EXL_SUBMODULE_TARGETS} ${OUT_DIR}/modules/imgui)

SET(IMGUI_INCLUDE_DIR ${IMGUI_ROOT})

endif()