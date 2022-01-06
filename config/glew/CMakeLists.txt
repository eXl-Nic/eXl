
if(${MSVC})
SET(MSBUILD_OPTIONS /p:PlatformToolset=v142 /property:Configuration=Release /property:Platform=x64)
if(${EXL_BUILD_SHARED})
SET(GLEW_BUILD_COMMAND "${GLEW_ROOT}/build/vc15/glew_shared.vcxproj")
SET(GLEW_BINARY ${GLEW_ROOT}/bin/${GLEW_VARIANT}/glew32.dll)
else()
SET(GLEW_BUILD_COMMAND "${GLEW_ROOT}/build/vc15/glew_static.vcxproj")
endif()
add_custom_command(OUTPUT ${GLEW_LIBRARY}
          COMMAND ${CMAKE_VS_MSBUILD_COMMAND} ${GLEW_BUILD_COMMAND} ${MSBUILD_OPTIONS}
          BYPRODUCTS ${GLEW_BINARY}
    )
endif()

add_custom_target(glew DEPENDS ${GLEW_LIBRARY})
    
if(${MSVC} AND ${EXL_BUILD_SHARED})
add_custom_command(
      TARGET glew
	  POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GLEW_BINARY} $<TARGET_FILE_DIR:eXl_Core>)
endif()