set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET3 OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET_ROBOTICS_EXTRA OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET_ROBOTICS_GUI_EXTRA OFF CACHE BOOL "" FORCE)
set(BUILD_CLSOCKET OFF CACHE BOOL "" FORCE)
set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_ENET OFF CACHE BOOL "" FORCE)
set(BUILD_OBJ2SDF_EXTRA OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_SERIALIZE_EXTRA OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS_EXTRA OFF CACHE BOOL "" FORCE)
set(USE_GLUT OFF CACHE BOOL "" FORCE)
set(USE_GRAPHICAL_BENCHMARK OFF CACHE BOOL "" FORCE)
set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(EXL_SUBMODULE_PROJECTS ${EXL_SUBMODULE_PROJECTS} ${BULLET_ROOT})
set(EXL_SUBMODULE_TARGETS ${EXL_SUBMODULE_TARGETS} ${OUT_DIR}/modules/bullet)

set(EXECUTABLE_OUTPUT_PATH ${OUT_DIR} CACHE PATH "Output Dir" FORCE)
set(LIBRARY_OUTPUT_PATH ${OUT_DIR} CACHE PATH "Output Dir" FORCE)

set(BULLET_INCLUDE_DIR "${BULLET_ROOT}/src")