set(CMAKE_DEBUG_POSTFIX "_d" CACHE STRING "")
set(CMAKE_RELWITHDEBINFO_POSTFIX "_dev" CACHE STRING "")
set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)

if(${ANDROID})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fno-rtti")
add_compile_options(-Wno-undefined-var-template)
add_compile_options(-Wno-inconsistent-missing-override)
endif()

if(MSVC) 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR- /we4541 /std:c++17")
endif()

if(${EXL_BUILD_OGL} OR ${EXL_BUILD_ENGINE})
set(EXL_REFLANG_ENABLED ON)
else()
set(EXL_REFLANG_ENABLED OFF)
endif()

set(CMAKE_DEBUG_POSTFIX _d)
set(OUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(EXL_DEPS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(EXL_COMPILER_DEFINITIONS -DTRACE_LEAKS -DNOMINMAX)
set(EXL_COMPILER_DEFINITIONS ${EXL_COMPILER_DEFINITIONS} -DEXL_THREADAWARE)
set(EXL_COMPILER_DEFINITIONS ${EXL_COMPILER_DEFINITIONS} -DEXL_TYPE_ENABLED)

set(EXL_COMPILER_SYS_DEFINITIONS)
if(MSVC)
  set(EXL_COMPILER_SYS_DEFINITIONS ${EXL_COMPILER_SYS_DEFINITIONS} -DMSVC_COMPILER)
endif()

if(EXL_BUILD_FOR_UE4)
  add_definitions(-D_ITERATOR_DEBUG_LEVEL=0 -DBOOST_FLYWEIGHT_EXPLICIT_INIT -DEXL_NAME_EXPLICIT_INIT)
endif()
set(EXECUTABLE_OUTPUT_PATH ${OUT_DIR})
set(LIBRARY_OUTPUT_PATH ${OUT_DIR})

if(${EXL_BUILD_SHARED})
  SET(LIBRARY_TYPE SHARED)
  SET(LIBRARY_DEPS_VISIBILITY PRIVATE)
  set(EXL_COMPILER_DEFINITIONS ${EXL_COMPILER_DEFINITIONS} -DEXL_SHARED_LIBRARY)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4251 /wd4275")
else()
  SET(LIBRARY_TYPE STATIC)
  SET(LIBRARY_DEPS_VISIBILITY PUBLIC)
endif()