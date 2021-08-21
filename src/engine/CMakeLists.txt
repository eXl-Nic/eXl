set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/engine)

SOURCE_GROUP("Common\\Src" REGULAR_EXPRESSION ".*cpp")
SOURCE_GROUP("Common\\Header" REGULAR_EXPRESSION ".*(hpp|inl)")
SOURCE_GROUP("Gfx\\Src" REGULAR_EXPRESSION "gfx/.*cpp")
SOURCE_GROUP("Gfx\\Header" REGULAR_EXPRESSION "gfx/.*(hpp|inl)")
SOURCE_GROUP("Game\\Src" REGULAR_EXPRESSION "game/.*cpp")
SOURCE_GROUP("Game\\Header" REGULAR_EXPRESSION "game/.*(hpp|inl)")
SOURCE_GROUP("Physics\\Src" REGULAR_EXPRESSION "physics/.*cpp")
SOURCE_GROUP("Physics\\Header" REGULAR_EXPRESSION "physics/.*(hpp|inl)")
SOURCE_GROUP("ImGui\\Src" REGULAR_EXPRESSION "imgui*.*cpp")
SOURCE_GROUP("ImGui\\Header" REGULAR_EXPRESSION "imgui*.*(hpp|inl)")
SOURCE_GROUP("Pathfinding\\Src" REGULAR_EXPRESSION "pathfinding/.*cpp")
SOURCE_GROUP("Pathfinding\\Header" REGULAR_EXPRESSION "pathfinding/.*(hpp|inl)")
SOURCE_GROUP("Network\\Src" REGULAR_EXPRESSION "net/.*cpp")
SOURCE_GROUP("Network\\Header" REGULAR_EXPRESSION "net/.*(hpp|inl)")

FILE(GLOB_RECURSE EXL_ENGINE_HEADER_FILES LIST_DIRECTORIES false ${INCLUDE_DIR}/*.hpp ${INCLUDE_DIR}/*.inl)

if(NOT ${BUILD_EXL_SHARED})
add_definitions(-DIMGUI_API=)
endif()

message(${BULLET_INCLUDE_DIR})

include_directories(${BULLET_INCLUDE_DIR} ${IMGUI_INCLUDE_DIR} ${SDL_INCLUDE_DIR} ${LIBB64_INCLUDE_DIR})

link_directories(${OGL_LIBRARY_DIR} ${OPENCV_LIBRARY_DIR})

set(GAME_HEADERS
${INCLUDE_DIR}/game/projectile.hpp
${INCLUDE_DIR}/game/commondef.hpp
${INCLUDE_DIR}/game/grabability.hpp
${INCLUDE_DIR}/game/pickability.hpp
${INCLUDE_DIR}/game/throwability.hpp
${INCLUDE_DIR}/game/walkability.hpp
${INCLUDE_DIR}/game/swordability.hpp
${INCLUDE_DIR}/game/scripttrigger.hpp
)

SET(GAME_SRC

game/ability.cpp
game/archetype.cpp
game/movementmanager.cpp
game/character.cpp
game/characteranimation.cpp
game/projectile.cpp
game/commondef.cpp
game/grabability.cpp
game/pickability.cpp
game/throwability.cpp
game/walkability.cpp
game/swordability.cpp
game/scenariobase.cpp
game/scripttrigger.cpp
)

SET(GFX_SRC)

if(${BUILD_EXL_OGL})

SET(GFX_SRC

gfx/gfxsystem.cpp
gfx/gfxcomponent.cpp
)

endif()

set (LUA_SRC

script/luascriptsystem.cpp
luabindengine.cpp
)

add_library(eXl_Engine ${LIBRARY_TYPE}

${EXL_ENGINE_HEADER_FILES}

${GAME_SRC}
${LUA_SRC}
${GFX_SRC}

common/app.cpp
common/transformanim.cpp
common/menumanager.cpp
common/project.cpp
common/object.cpp
common/transforms.cpp
common/world.cpp
common/debugtool.cpp

gfx/tileset.cpp

physics/physicsys.cpp
physics/physicsys_impl.cpp
physics/physiccomponent.cpp
physics/physiccomponent_impl.cpp
physics/trigger.cpp

pathfinding/navmesh.cpp
pathfinding/navigator.cpp
pathfinding/penumbratools.cpp
pathfinding/velocityobstacle.cpp
pathfinding/ORCA/ORCAAgent.hpp
pathfinding/ORCA/ORCAAgent.cpp

map/maptiler.cpp
map/dungeongraph.cpp
map/dungeongraph_res.cpp
map/dungeongraph_z.cpp
map/dungeonlayout.cpp
map/tilinggroup.cpp
map/map.cpp
map/map_instantiate.cpp

#net/network.cpp

net/spatial/spatialos_world.hpp
net/spatial/spatialos_world.cpp

engineplugin.cpp

${CMAKE_CURRENT_BINARY_DIR}/engine_gen.cpp
${CMAKE_CURRENT_BINARY_DIR}/engine_gen.hpp
)

set(HEADERS_TO_PARSE "${GAME_HEADERS}" 
${INCLUDE_DIR}/map/tilinggroup.hpp
${INCLUDE_DIR}/gfx/gfxcomponent.hpp
${INCLUDE_DIR}/map/map.hpp
${INCLUDE_DIR}/common/transforms.hpp
${INCLUDE_DIR}/physics/physicsdef.hpp)

SET (EXL_DEPENDENCIES eXl_Gen)

if(${BUILD_EXL_OGL})
SET (EXL_DEPENDENCIES ${EXL_DEPENDENCIES} eXl_OGL)
endif()

SETUP_EXL_LIB(eXl_Engine "ENGINE" DEPENDENCIES ${EXL_DEPENDENCIES} HEADERS_TO_PARSE "${HEADERS_TO_PARSE}")

set( PREREQS_PRIV
  imgui
  ${OGL_LIBRARIES}
  LinearMath
  BulletCollision
  BulletDynamics
  libb64
)

if(${WIN32})
set( PREREQS_PRIV
  ${PREREQS_PRIV}
  ${SPATIAL_LIBRARIES}
  bcrypt
  )
endif()

target_link_libraries(eXl_Engine LINK_PRIVATE ${PREREQS_PRIV})

if(${WIN32})
add_subdirectory(editor ${OUT_DIR}/engine/editor)
endif(${WIN32})

