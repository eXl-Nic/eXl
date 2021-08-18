/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <gen/gridrule.hpp>

namespace eXl
{

  MultiGrid::Iterator GridRule::MoveIter(MultiGrid::Iterator const& iIter, Vector2i const& iOffset, MultiGrid::Direction iBaseDir)
  {
    MultiGrid::Iterator curBox = iIter;
    for(unsigned int dim = 0; dim<2; ++dim)
    {
      if(iOffset.m_Data[dim] != 0)
      {
        MultiGrid::Direction dir = (MultiGrid::Direction)(2 * dim + (iOffset.m_Data[dim] > 0 ? 1 : 0));
        dir = LocalToWorld(dir, iBaseDir);
        unsigned int numIter = Mathi::Abs(iOffset.m_Data[dim]);
        for(unsigned int k = 0; k<numIter; ++k)
          curBox.Move(dir);
      }
    }
    return curBox;
  }

}