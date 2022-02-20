cmake_minimum_required(VERSION 3.13)

project(eXl)

set(ANDROID_PLATFORM 19)

set(EXL_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(ARMADILLO_ROOT "" CACHE PATH "Path to armadillo root directory")

set(EXL_BUILD_SHARED ON CACHE BOOL "Build library as dlls")
set(EXL_BUILD_OGL ON CACHE BOOL "Build the opengl renderer")
set(EXL_BUILD_ENGINE ON CACHE BOOL "Build the eXl engine")
set(EXL_BUILD_EDITOR ON CACHE BOOL "Build the eXl editor")
set(EXL_BUILD_WITH_LUA ON CACHE BOOL "Enable eXl lua integration")
set(EXL_BUILD_FOR_UE4 OFF CACHE BOOL "Make a dll compatible with UE4")
set(EXL_BUILD_WITH_FREEIMAGE ON CACHE BOOL "Enable eXl Freeimage integration")

set(EXL_BUILD_TESTS OFF CACHE BOOL "Build eXl tests")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(${WIN32})
  set(Clang_ROOT "${EXL_ROOT}/package/libclang" CACHE PATH "Path to libclang")
endif()
set(Boost_ROOT "${EXL_ROOT}/package/boost/boost_1_78_0" CACHE PATH "Path to boost")
set(FREEIMAGE_ROOT "${EXL_ROOT}/package/freeimage/FreeImage" CACHE PATH "Path to FreeImage")
set(GLEW_ROOT "${EXL_ROOT}/package/glew/glew-2.2.0" CACHE PATH "Path to Glew")
set(BULLET_ROOT "${EXL_ROOT}/modules/bullet")
set(SDL_ROOT "${EXL_ROOT}/modules/sdl")
set(IMGUI_ROOT "${EXL_ROOT}/modules/imgui")
set(XXHASH_ROOT "${EXL_ROOT}/modules/xxHash")
set(LIBB64_ROOT "${EXL_ROOT}/contrib/libb64-1.2.1")
set(VOROPP_ROOT "${EXL_ROOT}/contrib/voro++-0.4.6")
set(LBFGS_ROOT "${EXL_ROOT}/modules/lbfgs")
set(LIBSVM_ROOT "${EXL_ROOT}/modules/libsvm")
set(FREETYPE_ROOT "${EXL_ROOT}/modules/freetype")
set(LAYOUT_ROOT "${EXL_ROOT}/modules/layout")
set(GTEST_ROOT "${EXL_ROOT}/modules/gtest")
set(UTF8_ROOT "${EXL_ROOT}/modules/utf8")

include(${EXL_ROOT}/config/eXl/eXlconfig_min.cmake)
include(${EXL_ROOT}/config/eXl/eXlSetupBuildPath.cmake)
include(${EXL_ROOT}/config/eXl/eXlconfig.cmake)
include(${EXL_ROOT}/config/eXl/eXlMacros.cmake)
include(${EXL_ROOT}/config/eXl/eXlAddDependencies.cmake)

add_exl_dependencies()

if(${EXL_BUILD_TESTS})
    #Override gtest's opinionated folders to have something working out of the box
    if (TARGET gtest)
        set_target_properties(gtest
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR}
            LIBRARY_OUTPUT_DIRECTORY ${OUT_DIR}
            ARCHIVE_OUTPUT_DIRECTORY ${OUT_DIR}
            PDB_OUTPUT_DIRECTORY ${OUT_DIR})
    endif()

    if (TARGET gtest_main)
        set_target_properties(gtest_main
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR}
            LIBRARY_OUTPUT_DIRECTORY ${OUT_DIR}
            ARCHIVE_OUTPUT_DIRECTORY ${OUT_DIR}
            PDB_OUTPUT_DIRECTORY ${OUT_DIR})
    endif()
endif()

SET(LAYOUT_INCLUDE_DIR ${LAYOUT_ROOT})

add_subdirectory(src/core ${OUT_DIR}/eXl/core)
add_subdirectory(src/math ${OUT_DIR}/eXl/math)
add_subdirectory(src/gen ${OUT_DIR}/eXl/gen)

add_subdirectory(src/ogl ${OUT_DIR}/eXl/ogl)

if(${EXL_BUILD_ENGINE})
  add_subdirectory(src/engine ${OUT_DIR}/eXl/engine)
endif()
