
if(${ANDROID})  
  SET(Boost_INCLUDE_DIR ${Boost_ROOT})
  
if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
  SET(Boost_LIBRARY_DIR ${Boost_ROOT}/stage_arm_r20/lib)
  SET(Boost_LIBRARIES )
else()
  SET(Boost_LIBRARY_DIR ${Boost_ROOT}/stage/lib)
  SET(Boost_LIBRARIES )
endif()

link_directories(${Boost_LIBRARY_DIR})
  
else()
  SET(Boost_USE_STATIC_LIBS TRUE)
  find_package(Boost 1.47.0 )
endif()

set(EXL_DEPS_INCLUDE ${EXL_DEPS_INCLUDE} ${Boost_INCLUDE_DIR})
set(EXL_COMPILER_SYS_DEFINITIONS ${EXL_COMPILER_SYS_DEFINITIONS} -DBOOST_ALL_NO_LIB)

if(NOT ${Boost_USE_STATIC_LIBS})
  set(EXL_COMPILER_SYS_DEFINITIONS ${EXL_COMPILER_SYS_DEFINITIONS}
    -DBOOST_ALL_DYN_LINK
  )
endif()

set(EXL_COMPILER_DEFINITIONS ${EXL_COMPILER_DEFINITIONS}
  -DBOOST_DISABLE_ABI_HEADERS
)