
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -stdlib=libc++")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	set(LLVM_CONFIG true)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(Clang REQUIRED)
include_directories(${CLANG_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/modules/cxxopts/include)

set_target_properties(acc_gen PROPERTIES FOLDER Dependencies/Clang)
set_target_properties(clang-tablegen-targets PROPERTIES FOLDER Dependencies/Clang)
set_target_properties(intrinsics_gen PROPERTIES FOLDER Dependencies/Clang)
set_target_properties(omp_gen PROPERTIES FOLDER Dependencies/Clang)

set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")
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
