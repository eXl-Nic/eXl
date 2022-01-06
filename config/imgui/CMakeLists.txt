project(libImGui)

SET(IMGUI_DIR ${EXL_ROOT}/modules/imgui)

include_directories(${IMGUI_DIR})

SET(IMGUI_SOURCES  
  ${IMGUI_DIR}/imgui.cpp 
  ${IMGUI_DIR}/imgui_demo.cpp 
  ${IMGUI_DIR}/imgui_draw.cpp 
  ${IMGUI_DIR}/imgui_tables.cpp 
  ${IMGUI_DIR}/imgui_widgets.cpp 
)


if(${ANDROID})
SET(IMGUI_SOURCES ${IMGUI_SOURCES} ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
else()
	add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLEW)
  
	SET(IMGUI_SOURCES ${IMGUI_SOURCES} ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp ${IMGUI_DIR}/backends/imgui_impl_opengl2.cpp)

	include_directories(${GLEW_INCLUDE_DIR})
	link_directories(${GLEW_LIBRARY_DIR})
endif()

SET(PREREQS_PRIV ${OGL_LIBRARIES})

if(${EXL_BUILD_SHARED})
add_definitions(-DBUILD_IMGUI)
add_definitions("-DIMGUI_API=__declspec(dllexport)")
else()
add_definitions(-DIMGUI_API=)
endif()

add_library(imgui ${LIBRARY_TYPE} ${IMGUI_SOURCES})
target_link_libraries(imgui LINK_PRIVATE ${PREREQS_PRIV})

if(${ANDROID})
else()
add_dependencies(imgui glew)
endif()

if(${LINUX})
  SET_TARGET_PROPERTIES(imgui PROPERTIES COMPILE_FLAGS -fPIC)
endif(${LINUX})

