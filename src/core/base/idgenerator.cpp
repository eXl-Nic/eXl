/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/idgenerator.hpp>
#include <algorithm>

namespace eXl
{
  IdGenerator::IdGenerator(unsigned int heapSize):m_Size(heapSize),count(0)
  {
    if(heapSize == 0)
    {
      m_Set.add(boost::icl::discrete_interval<unsigned int>::right_open(0,0xFFFFFFFF));
      m_Size = 0xFFFFFFFF;
    }
    else
    {
      m_Set.add(boost::icl::discrete_interval<unsigned int>::right_open(0,heapSize));
    }
  }

  unsigned int IdGenerator::Get()
  {
    if(m_Set.empty())
    {
      return m_Size;
    }
    else
    {
      unsigned int res = m_Set.begin()->lower();
      m_Set -= boost::icl::discrete_interval<unsigned int>::right_open(res,res+1);
      count++;
      return res;
    }
  }

  unsigned int IdGenerator::Peek() const
  {
    if(m_Set.empty())
    {
      return m_Size;
    }
    else
    {
      unsigned int res = m_Set.begin()->lower();
      return res;
    }
  }

  void IdGenerator::Reserve(unsigned int iId)
  {
    m_Set -= boost::icl::discrete_interval<unsigned int>::right_open(iId,iId+1);
    count++;
  }

  void IdGenerator::Return(unsigned int id)
  {
    boost::icl::interval_set<unsigned int> contains = m_Set & id;
    if(contains.empty())
    {
      m_Set += boost::icl::discrete_interval<unsigned int>::right_open(id,id+1);
      count--;
    }
  }

  unsigned int IdGenerator::GetMaxId()
  {
    if(count == 0 
    || m_Set.empty()
    || m_Set.rbegin()->upper() != m_Size
    )
    {
      return m_Size;
    }
    else
    {
      return m_Set.rbegin()->lower() - 1;
    }
  }

  void IdGenerator::Grow(uint32_t iNum)
  {
    m_Set += boost::icl::discrete_interval<unsigned int>::right_open(m_Size, m_Size + iNum);
    m_Size += iNum;
  }
}
