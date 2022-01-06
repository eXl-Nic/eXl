SET(SDL_INCLUDE_DIR ${SDL_ROOT}/include)
SET(SDL_LIBRARY_DIR ${SDL_ROOT}/include)
add_subdirectory(${SDL_ROOT} ${OUT_DIR}/modules/sdl)
set_target_properties(SDL2 PROPERTIES FOLDER Dependencies/SDL2)
set_target_properties(SDL2main PROPERTIES FOLDER Dependencies/SDL2)
#set_target_properties(SDL2-static PROPERTIES FOLDER Dependencies/SDL2)
set_target_properties(uninstall PROPERTIES FOLDER Dependencies/SDL2)