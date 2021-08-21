#project(eXl_Gen)

set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/gen)

FILE(GLOB_RECURSE EXL_GEN_HEADER_FILES LIST_DIRECTORIES false ${INCLUDE_DIR}/*.hpp ${INCLUDE_DIR}/*.inl)

set (SOURCES

  ${EXL_GEN_HEADER_FILES}

  perlin.cpp
  voronoigr.cpp
  floodfill.cpp
  gridrule.cpp
  hamiltonianpath.cpp
  multigrid.cpp
  lsystemevolver.cpp
  lsysteminterpreter.cpp
  poissonsampling.cpp
  diskdistribution.cpp
  mcmcsynthesis.cpp
  mcmcdiskmodel.cpp
  mcmcsvmmodel.cpp
  pregraph.cpp
) 

if(${ENABLE_ARMADILLO})
set (SOURCES ${SOURCES}
	terrain.cpp
	${ARMADILLO_WRAPPER_SRC}
	)
endif()

add_library(eXl_Gen ${LIBRARY_TYPE} ${SOURCES})

SETUP_EXL_LIB(eXl_Gen "LEVELGEN" DEPENDENCIES eXl_Core eXl_MathStatic eXl_Math)

target_include_directories(eXl_Gen PRIVATE 
${VOROPP_INCLUDE_DIR} 
${LBFGS_INCLUDE_DIR} 
${LIBSVM_INCLUDE_DIR} 
)

if(${ENABLE_ARMADILLO})
  target_include_directories(eXl_Gen PRIVATE ${ARMADILLO_INCLUDE_DIR})
endif()

target_link_libraries(eXl_Gen LINK_PUBLIC ${BOOST_LIBRARIES})

set(PRIVATE_LIBRARIES
	Voro++ 
	LibSVM
	lbfgs_lib
)

if(${ENABLE_ARMADILLO})
target_link_directories(eXl_Gen ${LIBRARY_DEPS_VISIBILITY} ${ARMADILLO_LIBRARY_DIR})
set(PRIVATE_LIBRARIES ${PRIVATE_LIBRARIES} ${ARMADILLO_LIBRARIES})
endif()

target_link_libraries(eXl_Gen PRIVATE ${PRIVATE_LIBRARIES})