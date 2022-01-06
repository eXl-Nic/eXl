SET(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/modules/lua)

project(lua C)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_DEBUG_POSTFIX "_d")

include(${CMAKE_MODULE_PATH}/CustomMacros.cmake)

enable_testing()

if(${EXL_BUILD_SHARED})
	set(BUILD_STATIC OFF)
else()
	set(BUILD_STATIC ON)
endif()

option(LUA_USE_DLOPEN "Enable dlopen support.")
option(LUA_USE_ISATTY "Enable isatty support.")
option(LUA_USE_READLINE "Enable readline support.")
option(LUA_USE_POPEN "Enable lua_popen support.")
option(LUA_USE_ULONGJMP "Try using _longjmp/_setjmp (more efficient)")
option(LUA_USE_APICHECK "Enable API checks.")
option(LUA_USE_MKSTEMP "Use mkstemp.")
option(LUA_ANSI "Disable non-ansi features." OFF)

#
# Lua version
#
set(LUA_VERSION_MAJOR 5)
set(LUA_VERSION_MINOR 3)
set(LUA_VERSION_PATCH 5)
set(LUA_VERSION
				"${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}.${LUA_VERSION_PATCH}")
set(LUA_SOVERSION
				"${LUA_VERSION_MAJOR}")
##
## Lua package info.
##
#set(CPACK_PACKAGE_VERSION_MAJOR ${LUA_VERSION_MAJOR})
#set(CPACK_PACKAGE_VERSION_MINOR ${LUA_VERSION_MINOR})
#set(CPACK_PACKAGE_VERSION_PATCH ${LUA_VERSION_PATCH})
#set(CPACK_RESOURCE_FILE_LICENSE "${EXL_ROOT}/COPYRIGHT")
#set(CPACK_RESOURCE_FILE_README "${EXL_ROOT}/README")
#set(CPACK_PACKAGE_VENDOR "Lua.org, PUC-Rio.")
#set(CPACK_SOURCE_GENERATOR "TGZ")
#set(CPACK_SOURCE_IGNORE_FILES
#"/\\\\.;/\\\\.git.*/;~$;build/;CMakeFiles/;CMakeCache;Testing/;cmake_install;CPack;Dart;Makefile$")
#set(CPACK_SOURCE_PACKAGE_FILE_NAME
#	"${CMAKE_PROJECT_NAME}-${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}.${LUA_VERSION_PATCH}")
## MUST be after CPACK_* variables.
#include(CPack)

set(COMMON_CFLAGS)
set(COMMON_LDFLAGS " -lm ")

#
# Detect system type
#
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(LUA_USE_LINUX TRUE)
	set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_USE_LINUX")
	set(COMMON_LDFLAGS "${COMMON_LDFLAGS} -Wl,-E -ldl -lreadline -lhistory -lncurses")
else(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	if(APPLE)
		set(LUA_USE_MACOSX TRUE)
		set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_USE_LINUX")
		set(COMMON_LDFLAGS "${COMMON_LDFLAGS} -lreadline ")
		# use this on Mac OS X 10.3-
		#set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_USE_MACOSX")
	elseif(WIN32)
		set(LUA_WIN TRUE)
		set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_BUILD_AS_DLL")
	elseif(UNIX)
		set(LUA_USE_POSIX TRUE)
		set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_USE_POSIX")
	else(APPLE)
		set(LUA_ANSI TRUE)
		set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_ANSI")
	endif(APPLE)
endif(CMAKE_SYSTEM_NAME STREQUAL "Linux")

#
# standard flags to use for each build type.
#
if(CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -Wall -Wextra -Wshadow -W -pedantic -std=gnu99 -fgnu89-inline")
	set(CMAKE_C_FLAGS_RELEASE        "${CMAKE_C_FLAGS_RELEASE}     -O2")
	set(CMAKE_C_FLAGS_DEBUG          "${CMAKE_C_FLAGS_DEBUG}       -O0 -g")
	set(CMAKE_C_FLAGS_PROFILE        "${CMAKE_C_FLAGS_PROFILE}     -O1 -g")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_WITHDEBINFO} -O2 -g")
endif(CMAKE_COMPILER_IS_GNUCC)

SET(LUA_SRC_DIR ${EXL_ROOT}/modules/lua)

# Lua core source files.
set(LUA_CORE_SRC
	${LUA_SRC_DIR}/lapi.c
	${LUA_SRC_DIR}/lauxlib.c
	${LUA_SRC_DIR}/lbaselib.c
  ${LUA_SRC_DIR}/lbitlib.c
	${LUA_SRC_DIR}/lcode.c
  ${LUA_SRC_DIR}/lcorolib.c
  ${LUA_SRC_DIR}/lctype.c
	${LUA_SRC_DIR}/ldblib.c
	${LUA_SRC_DIR}/ldebug.c
	${LUA_SRC_DIR}/ldo.c
	${LUA_SRC_DIR}/ldump.c
	${LUA_SRC_DIR}/lfunc.c
	${LUA_SRC_DIR}/lgc.c
	${LUA_SRC_DIR}/linit.c
	${LUA_SRC_DIR}/liolib.c
	${LUA_SRC_DIR}/llex.c
	${LUA_SRC_DIR}/lmathlib.c
	${LUA_SRC_DIR}/lmem.c
	${LUA_SRC_DIR}/loadlib.c
	${LUA_SRC_DIR}/lobject.c
	${LUA_SRC_DIR}/lopcodes.c
	${LUA_SRC_DIR}/loslib.c
	${LUA_SRC_DIR}/lparser.c
	${LUA_SRC_DIR}/lstate.c
	${LUA_SRC_DIR}/lstring.c
	${LUA_SRC_DIR}/lstrlib.c
	${LUA_SRC_DIR}/ltable.c
	${LUA_SRC_DIR}/ltablib.c
	${LUA_SRC_DIR}/ltm.c
	${LUA_SRC_DIR}/lundump.c
	${LUA_SRC_DIR}/lvm.c
	${LUA_SRC_DIR}/lzio.c
  ${LUA_SRC_DIR}/lutf8lib.c
)
set(LUA_LIB_HEADERS
	${LUA_SRC_DIR}/lua.h
	${LUA_SRC_DIR}/lualib.h
	${LUA_SRC_DIR}/lauxlib.h
	${LUA_SRC_DIR}/luaconf.h
)

#
# Lua library.
#
if(BUILD_STATIC)
	add_library(lua STATIC ${LUA_CORE_SRC})
	add_target_properties(lua COMPILE_FLAGS "${COMMON_CFLAGS}")
	add_target_properties(lua LINK_FLAGS "${LD_FLAGS} ${COMMON_LDFLAGS}")
	set(LUA_STATIC_LIB lua)
	set(LUA_LIBS lua)
else(BUILD_STATIC)
	add_library(lua_static STATIC ${LUA_CORE_SRC})
	add_target_properties(lua_static COMPILE_FLAGS "${COMMON_CFLAGS}")
	add_target_properties(lua_static LINK_FLAGS "${LD_FLAGS} ${COMMON_LDFLAGS}")
	set(LUA_STATIC_LIB lua_static)

	add_library(lua SHARED ${LUA_CORE_SRC})
	add_target_properties(lua COMPILE_FLAGS "${COMMON_CFLAGS}")
	add_target_properties(lua LINK_FLAGS "${LD_FLAGS} ${COMMON_LDFLAGS}")

	set(LUA_LIBS lua_static lua)
  set_target_properties(lua_static PROPERTIES FOLDER Dependencies/Lua)
endif(BUILD_STATIC)
set_target_properties(${LUA_LIBS} PROPERTIES
	VERSION ${LUA_VERSION}
	SOVERSION ${LUA_SOVERSION}
	CLEAN_DIRECT_OUTPUT 1
)

set_target_properties(lua PROPERTIES FOLDER Dependencies/Lua)