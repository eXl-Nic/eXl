/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/corelibexp.hpp>
#include <atomic>

namespace eXl
{

#define DECLARE_RefC         \
  public:                    \
    void AddRef() const;     \
    void Release() const;    \
  private:                   \
    mutable std::atomic<unsigned int> m_RefCount{0}
  


#define IMPLEMENT_RefC(ClassName)         \
  void ClassName::AddRef() const          \
  {                                       \
    m_RefCount.fetch_add(1);              \
  }                                       \
  void ClassName::Release() const         \
  {                                       \
    if(m_RefCount.fetch_sub(1) == 1)      \
      eXl_DELETE this;                    \
  }

#define IMPLEMENT_RefCCustom(ClassName)   \
  void ClassName::AddRef() const          \
  {                                       \
    m_RefCount.fetch_add(1);              \
  }                                       \
  void ClassName::Release() const         \
  {                                       \
    if(m_RefCount.fetch_sub(1) == 1)      \
      OnNullRefC();                       \
  }
}
