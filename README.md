# eXl

Toy game engine to test simulation and procedural generation algorithm

## Modules : 

- Core : Low level wrappers, reflection, serialization, lua bindings
- Math : Geometry tools
- Gen : Procedural generation algorithms
- OGL : Rendering tools based on OpenGL
- Engine : Sprite rendering, animation, physics, pathfinding
- Editor : Assets editor for the Engine.

## Building : 

CMake is used as build system

eXl needs the following dependencies : 
- Boost (header only)
- LLVM/Clang (If building OGL/Engine)
- FreeImage (If wanting to display textures)
- GLEW (If building OGL)
- Qt (If building the Editor)

Having the dependencies setup that way make it possible to build eXl_Gen with only boost as build dependency and no other runtime dependencies.
Building and integrating it into other engines then becomes much easier.

The other dependencies are bundled in the project as sub-modules.

The following variables shows where to find the dependencies : 
- Boost_ROOT
- GLEW_ROOT
- FREEIMAGE_ROOT (expects include/lib folders, might need to reshuffle a built package)
- Clang_DIR (set to <clang-dir>/lib/cmake/clang)
 -LLVM_DIR (set to <llvm-dir>/lib/cmake/llvm)

The following options are exposed to control what is built: 

- BUILD_EXL_SHARED -> Build libraries as shared libraries
- BUILD_EXL_WITH_LUA -> Enable eXl lua integration
- BUILD_EXL_OGL -> Build the opengl renderer
- BUILD_EXL_ENGINE ->Build the eXl engine
- BUILD_EXL_EDITOR -> Build the eXl editor

