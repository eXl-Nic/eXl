
if(${WIN32})
  SET(Clang_ROOT "${EXL_ROOT}/package/libclang")
endif()

if(${ANDROID})
  SET(REFLANG_PATH "D:/eXlProject_Build/RelWithDebInfo/eXl_reflang.exe" CACHE PATH "Path to eXl_reflang")
endif()