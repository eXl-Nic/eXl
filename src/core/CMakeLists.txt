set(INCLUDE_DIR ${EXL_ROOT}/include/core)

SOURCE_GROUP("Base\\Src" REGULAR_EXPRESSION "base/.*cpp")
SOURCE_GROUP("Kernel\\Header" REGULAR_EXPRESSION "kernel/.*(hpp|inl)")
SOURCE_GROUP("Kernel\\Src" REGULAR_EXPRESSION "kernel/.*cpp")
SOURCE_GROUP("Type\\Header" REGULAR_EXPRESSION "type/.*(hpp|inl)")
SOURCE_GROUP("Type\\Src" REGULAR_EXPRESSION "type/.*cpp")
SOURCE_GROUP("Stream\\Header" REGULAR_EXPRESSION "stream/.*(hpp|inl)")
SOURCE_GROUP("Stream\\Src" REGULAR_EXPRESSION "stream/.*cpp")
SOURCE_GROUP("Thread\\Header" REGULAR_EXPRESSION "thread/.*(hpp|inl)")
SOURCE_GROUP("Thread\\Src" REGULAR_EXPRESSION "thread/.*cpp")
SOURCE_GROUP("Lua\\Header" REGULAR_EXPRESSION "lua/.*(hpp|inl)")
SOURCE_GROUP("Lua\\Src" REGULAR_EXPRESSION "lua/.*cpp")
SOURCE_GROUP("Image\\Header" REGULAR_EXPRESSION "image/.*(hpp|inl)")
SOURCE_GROUP("Image\\Src" REGULAR_EXPRESSION "image/.*cpp")
SOURCE_GROUP("Utils\\Header" REGULAR_EXPRESSION "utils/.*(hpp|inl)")
SOURCE_GROUP("Utils\\Src" REGULAR_EXPRESSION "utils/.*cpp")
SOURCE_GROUP("Resource\\Header" REGULAR_EXPRESSION "resource/.*(hpp|inl)")
SOURCE_GROUP("Resource\\Src" REGULAR_EXPRESSION "resource/.*cpp")

FILE(GLOB_RECURSE EXL_CORE_HEADER_FILES LIST_DIRECTORIES false ${INCLUDE_DIR}/*.hpp ${INCLUDE_DIR}/*.inl)

SET(SOURCES

${EXL_CORE_HEADER_FILES}

base/allocator.cpp
base/application.cpp
base/clock.cpp
base/corelib.cpp
base/coretest.cpp
base/idgenerator.cpp
base/input.cpp
base/log.cpp
base/name.cpp
base/memorymanager.cpp
base/plugin.cpp
base/process.cpp
base/random.cpp
base/randomsetwalk.cpp
base/rtti.cpp
base/string.cpp
base/vlog.cpp

stream/stream_base.cpp
stream/streamer.cpp
stream/unstreamer.cpp
stream/jsonstreamer.cpp
stream/jsonunstreamer.cpp
stream/inputstream.cpp
stream/outputstream.cpp
stream/textreader.cpp

resource/resource.cpp
resource/resourceloader.cpp
resource/resourcemanager.cpp

image/image.cpp
utils/filetextreader.cpp
lua/luascript.cpp
)

set (SOURCES ${SOURCES}
  
  type/arraytype.cpp
  type/coretype.cpp
  type/fixedlengtharray.cpp
  type/resourcehandletype.cpp
  type/type.cpp
  type/typemanager.cpp
  type/tupletype.cpp
  type/tupletypestruct.cpp
  type/tupletypeview.cpp
  type/enumtype.cpp
  type/dynobject.cpp
  type/objectptrtype.cpp
  type/classtype.cpp
  type/classtyperttiobject.cpp
  type/tagtype.cpp
)

if(${EXL_BUILD_WITH_LUA})

include(lua/luabind/luabind.cmake)

set(SOURCES ${SOURCES}
  ${LUABIND_SRCS}
  lua/luabindcore.cpp
  lua/luaconverter.cpp
  lua/luamanager.cpp
  lua/luabinderimpl.cpp
  lua/luabind_eXl.cpp
)

endif()

SET(CORE_LIBS_PRIVATE ${CORE_LIBS_PRIVATE} xxhash)

if(${ENABLE_FREEIMAGE})
  set(SOURCES ${SOURCES}
    image/imagestreamer.cpp
  )
endif()

set(SOURCES ${SOURCES}
    thread/event.cpp
    thread/workerthread.cpp
)

add_library(eXl_Core ${LIBRARY_TYPE} ${SOURCES})

if(${EXL_BUILD_WITH_LUA})
target_compile_definitions(eXl_Core PRIVATE -DLUABIND_BUILDING)
SET(CORE_LIBS ${CORE_LIBS} ${LUABIND_LIBRARIES} ${LUA_LIBRARIES})
endif()

if(${ENABLE_FREEIMAGE})
  target_include_directories(eXl_Core PRIVATE ${FREEIMAGE_INCLUDE_DIR})
  target_link_directories(eXl_Core ${LIBRARY_DEPS_VISIBILITY} ${FREEIMAGE_LIBRARY_DIR})
  
  SET(CORE_LIBS_PRIVATE ${CORE_LIBS_PRIVATE} ${FREEIMAGE_LIBRARIES})
endif()

ADD_EXL_COMMON_DEFS(eXl_Core)
ADD_EXL_LIB_PROPERTIES(eXl_Core "CORE")

if(${ANDROID})
  SET(CORE_LIBS_PRIVATE ${CORE_LIBS_PRIVATE} dl log)
endif()

if(${WIN32})
  SET(CORE_LIBS_PRIVATE ${CORE_LIBS_PRIVATE} winmm bcrypt)
endif()

if(${UNIX})
  SET(CORE_LIBS_PRIVATE ${CORE_LIBS_PRIVATE} dl)
endif()

target_include_directories(eXl_Core PRIVATE ${XXHASH_INCLUDE_DIR})
target_include_directories(eXl_Core PRIVATE ${EXL_ROOT}/modules/cxxopts/include)
target_link_libraries(eXl_Core LINK_PUBLIC ${CORE_LIBS})
target_link_libraries(eXl_Core LINK_PRIVATE ${CORE_LIBS_PRIVATE})

if(${ANDROID})
else()
  if(${EXL_REFLANG_ENABLED})
    add_subdirectory(reflang ${OUT_DIR}/eXl/core/reflang)
  endif()
endif()