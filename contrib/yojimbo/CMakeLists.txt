SET(YOJIMBO_DIR ${CMAKE_SOURCE_DIR}/modules/yojimbo)

add_library(yojimbo STATIC 
"${YOJIMBO_DIR}/yojimbo.h"
"${YOJIMBO_DIR}/yojimbo.cpp"
"${YOJIMBO_DIR}/tlsf/tlsf.h"
"${YOJIMBO_DIR}/tlsf/tlsf.c"
"${YOJIMBO_DIR}/netcode.io/netcode.c"
"${YOJIMBO_DIR}/netcode.io/netcode.h"
"${YOJIMBO_DIR}/reliable.io/reliable.c"
"${YOJIMBO_DIR}/reliable.io/reliable.h" 
)

target_include_directories(yojimbo PRIVATE
${YOJIMBO_DIR}
"${YOJIMBO_DIR}/windows"
"${YOJIMBO_DIR}/netcode.io"
"${YOJIMBO_DIR}/reliable.io")

target_link_directories(yojimbo PUBLIC "${YOJIMBO_DIR}/windows")
target_link_libraries(yojimbo PRIVATE "sodium" "mbedtls" "mbedx509" "mbedcrypto")
#if (CMAKE_BUILD_TYPE MATCHES Debug)
#target_compile_definitions(yojimbo PUBLIC -DYOJIMBO_DEBUG -DNETCODE_DEBUG -DRELIABLE_DEBUG)
#else()
#target_compile_definitions(yojimbo PUBLIC -DYOJIMBO_RELEASE -DNETCODE_RELEASE -DRELIABLE_RELEASE)
#endif()

set_target_properties(yojimbo PROPERTIES FOLDER Dependencies)