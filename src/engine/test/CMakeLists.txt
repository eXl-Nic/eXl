
include_directories(${GTEST_INCLUDE_DIR})
link_directories(${GTEST_LIBRARY_DIR})

add_executable(engine_tests
objecttest.cpp
navmeshtest.cpp
penumbratest.cpp
transformtest.cpp
resourcetest.cpp
graphtest.cpp
maptest.cpp
mphf.cpp

main.cpp
)

SETUP_EXL_TARGET(engine_tests DEPENDENCIES eXl_Engine)
target_link_libraries(engine_tests PRIVATE ${GTEST_LIBRARIES})