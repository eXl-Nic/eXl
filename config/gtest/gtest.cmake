if(${EXL_BUILD_TESTS})

SET(BUILD_SHARED_LIBS ON)
SET(BUILD_GMOCK OFF)
SET(INSTALL_GTEST OFF)
SET(gtest_force_shared_crt ON)

add_subdirectory(${GTEST_ROOT} ${OUT_DIR}/modules/gtest)
set_target_properties(gtest PROPERTIES FOLDER Dependencies)
set_target_properties(gtest_main PROPERTIES FOLDER Dependencies)

#Override gtest's opinionated folders to have something working out of the box
set_target_properties(gtest
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${OUT_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${OUT_DIR}
    PDB_OUTPUT_DIRECTORY ${OUT_DIR})
	
set_target_properties(gtest_main
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${OUT_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${OUT_DIR}
    PDB_OUTPUT_DIRECTORY ${OUT_DIR})

SET(GTEST_INCLUDE_DIR ${GTEST_ROOT}/googletest/include)
SET(GTEST_LIBRARIES gtest gtest_main)

endif()