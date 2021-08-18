function(ADD_EXL_COMMON_DEFS TARGET_NAME)
  target_compile_options(${TARGET_NAME} PRIVATE ${EXL_COMPILER_DEFINITIONS} ${EXL_COMPILER_SYS_DEFINITIONS})
	target_include_directories(${TARGET_NAME} PUBLIC ${EXL_DEPS_INCLUDE})
 endfunction()

function(ADD_EXL_LIB_PROPERTIES TARGET_NAME LIB_SUFFIX)
  if(${BUILD_EXL_SHARED})
		target_compile_options(${TARGET_NAME} PRIVATE -DBUILD_${LIB_SUFFIX}_DLL -DPLUGIN_NAME=${TARGET_NAME})
	endif()
	if(${WIN32})
	set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME ${TARGET_NAME} PREFIX "" ENABLE_EXPORTS 1)
	endif()
endfunction()

function(SETUP_EXL_TARGET TARGET_NAME)

  set(options)
  set(oneValueArgs LIB_SUFFIX)
  set(multiValueArgs DEPENDENCIES HEADERS_TO_PARSE)
  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
  
  ADD_EXL_COMMON_DEFS(${TARGET_NAME})

  if(ARGS_DEPENDENCIES)
    target_link_libraries(${TARGET_NAME} LINK_PUBLIC ${ARGS_DEPENDENCIES})
  endif()
  
	if(ARGS_HEADERS_TO_PARSE)
		set(REFLANG_INCLUDES "")
    
    if(NOT ARGS_LIB_SUFFIX)
      message(ERROR "Lib suffix missing for code gen")
    endif()

		foreach(DIRECTORY_ITEM ${EXL_DEPS_INCLUDE})
			set(REFLANG_INCLUDES "${REFLANG_INCLUDES} --include-directory=\"${DIRECTORY_ITEM}\"")
		endforeach()

		set(REFLECTION_TARGET ${TARGET_NAME}_Reflection)
    
    if(${WIN32})
		set(REFLANG_PATH $<TARGET_FILE:eXl_reflang>)
    endif(${WIN32})
    
    string(TOLOWER ${LIB_SUFFIX} LIB_SUFFIX_FILENAME)

    set(REFLANG_OUTPUT ${LIB_SUFFIX_FILENAME}_gen)
    set(REFLANG_OUTPUT_HPP ${CMAKE_CURRENT_BINARY_DIR}/${LIB_SUFFIX_FILENAME}_gen.hpp)
    set(REFLANG_OUTPUT_CPP ${CMAKE_CURRENT_BINARY_DIR}/${LIB_SUFFIX_FILENAME}_gen.cpp)

    set(REFLANG_COMMAND ${REFLANG_PATH} --out-hpp ${REFLANG_OUTPUT_HPP} --out-cpp ${REFLANG_OUTPUT_CPP} --internal-name ${ARGS_LIB_SUFFIX} ${ARGS_HEADERS_TO_PARSE}
	-- ${REFLANG_INCLUDES} -DEXL_REFLANG_COMPILER -std=c++17 -Wno-undefined-var-template -Wno-inconsistent-missing-override)

    set (DEPENDENCIES ${ARGS_HEADERS_TO_PARSE})
    if(${WIN32})
    set (DEPENDENCIES ${DEPENDENCIES} eXl_reflang)
    endif(${WIN32})

    add_custom_command(OUTPUT ${REFLANG_OUTPUT_CPP}
          COMMAND ${REFLANG_COMMAND}
          DEPENDS ${DEPENDENCIES} 
          BYPRODUCTS ${REFLANG_OUTPUT_HPP}
    )
    
    #add_custom_target(${TARGET_NAME}_outputdebugcmd COMMAND ${CMAKE_COMMAND} -E echo ${REFLANG_COMMAND})
    if(${WIN32})
      add_dependencies(${TARGET_NAME} eXl_reflang)
    endif(${WIN32})
    #add_dependencies(${TARGET_NAME} ${TARGET_NAME}_outputdebugcmd)
    
    target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

	endif()

endfunction()


function(SETUP_EXL_LIB TARGET_NAME LIB_SUFFIX)

  set(options)
  set(oneValueArgs)
  set(multiValueArgs DEPENDENCIES HEADERS_TO_PARSE)
  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

	SETUP_EXL_TARGET(${TARGET_NAME} DEPENDENCIES ${ARGS_DEPENDENCIES} HEADERS_TO_PARSE ${ARGS_HEADERS_TO_PARSE} LIB_SUFFIX ${LIB_SUFFIX})

	ADD_EXL_LIB_PROPERTIES(${TARGET_NAME} ${LIB_SUFFIX})

endfunction()