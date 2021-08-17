#pragma once

#ifdef EXL_SHARED_LIBRARY

#ifdef WIN32

#ifdef BUILD_MATH_DLL
#define EXL_MATH_API __declspec(dllexport)
#else
#define EXL_MATH_API __declspec(dllimport)
#endif
#else
#define EXL_MATH_API
#endif

#else
#define EXL_MATH_API
#endif