if(${EXL_BUILD_OGL})
  
	if(${ANDROID})
		SET(OGL_LIBRARIES GLESv2 GLESv3)
	endif()
	
	if(${WIN32})
        include(${EXL_ROOT}/config/glew/glew.cmake)
		SET(OGL_LIBRARIES opengl32 ${GLEW_LIBRARY})
	endif()

	set(EXL_COMPILER_DEFINITIONS ${EXL_COMPILER_DEFINITIONS} -DEXL_WITH_OGL)
	
endif()