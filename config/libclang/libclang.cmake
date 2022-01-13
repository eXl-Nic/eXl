
if(${WIN32})
  SET(CLANG_INCLUDE_DIRS ${Clang_ROOT}/include)
  SET(CLANG_LIBRARY_DIRS ${Clang_ROOT}/lib)
  SET(CLANG_BINARY_DIR ${Clang_ROOT}/bin)
else()
  find_package(Clang REQUIRED)
endif()