set(INCLUDE_DIR ${EXL_ROOT}/include/engine)

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
SOURCE_GROUP("GUI\\Src" REGULAR_EXPRESSION "gui/.*cpp")
SOURCE_GROUP("GUI\\Header" REGULAR_EXPRESSION "gui/.*(hpp|inl)")

FILE(GLOB_RECURSE EXL_ENGINE_HEADER_FILES LIST_DIRECTORIES false ${INCLUDE_DIR}/*.hpp ${INCLUDE_DIR}/*.inl)

if(NOT ${EXL_BUILD_SHARED})
add_definitions(-DIMGUI_API=)
endif()

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

SET(GFX_SRC

gfx/gfxsystem.cpp
gfx/gfxcomponent.cpp
gfx/gfxdebugdrawer.cpp
gfx/gfxcomponentrendernode.cpp
gfx/gfxspriterendernode.cpp
gfx/gfxguirendernode.cpp
)

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
common/gamedata.cpp
common/gamedatabase.cpp

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
map/mcmcmodelrsc.cpp

net/network.cpp
net/network_internal.hpp
net/yojimbo/network_yojimbo.hpp
net/yojimbo/streamer_yojimbo.cpp
net/yojimbo/streamer_yojimbo.hpp
net/yojimbo/client_yojimbo.cpp
net/yojimbo/server_yojimbo.cpp
net/yojimbo/protocol_yojimbo.cpp
net/dispatcher.cpp

gui/fontresource.cpp

engineplugin.cpp

${CMAKE_CURRENT_BINARY_DIR}/engine_gen.cpp
${CMAKE_CURRENT_BINARY_DIR}/engine_gen.hpp
)

target_include_directories(eXl_Engine PRIVATE 
  ${BULLET_INCLUDE_DIR} 
  ${IMGUI_INCLUDE_DIR} 
  ${LIBB64_INCLUDE_DIR} 
  ${FREETYPE_INCLUDE_DIR}
  ${UTF8_INCLUDE_DIR}
  ${YOJIMBO_INCLUDE_DIR}
  ${EXL_ROOT}/modules/cxxopts/include)

target_include_directories(eXl_Engine PUBLIC  
  ${IMGUI_INCLUDE_DIR} 
  )

set(HEADERS_TO_PARSE "${GAME_HEADERS}" 
${INCLUDE_DIR}/map/tilinggroup.hpp
${INCLUDE_DIR}/gfx/gfxcomponent.hpp
${INCLUDE_DIR}/gfx/tileset.hpp
${INCLUDE_DIR}/map/map.hpp
${INCLUDE_DIR}/common/transforms.hpp
${INCLUDE_DIR}/physics/physicsdef.hpp)

SET (EXL_DEPENDENCIES eXl_Gen eXl_OGL)


SETUP_EXL_LIB(eXl_Engine "ENGINE" DEPENDENCIES ${EXL_DEPENDENCIES} HEADERS_TO_PARSE "${HEADERS_TO_PARSE}")

set( PREREQS_PRIV
  BulletDynamics
  BulletCollision
  LinearMath
  libb64
  freetype
  yojimbo
)

if(${EXL_BUILD_OGL})
set( PREREQS_PRIV
  ${PREREQS_PRIV}
  imgui
  ${OGL_LIBRARIES}
  )
endif()

if(${WIN32})
set( PREREQS_PRIV
  ${PREREQS_PRIV}
  ${SPATIAL_LIBRARIES}
  bcrypt
  )
endif()

target_link_libraries(eXl_Engine LINK_PRIVATE ${PREREQS_PRIV})

set( MAIN_SOURCES
	eXl_main.cpp 
	console.cpp
	debugviews.cpp
	sdlkeytranslator.cpp
    navigatorbench.cpp
    ${EXL_ROOT}/modules/imgui/backends/imgui_impl_sdl.cpp
)

add_library(eXl_Main ${LIBRARY_TYPE} ${MAIN_SOURCES})

target_include_directories(eXl_Main PRIVATE ${SDL_INCLUDE_DIR} ${IMGUI_INCLUDE_DIR} ${GLEW_INCLUDE_DIR})
target_link_directories(eXl_Main PRIVATE ${GLEW_LIBRARY_DIR})
SETUP_EXL_LIB(eXl_Main "MAIN" DEPENDENCIES eXl_Engine)

set(PRIVATE_LIBRARIES
	imgui
)

if(${MSVC})
	set( PRIVATE_LIBRARIES  
	${PRIVATE_LIBRARIES}
	legacy_stdio_definitions 
	${OGL_LIBRARIES} 
	SDL2)
endif()

if(${ANDROID})
set( PRIVATE_LIBRARIES  
	${PRIVATE_LIBRARIES}
	 SDL2-static)
endif()

target_link_libraries(eXl_Main PRIVATE ${PRIVATE_LIBRARIES})

if(${EXL_BUILD_EDITOR})
add_subdirectory(editor ${OUT_DIR}/eXl/editor)
endif()

add_executable(eXl_Player main_player.cpp)
SETUP_EXL_TARGET(eXl_Player DEPENDENCIES eXl_Main)

if(${EXL_BUILD_TESTS})
add_subdirectory(test)
endif()
