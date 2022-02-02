include_guard(GLOBAL)

function(filter_deps_folder name filterRes)
    set(${filterRes} ON PARENT_SCOPE)
    get_target_property(TARGET_SOURCE_DIR ${name} SOURCE_DIR)

    if(${TARGET_SOURCE_DIR} MATCHES "${DEPS_ROOT_DIR}/modules/*")
        return()
    endif()

    if(${TARGET_SOURCE_DIR} MATCHES "${DEPS_ROOT_DIR}/package/*")
        return()
    endif()

    if(${TARGET_SOURCE_DIR} MATCHES "${DEPS_ROOT_DIR}/config/*")
        return()
    endif()

    if(${TARGET_SOURCE_DIR} MATCHES "${DEPS_ROOT_DIR}/contrib/*")
        return()
    endif()
    
    set(${filterRes} OFF PARENT_SCOPE)
  endfunction()

  function (add_library name)
    _add_library (${ARGV})
    
    filter_deps_folder(${name} is_filtered)

    if(NOT ${is_filtered})
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

    filter_deps_folder(${name} is_filtered)

    if(NOT ${is_filtered})
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

  function (add_custom_target name)
    _add_custom_target (${ARGV})

    filter_deps_folder(${name} is_filtered)

    if(NOT ${is_filtered})
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


function (add_exl_dependencies)

  SET(DEPS_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  
  foreach(project targetFolder IN ZIP_LISTS EXL_SUBMODULE_PROJECTS EXL_SUBMODULE_TARGETS)
    add_subdirectory(${project} ${targetFolder})
  endforeach()

endfunction()
