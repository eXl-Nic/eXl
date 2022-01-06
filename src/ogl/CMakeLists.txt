#project(eXl_OGL)

set(INCLUDE_DIR ${EXL_ROOT}/include/ogl)

FILE(GLOB_RECURSE EXL_OGL_HEADER_FILES LIST_DIRECTORIES false ${INCLUDE_DIR}/*.hpp ${INCLUDE_DIR}/*.inl)

SET(SOURCES

${EXL_OGL_HEADER_FILES}

oglspritealgo.cpp
oglutils.cpp
oglplugin.cpp

renderer/oglrendercommand.cpp
renderer/oglcompiledprogram.cpp
renderer/oglrendercontext.cpp
renderer/oglframebuffer.cpp
renderer/ogltexture.cpp
renderer/oglprogram.cpp
renderer/oglshaderdata.cpp
renderer/oglbuffer.cpp
renderer/ogltextureloader.cpp
renderer/oglsemanticmanager.cpp
renderer/ogldisplaylist.cpp
renderer/ogltypesconv.cpp

${CMAKE_CURRENT_BINARY_DIR}/ogl_gen.cpp
${CMAKE_CURRENT_BINARY_DIR}/ogl_gen.hpp
)

SET(PREREQS_PRIV 
    ${PREREQS_PRIV}
    ${OGL_LIBRARIES}
  )

add_library(eXl_OGL ${LIBRARY_TYPE}
  ${SOURCES}
)

if(${WIN32})

  target_include_directories(eXl_OGL PRIVATE ${GLEW_INCLUDE_DIR})
  target_link_directories(eXl_OGL PRIVATE ${GLEW_LIBRARY_DIR})

endif()

set (REFLECTED_HEADERS ${INCLUDE_DIR}/oglspritealgo.hpp)

SETUP_EXL_LIB(eXl_OGL "OGL" DEPENDENCIES eXl_Core eXl_Math HEADERS_TO_PARSE ${REFLECTED_HEADERS})

target_link_libraries(eXl_OGL LINK_PRIVATE ${PREREQS_PRIV})
if(${ANDROID})
else()
if(${EXL_BUILD_OGL})
add_dependencies(eXl_OGL glew)
endif()
endif()