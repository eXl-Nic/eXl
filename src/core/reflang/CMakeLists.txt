SET(CLANG_INCLUDE_DIRS ${Clang_ROOT}/include)
SET(CLANG_LIBRARY_DIRS ${Clang_ROOT}/lib)
SET(CLANG_BINARY_DIR ${Clang_ROOT}/bin)

include_directories(${CLANG_INCLUDE_DIRS})
include_directories(${EXL_ROOT}/modules/cxxopts/include)
link_directories(${CLANG_LIBRARY_DIRS})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_executable(
  eXl_reflang
  main.cpp
  parser.class.cpp parser.class.hpp
  parser.cpp parser.hpp
  parser.enum.cpp parser.enum.hpp
  parser.function.cpp parser.function.hpp
  parser.util.cpp parser.util.hpp
  serializer.class.cpp serializer.class.hpp
  serializer.cpp serializer.hpp
  serializer.enum.cpp serializer.enum.hpp
  serializer.function.cpp serializer.function.hpp
  serializer.util.cpp serializer.util.hpp
  types.cpp types.hpp)

ADD_EXL_COMMON_DEFS(eXl_reflang)
target_link_libraries(eXl_reflang eXl_Core libclang)

if(${WIN32})
add_custom_command(
      TARGET eXl_reflang
	  POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CLANG_BINARY_DIR}/libclang.dll $<TARGET_FILE_DIR:eXl_reflang>)
endif()
