
if(${EXL_BUILD_WITH_FREEIMAGE})
  add_subdirectory(${EXL_ROOT}/config/freeimage ${OUT_DIR}/modules/freeimage)
  set_target_properties(FreeImage PROPERTIES FOLDER Dependencies)
  SET(ENABLE_FREEIMAGE ON)
  SET(FREEIMAGE_INCLUDE_DIR ${FREEIMAGE_ROOT}/Source)
  SET(FREEIMAGE_LIBRARIES FreeImage)

  set(EXL_COMPILER_DEFINITIONS ${EXL_COMPILER_DEFINITIONS} -DEXL_IMAGESTREAMER_ENABLED)
endif()