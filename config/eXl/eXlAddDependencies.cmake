function (add_exl_dependencies)

  set(EXL_ADDING_DEPENDENCIES ON)

  function (add_library name)
    _add_library (${ARGV})

    if(NOT ${EXL_ADDING_DEPENDENCIES})
        return()
    endif()

    if(${name} MATCHES "eXl_*")
        return()
    endif()

    if (type STREQUAL INTERFACE)
      return()
    endif()
    get_target_property(_aliased ${name} ALIASED_TARGET)

    if(_aliased)
        return()
    endif()
    set_target_properties(${name}
        PROPERTIES
        FOLDER "Dependencies"
    )
  endfunction()

  function (add_executable name)
    _add_executable (${ARGV})

    if(NOT ${EXL_ADDING_DEPENDENCIES})
        return()
    endif()
    
    if(${name} MATCHES "eXl_*")
        return()
    endif()

    get_target_property(_aliased ${name} ALIASED_TARGET)
    if(_aliased)
        return()
    endif()
    set_target_properties(${name}
        PROPERTIES
        FOLDER "Dependencies"
    )
  endfunction()

  foreach(project targetFolder IN ZIP_LISTS EXL_SUBMODULE_PROJECTS EXL_SUBMODULE_TARGETS)
    add_subdirectory(${project} ${targetFolder})
  endforeach()
  
  set(EXL_ADDING_DEPENDENCIES OFF)
endfunction()

add_exl_dependencies()

if(${EXL_BUILD_TESTS})
#Override gtest's opinionated folders to have something working out of the box
    if (TARGET gtest)
        set_target_properties(gtest
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR}
            LIBRARY_OUTPUT_DIRECTORY ${OUT_DIR}
            ARCHIVE_OUTPUT_DIRECTORY ${OUT_DIR}
            PDB_OUTPUT_DIRECTORY ${OUT_DIR})
    endif()

    if (TARGET gtest_main)
        set_target_properties(gtest_main
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR}
            LIBRARY_OUTPUT_DIRECTORY ${OUT_DIR}
            ARCHIVE_OUTPUT_DIRECTORY ${OUT_DIR}
            PDB_OUTPUT_DIRECTORY ${OUT_DIR})
    endif()
endif()