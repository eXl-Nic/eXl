project(LibSvm)

SET(SOURCES
  ${LIBSVM_ROOT}/svm.cpp
)

add_library(LibSVM STATIC ${SOURCES})

if(${LINUX})
  SET_TARGET_PROPERTIES(LibSVM PROPERTIES COMPILE_FLAGS -fPIC)
endif(${LINUX})

set_target_properties(LibSVM PROPERTIES FOLDER Dependencies)