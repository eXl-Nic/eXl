
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
"${YOJIMBO_ROOT}/windows"
"${YOJIMBO_ROOT}/netcode.io"
"${YOJIMBO_ROOT}/reliable.io")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
target_link_directories(yojimbo PUBLIC "${YOJIMBO_ROOT}/windows")
target_link_libraries(yojimbo PRIVATE "sodium" "mbedtls" "mbedx509" "mbedcrypto")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

endif()

#if (CMAKE_BUILD_TYPE MATCHES Debug)
#target_compile_definitions(yojimbo PUBLIC -DYOJIMBO_DEBUG -DNETCODE_DEBUG -DRELIABLE_DEBUG)
#else()
#target_compile_definitions(yojimbo PUBLIC -DYOJIMBO_RELEASE -DNETCODE_RELEASE -DRELIABLE_RELEASE)
#endif()

set_target_properties(yojimbo PROPERTIES FOLDER Dependencies)