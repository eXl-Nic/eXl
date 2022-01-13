# eXl

Game engine aiming for the following goals :
- ECS design enabling data-oriented processing of game data.
- Aim for large scale scenarios right from the start.
- Multiplayer ?
- Toolkit for Procedural Generation algorithms.
- Support at least Win32, Linux, Android.

eXl is a work in progress and not production ready.

## Modules : 

- Core : Low level wrappers, reflection, serialization, lua bindings
- Math : Geometry tools
- Gen : Procedural generation algorithms
- OGL : Rendering tools based on OpenGL
- Engine : Sprite rendering, animation, physics, pathfinding, networking
- Editor : Assets editor for the Engine.

## Building : 

CMake is used as build system.
The dependencies are retrieved using git submodules, and a custom script for the few libraries that could not fit in submodules.

The following command retrieves all the submodules:

`git submodule update --init --recursive`

To not retrieve the entire history of the submodules, use 

`git submodule update --init --recursive --depth 1`

Then, for the other dependencies, just run the setup.py script (Python3 required).
This scripts download the archives listed in dependencies.json and unzip them in a 'package' folder.
In case of issues, this can be manually done instead.
> On Linux, libclang is expected to be a system-installed library

> On Windows, setup.py expects to find a patch executable and 7zip installed at the default location.

The only dependency that is not automatically retrieved is Qt (If building the Editor).

Note that the usage of pre-built libraries is discouraged because that would make packaging binaries for various platforms harder.
Qt and libclang are the exception because they are expected to be development dependencies and would not be packaged with a game.

The following options are exposed in CMake to control what is built: 

- EXL_BUILD_SHARED -> Build libraries as shared libraries
- EXL_BUILD_WITH_LUA -> Enable eXl lua integration
- EXL_BUILD_OGL -> Build the opengl renderer
- EXL_BUILD_ENGINE ->Build the eXl engine
- EXL_BUILD_EDITOR -> Build the eXl editor

## Features :

There are the following algorithm and constructs of interest in eXl :
- A type system enabling reflection and serialization of C structs in core/type
- A code parser (inspired by https://github.com/chakaz/reflang) to automatically populate the type system.
- Lua integration with luabind, modified to rely on eXl's flavour of RTTI (some Game Engines(UE) entirely disable C++'s rtti)
- A Bentley-Ottmann segment intersection algorithm in math/seginter.hpp (focus on speed over accuracy)
- Data-oriented containers for game data in engine/common/gamedata (WIP)
- Basic pathfinding and collision avoidance based on RVO in engine/pathfinding (ORCA added for comparison).
- An implementation of http://chongyangma.com/publications/gl/2014_gl_preprint.pdf in engine/map/dungeonlayout
- ... and more to come.

