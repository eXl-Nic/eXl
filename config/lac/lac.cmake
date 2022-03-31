if(${EXL_BUILD_EDITOR})

#set(LAC_INSTALL_DIR ${EXL_DEPENDENCIES_FOLDER}/lac)
#set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${LAC_INSTALL_DIR}/cmake)
#find_package(LuaAutoComplete)
set(LAC_ROOT ${EXL_ROOT}/modules/lac)
set(LAC_INCLUDE_DIRS ${LAC_ROOT}/libs ${LAC_ROOT}/libs/editor)
add_subdirectory(${EXL_ROOT}/config/lac)

#ExternalProject_Add(LuaAutoComplete 
#  SOURCE_DIR ${EXL_ROOT}/modules/lac
#  INSTALL_DIR ${LAC_INSTALL_DIR}
#  STEP_TARGETS install
#  CMAKE_ARGS 
#    -DWITH_NLOHMANN_JSON=OFF 
#    -DBUILD_SHARED_LIBS=${EXL_BUILD_SHARED} 
#    -DBOOST_ROOT=${Boost_ROOT}
#    -DQt5_DIR=${Qt5_DIR}
#    -DCMAKE_INSTALL_PREFIX=${LAC_INSTALL_DIR}
#)


endif()