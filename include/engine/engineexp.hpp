#pragma once

#ifdef EXL_SHARED_LIBRARY

#ifdef WIN32

#ifdef BUILD_ENGINE_DLL
#define EXL_ENGINE_API __declspec(dllexport)
#else
#define EXL_ENGINE_API __declspec(dllimport)
#endif

#elif defined __GNUC__

#ifdef BUILD_ENGINE_DLL
#define EXL_ENGINE_API __attribute__ ((visibility("default")))
#else
#define EXL_ENGINE_API
#endif

#else
#define EXL_ENGINE_API
#endif

#else
#define EXL_ENGINE_API
#endif