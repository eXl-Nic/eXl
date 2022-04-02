#project(eXl_Math)

set(INCLUDE_DIR ${EXL_ROOT}/include/math)

FILE(GLOB_RECURSE EXL_MATH_HEADER_FILES LIST_DIRECTORIES false ${INCLUDE_DIR}/*.hpp ${INCLUDE_DIR}/*.inl)


if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
endif()

set( SOURCES
  mathplugin.cpp
  mathtools.cpp
  halfedge.cpp
  
  ${EXL_MATH_HEADER_FILES}
)

set( SOURCES
  ${SOURCES}
  seginter.cpp
  )

if("ON" OR NOT ${EXL_BUILD_SHARED} OR MSVC)
set( SOURCES
  ${SOURCES}
  polygoni.cpp
  polygonf.cpp
  polygond.cpp
  aabb2dpolygoni.cpp
)
endif()

if(${ENABLE_ARMADILLO})
  set(SOURCES ${SOURCES}
    obb.cpp
    principalaxis.cpp
  )
endif()

add_library(eXl_Math ${LIBRARY_TYPE} ${SOURCES})
SETUP_EXL_LIB(eXl_Math "MATH" DEPENDENCIES eXl_Core)

if(${ENABLE_ARMADILLO})
    target_include_directories(eXl_Math PRIVATE ${ARMADILLO_INCLUDE_DIR})
    target_link_directories(eXl_Math ${LIBRARY_DEPS_VISIBILITY} ${ARMADILLO_LIBRARY_DIR})
    set(PRIVATE_LIBRARIES ${PRIVATE_LIBRARIES} ${ARMADILLO_LIBRARIES})
endif()

target_link_libraries(eXl_Math PRIVATE ${PRIVATE_LIBRARIES})
