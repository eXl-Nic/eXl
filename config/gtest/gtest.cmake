if(${EXL_BUILD_TESTS})

SET(BUILD_SHARED_LIBS ON)
SET(BUILD_GMOCK OFF)
SET(INSTALL_GTEST OFF)
SET(gtest_force_shared_crt ON)

set(EXL_SUBMODULE_PROJECTS ${EXL_SUBMODULE_PROJECTS} ${EXL_ROOT}/modules/gtest)
set(EXL_SUBMODULE_TARGETS ${EXL_SUBMODULE_TARGETS} ${OUT_DIR}/modules/gtest)


SET(GTEST_INCLUDE_DIR ${GTEST_ROOT}/googletest/include)
SET(GTEST_LIBRARIES gtest gtest_main)

endif()