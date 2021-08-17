
include_directories(${GTEST_INCLUDE_DIR})
link_directories(${GTEST_LIBRARY_DIR})

add_executable(dunatk_tests
objecttest.cpp
navmeshtest.cpp
penumbratest.cpp
transformtest.cpp
resourcetest.cpp
graphtest.cpp
maptest.cpp
)

SETUP_EXL_TARGET(dunatk_tests DEPENDENCIES DunAtkLib)
target_link_libraries(dunatk_tests PRIVATE ${GTEST_LIBRARIES})