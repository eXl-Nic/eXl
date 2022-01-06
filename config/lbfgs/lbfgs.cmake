add_subdirectory(${LBFGS_ROOT} ${OUT_DIR}/modules/lbfgs)
set_target_properties(lbfgs_lib PROPERTIES FOLDER Dependencies)
SET(LBFGS_INCLUDE_DIR ${LBFGS_ROOT}/include)