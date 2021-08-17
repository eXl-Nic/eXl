
include_directories(${GTEST_INCLUDE_DIR})
link_directories(${GTEST_LIBRARY_DIR})

add_executable(core_tests
luabindtest.cpp
)

SETUP_EXL_TARGET(core_tests DEPENDENCIES eXl_Core)
target_link_libraries(core_tests PRIVATE ${GTEST_LIBRARIES})