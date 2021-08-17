project(LibB64)

include_directories("include")

SET(SOURCES
  src/cdecode.c
  src/cencode.c
)

add_library(libb64 STATIC
  ${SOURCES}
)

if(${LINUX})
  SET_TARGET_PROPERTIES(libb64 PROPERTIES COMPILE_FLAGS -fPIC)
endif(${LINUX})