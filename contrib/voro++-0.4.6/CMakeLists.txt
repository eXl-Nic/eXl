project(Voro++)

SET(SOURCES
  src/cell.cc
  src/cell.hh
  src/common.cc
  src/common.hh
  src/config.hh
  src/container.cc
  src/container.hh
  src/container_prd.cc
  src/container_prd.hh
  src/c_loops.cc
  src/c_loops.hh
  src/pre_container.cc
  src/pre_container.hh
  src/rad_option.hh
  src/unitcell.cc
  src/unitcell.hh
  src/voro++.hh
  src/v_base.cc
  src/v_base.hh
  src/v_compute.cc
  src/v_compute.hh
  src/wall.cc
  src/wall.hh
  src/worklist.hh
)

add_library(Voro++ STATIC
  ${SOURCES}
)

if(${LINUX})
  SET_TARGET_PROPERTIES(Voro++ PROPERTIES COMPILE_FLAGS -fPIC)
endif(${LINUX})