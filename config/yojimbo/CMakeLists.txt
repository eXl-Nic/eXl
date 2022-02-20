set(SODIUM_ROOT ${EXL_ROOT}/modules/libsodium)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
include(ExternalProject)

set(SODIUM_LIBRARY ${OUT_DIR}/modules/libsodium/lib/libsodium.a)
ExternalProject_Add(
  libsodium
  SOURCE_DIR ${SODIUM_ROOT}
  CONFIGURE_COMMAND ${SODIUM_ROOT}/configure --prefix=${OUT_DIR}/modules/libsodium
  PREFIX ${OUT_DIR}/modules/libsodium
  BUILD_COMMAND make
  BUILD_BYPRODUCTS ${SODIUM_LIBRARY}
)
set(SODIUM_INCLUDE_DIRS ${OUT_DIR}/modules/libsodium/include)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Android")
set(SODIUM_INCLUDE_DIRS ${SODIUM_ROOT}/libsodium-android-armv8-a/include)
set(SODIUM_LIBRARY ${SODIUM_ROOT}/libsodium-android-armv8-a/lib/libsodium.a)
endif()

if(${MSVC})
set(SODIUM_LIBRARY ${SODIUM_ROOT}/bin/x64/Release/v142/static/libsodium.lib)
SET(MSBUILD_OPTIONS /p:PlatformToolset=v142 /property:Configuration=ReleaseLIB /property:Platform=x64)
SET(SODIUM_BUILD_COMMAND "${SODIUM_ROOT}/builds/msvc/vs2019/libsodium/libsodium.vcxproj")
add_custom_command(OUTPUT ${SODIUM_LIBRARY}
          COMMAND ${CMAKE_VS_MSBUILD_COMMAND} ${SODIUM_BUILD_COMMAND} ${MSBUILD_OPTIONS}
    )

add_custom_target(libsodium DEPENDS ${SODIUM_LIBRARY})
set(SODIUM_INCLUDE_DIRS ${SODIUM_ROOT}/src/libsodium/include)
endif()


add_library(yojimbo STATIC 
"${YOJIMBO_ROOT}/yojimbo.h"
"${YOJIMBO_ROOT}/yojimbo.cpp"
"${YOJIMBO_ROOT}/tlsf/tlsf.h"
"${YOJIMBO_ROOT}/tlsf/tlsf.c"
"${YOJIMBO_ROOT}/netcode.io/netcode.c"
"${YOJIMBO_ROOT}/netcode.io/netcode.h"
"${YOJIMBO_ROOT}/reliable.io/reliable.c"
"${YOJIMBO_ROOT}/reliable.io/reliable.h" 
)

target_include_directories(yojimbo PRIVATE
${YOJIMBO_ROOT}
${SODIUM_INCLUDE_DIRS}
"${YOJIMBO_ROOT}/windows"
"${YOJIMBO_ROOT}/netcode.io"
"${YOJIMBO_ROOT}/reliable.io")

target_compile_definitions(yojimbo PUBLIC -DYOJIMBO_WITH_MBEDTLS=0)
target_link_libraries(yojimbo PRIVATE ${SODIUM_LIBRARY})
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux"
OR ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  add_dependencies(yojimbo libsodium)
endif()

#if (CMAKE_BUILD_TYPE MATCHES Debug)
#target_compile_definitions(yojimbo PUBLIC -DYOJIMBO_DEBUG -DNETCODE_DEBUG -DRELIABLE_DEBUG)
#else()
#target_compile_definitions(yojimbo PUBLIC -DYOJIMBO_RELEASE -DNETCODE_RELEASE -DRELIABLE_RELEASE)
#endif()