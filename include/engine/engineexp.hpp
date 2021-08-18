/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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