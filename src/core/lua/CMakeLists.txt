
if(${BUILD_EXL_SHARED})
	add_definitions(-DLUABIND_DYNAMIC_LINK)
endif()

add_subdirectory(luabind)