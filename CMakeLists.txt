cmake_minimum_required(VERSION 3.13)

project(eXl)

include(eXlMacros.cmake)

add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)

set(ANDROID_PLATFORM 19)

set(Boost_ROOT "" CACHE PATH "Path to boost root directory")
set(GLEW_ROOT "" CACHE PATH "Path to glew root directory")
set(FREEIMAGE_ROOT "" CACHE PATH "Path to FreeImage")

set(Clang_DIR "" CACHE PATH "Path to Clang")
set(LLVM_DIR "" CACHE PATH "Path to LLVM")

set(BUILD_EXL_OGL ON CACHE BOOL "Build the opengl renderer")
set(BUILD_EXL_SHARED ON CACHE BOOL "Build library as dlls")
set(BUILD_FOR_UE4 OFF CACHE BOOL "Make a dll compatible with UE4")

set(BULLET_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/modules/bullet/src")

set(SDL_ROOT "${CMAKE_SOURCE_DIR}/modules/sdl")
set(IMGUI_ROOT "${CMAKE_SOURCE_DIR}/modules/imgui")
set(XXHASH_ROOT "${CMAKE_SOURCE_DIR}/modules/xxHash")
set(LIBB64_ROOT "${CMAKE_SOURCE_DIR}/contrib/libb64-1.2.1")
set(VOROPP_ROOT "${CMAKE_SOURCE_DIR}/contrib/voro++-0.4.6")
set(LBFGS_ROOT "${CMAKE_SOURCE_DIR}/modules/lbfgs")
set(LIBSVM_ROOT "${CMAKE_SOURCE_DIR}/modules/libsvm")

set(BOOST_COMPONENTS_LIST COMPONENTS)

if(${ANDROID})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fno-rtti")
add_compile_options(-Wno-undefined-var-template)
add_compile_options(-Wno-inconsistent-missing-override)
endif()

if(MSVC) 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR- /we4541 /std:c++17")
endif()


set(BOOST_COMPONENTS_LIST ${BOOST_COMPONENTS_LIST} filesystem program_options)

set(EXL_COMPILER_DEFINITIONS)
set(EXL_DEPS_INCLUDE)

set(BOOST_COMPONENTS_LIST ${BOOST_COMPONENTS_LIST}
    system 
    thread 
    date_time 
  ) 
  
set(EXL_COMPILER_DEFINITIONS ${EXL_COMPILER_DEFINITIONS} -DEXL_THREADAWARE)
set(EXL_COMPILER_DEFINITIONS ${EXL_COMPILER_DEFINITIONS} -DEXL_TYPE_ENABLED)
set(LUABIND_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/core/lua/luabind)
set(EXL_COMPILER_DEFINITIONS ${EXL_COMPILER_DEFINITIONS} -DEXL_LUA)
set(EXL_COMPILER_DEFINITIONS ${EXL_COMPILER_DEFINITIONS} -DLUABIND_DYNAMIC_LINK )

#if(${BUILD_TESTS})
#  SET(GTEST_INCLUDE_DIR ${GTEST_ROOT}/include)
#  SET(GTEST_LIBRARY_DIR ${GTEST_ROOT}/lib)
#  SET(GTEST_LIBRARIES optimized gtest optimized gtest_main debug gtestd debug gtest_maind)
#endif()

if(NOT FREEIMAGE_ROOT STREQUAL "")

  SET(ENABLE_FREEIMAGE ON)
  SET(FREEIMAGE_INCLUDE_DIR ${FREEIMAGE_ROOT}/include)
  SET(FREEIMAGE_LIBRARY_DIR ${FREEIMAGE_ROOT}/lib)
  SET(FREEIMAGE_LIBRARIES FreeImage)

  set(EXL_COMPILER_DEFINITIONS ${EXL_COMPILER_DEFINITIONS} -DEXL_IMAGESTREAMER_ENABLED)

endif()

if(${BUILD_EXL_OGL})
  
	if(${ANDROID})
		SET(OGL_LIBRARIES GLESv2 GLESv3)
	endif()
	
	if(${WIN32})
		SET(OGL_LIBRARIES glew32 opengl32)
		SET(GLEW_INCLUDE_DIR ${GLEW_ROOT}/include)
    
		if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
			SET(GLEW_LIBRARY_DIR ${GLEW_ROOT}/lib/Release/x64)
		else(CMAKE_SIZEOF_VOID_P EQUAL 8)
			SET(GLEW_LIBRARY_DIR ${GLEW_ROOT}/lib/Release/Win32)
		endif()

		SET(OGL_LIBRARY_DIR ${GLEW_LIBRARY_DIR})
		SET(OGL_LIBRARIES ${OGL_LIBRARIES} glew32.lib)
		
	endif()

	set(EXL_COMPILER_DEFINITIONS ${EXL_COMPILER_DEFINITIONS} -DEXL_WITH_OGL)
	
endif()

list(LENGTH BOOST_COMPONENTS_LIST NUM_BOOST_COMPS)

if(${NUM_BOOST_COMPS} GREATER 1)
  list(REMOVE_DUPLICATES BOOST_COMPONENTS_LIST)  
else()
  unset(BOOST_COMPONENTS_LIST)
endif()

if(${ANDROID})  
  SET(Boost_INCLUDE_DIR ${Boost_ROOT})
  
if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
  SET(Boost_LIBRARY_DIR ${Boost_ROOT}/stage_arm_r20/lib)
  SET(Boost_LIBRARIES 
  boost_system-mt-a32 
  boost_date_time-mt-a32 
  boost_thread-mt-a32 
  #boost_filesystem-mt-a32
  )
else()
  SET(Boost_LIBRARY_DIR ${Boost_ROOT}/stage/lib)
  SET(Boost_LIBRARIES 
  boost_system-clang-mt-a64-1_70
  boost_date_time-clang-mt-a64-1_70
  boost_thread-clang-mt-a64-1_70
  #boost_filesystem-mt-a32
  )
endif()

link_directories(${Boost_LIBRARY_DIR})
  
else()
  SET(Boost_USE_STATIC_LIBS TRUE)
  find_package(Boost 1.47.0 
    ${BOOST_COMPONENTS_LIST}
  )
endif()

set(MAIN_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(OUT_DIR ${CMAKE_CURRENT_BINARY_DIR})

set(EXL_DEPS_INCLUDE ${EXL_DEPS_INCLUDE} ${CMAKE_SOURCE_DIR}/include
  ${Boost_INCLUDE_DIR}
)

set (LUA_ROOT ${CMAKE_SOURCE_DIR}/modules/lua)
set (LUA_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/modules/lua)
set (LUA_LIBRARIES lua)

set(EXL_DEPS_INCLUDE ${EXL_DEPS_INCLUDE}
  ${LUA_INCLUDE_DIR} 
  ${CMAKE_CURRENT_SOURCE_DIR}/include/core/lua
)

SET(SDL_INCLUDE_DIR ${SDL_ROOT}/include)
SET(SDL_LIBRARY_DIR ${SDL_ROOT}/include)

set(EXECUTABLE_OUTPUT_PATH ${OUT_DIR})
set(LIBRARY_OUTPUT_PATH ${OUT_DIR})

if(${BUILD_EXL_SHARED})
  SET(LIBRARY_TYPE SHARED)
  SET(LIBRARY_DEPS_VISIBILITY PRIVATE)
  set(EXL_COMPILER_DEFINITIONS ${EXL_COMPILER_DEFINITIONS} -DEXL_SHARED_LIBRARY)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4251 /wd4275")
else()
  SET(LIBRARY_TYPE STATIC)
  SET(LIBRARY_DEPS_VISIBILITY PUBLIC)
endif()
 
 set(EXL_COMPILER_SYS_DEFINITIONS)

 if(MSVC)
  set(EXL_COMPILER_SYS_DEFINITIONS ${EXL_COMPILER_SYS_DEFINITIONS} -DMSVC_COMPILER)
 endif()
 
 set(EXL_COMPILER_SYS_DEFINITIONS ${EXL_COMPILER_SYS_DEFINITIONS} -DBOOST_ALL_NO_LIB)

 if(NOT ${Boost_USE_STATIC_LIBS})
  set(EXL_COMPILER_SYS_DEFINITIONS ${EXL_COMPILER_SYS_DEFINITIONS}
    -DBOOST_ALL_DYN_LINK
  )
 endif()

set(CMAKE_DEBUG_POSTFIX _d)

# Flags matching UE's flags
if(BUILD_FOR_UE4)
  add_definitions(-D_ITERATOR_DEBUG_LEVEL=0)
endif()

set(EXL_COMPILER_DEFINITIONS ${EXL_COMPILER_DEFINITIONS}
  -DBOOST_DISABLE_ABI_HEADERS
  -DTRACE_LEAKS
  -DNOMINMAX
)

add_subdirectory(${SDL_ROOT} ${OUT_DIR}/modules/sdl)
add_subdirectory(${LIBB64_ROOT} ${OUT_DIR}/modules/libb64)
add_subdirectory(${VOROPP_ROOT} ${OUT_DIR}/modules/voropp)
add_subdirectory(${LBFGS_ROOT} ${OUT_DIR}/modules/lbfgs)
add_subdirectory(${CMAKE_SOURCE_DIR}/contrib/libsvm ${OUT_DIR}/modules/libsvm)
set(XXHASH_BUILD_XXHSUM OFF CACHE BOOL "Build the xxhsum binary" FORCE)
add_subdirectory(${XXHASH_ROOT}/cmake_unofficial ${OUT_DIR}/modules/xxhash)
add_subdirectory(${CMAKE_SOURCE_DIR}/contrib/lua ${OUT_DIR}/modules/lua)
add_subdirectory(${CMAKE_SOURCE_DIR}/contrib/imgui ${OUT_DIR}/modules/imgui)
add_subdirectory(${CMAKE_SOURCE_DIR}/contrib/bullet ${OUT_DIR}/contrib/bullet)

if(${WIN32})
SET(LLVM_DIR "D:/MSVC14Libs/llvm/lib/cmake/llvm" CACHE PATH "Path to llvm")
SET(Clang_DIR "D:/MSVC14Libs/clang/lib/cmake/clang" CACHE PATH "Path to libclang")
else()
SET(REFLANG_PATH "D:/eXlProject_Build/RelWithDebInfo/eXl_reflang.exe" CACHE PATH "Path to eXl_reflang")
endif(${WIN32})

SET(IMGUI_INCLUDE_DIR ${IMGUI_ROOT})
SET(XXHASH_INCLUDE_DIR ${XXHASH_ROOT})
SET(LIBB64_INCLUDE_DIR ${LIBB64_ROOT}/include)
SET(LBFGS_INCLUDE_DIR ${LBFGS_ROOT}/include)
SET(LIBSVM_INCLUDE_DIR ${LIBSVM_ROOT})
SET(VOROPP_INCLUDE_DIR ${VOROPP_ROOT}/src)

add_subdirectory(src/core ${OUT_DIR}/eXl/core)
add_subdirectory(src/math ${OUT_DIR}/eXl/math)
add_subdirectory(src/gen ${OUT_DIR}/eXl/gen)

if(${BUILD_EXL_OGL})
  add_subdirectory(src/ogl ${OUT_DIR}/eXl/ogl)
endif()

add_subdirectory(src/engine ${OUT_DIR}/eXl/engine)
