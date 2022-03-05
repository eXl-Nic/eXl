cmake_minimum_required(VERSION 3.13)

project(eXl_Dependencies)

set(EXL_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

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

include(${EXL_ROOT}/config/lac/lac_build.cmake)

