#project(eXl_Math)

set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/math)

FILE(GLOB_RECURSE EXL_MATH_HEADER_FILES LIST_DIRECTORIES false ${INCLUDE_DIR}/*.hpp ${INCLUDE_DIR}/*.inl)

SET(SOURCES_STATIC

  vector4.cpp
  vector3.cpp
  vector2.cpp
  matrix4.cpp
  math.cpp
  quaternion.cpp

)

add_library(eXl_MathStatic STATIC
  ${SOURCES_STATIC}
)

ADD_EXL_COMMON_DEFS(eXl_MathStatic)

target_compile_definitions(eXl_MathStatic PRIVATE EXL_DISABLE_GLOBAL_ALLOC)

if(${LINUX})
  SET_TARGET_PROPERTIES( eXl_MathStatic PROPERTIES COMPILE_FLAGS -fPIC)
endif(${LINUX})

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
  polygoni.cpp
  polygonf.cpp
  polygond.cpp
  aabb2dpolygoni.cpp
  seginter.cpp
  )

if(${ENABLE_ARMADILLO})
  set(SOURCES ${SOURCES}
  
    ${INCLUDE_DIR}/obb.hpp
    ${INCLUDE_DIR}/principalaxis.hpp
  
    obb.cpp
    principalaxis.cpp
  )
endif()

add_library(eXl_Math ${LIBRARY_TYPE} ${SOURCES})

SETUP_EXL_LIB(eXl_Math "MATH" DEPENDENCIES eXl_MathStatic eXl_Core)



