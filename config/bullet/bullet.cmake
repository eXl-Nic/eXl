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

add_subdirectory(${BULLET_ROOT} ${OUT_DIR}/modules/bullet)

set(EXECUTABLE_OUTPUT_PATH ${OUT_DIR} CACHE PATH "Output Dir" FORCE)
set(LIBRARY_OUTPUT_PATH ${OUT_DIR} CACHE PATH "Output Dir" FORCE)

set_target_properties(Bullet3Common PROPERTIES FOLDER Dependencies/Bullet)
set_target_properties(BulletCollision PROPERTIES FOLDER Dependencies/Bullet)
set_target_properties(BulletDynamics PROPERTIES FOLDER Dependencies/Bullet)
set_target_properties(BulletInverseDynamics PROPERTIES FOLDER Dependencies/Bullet)
set_target_properties(BulletInverseDynamicsUtils PROPERTIES FOLDER Dependencies/Bullet)
set_target_properties(BulletSoftBody PROPERTIES FOLDER Dependencies/Bullet)
set_target_properties(ConvexDecomposition PROPERTIES FOLDER Dependencies/Bullet)
set_target_properties(GIMPACTUtils PROPERTIES FOLDER Dependencies/Bullet)
set_target_properties(HACD PROPERTIES FOLDER Dependencies/Bullet)
set_target_properties(LinearMath PROPERTIES FOLDER Dependencies/Bullet)

set(BULLET_INCLUDE_DIR "${BULLET_ROOT}/src")