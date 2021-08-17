#pragma once

#ifdef EXL_SHARED_LIBRARY

#ifdef WIN32

//Add for GCC : with -fvisibility=hidden
//__attribute__ ((visibility("hidden"))) __attribute__ ((visibility("default")))


#ifdef BUILD_LEVELGEN_DLL
#define EXL_GEN_API __declspec(dllexport)
#else
#define EXL_GEN_API __declspec(dllimport)
#endif

#elif defined __GNUC__

#ifdef BUILD_GEN_DLL
#define EXL_GEN_API __attribute__ ((visibility("default")))
#else
#define EXL_GEN_API
#endif

#else
#define EXL_GEN_API
#endif

#else
#define EXL_GEN_API
#endif
