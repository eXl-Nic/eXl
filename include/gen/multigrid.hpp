/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <gen/gen_exp.hpp>
#include <core/containers.hpp>
#include <math/math.hpp>

#include <cstdint>

namespace eXl
{
  class EXL_GEN_API MultiGrid
  {
  public:

    enum Dimension
    {
      XDim = 0,
      YDim = 1
    };

    enum Direction
    {
      LeftDir = 0,
      RightDir = 1,
      DownDir = 2,
      UpDir = 3
    };

    struct BoxKey
    {
      inline BoxKey(uint8_t iLeft = 0, uint8_t iRight = 0, uint8_t iDown = 0, uint8_t iUp = 0)
      {
        left = iLeft;
        right = iRight;
        down = iDown;
        up = iUp;
      }
      union
      {
        uint32_t value;
        struct
        {
          uint8_t left;
          uint8_t right;          
          uint8_t down;
          uint8_t up;
        };
        uint8_t data[4];
      };
      inline bool operator == (BoxKey const& iOther) const {return value == iOther.value;}
      inline bool operator != (BoxKey const& iOther) const {return value != iOther.value;}
      inline bool operator <  (BoxKey const& iOther) const {return value <  iOther.value;};
    };

    class EXL_GEN_API Iterator
    {
      friend class MultiGrid;
    public:

      inline BoxKey const&   GetKey() const{return m_Key;}
      inline AABB2Di         GetBox() const{return AABB2Di(m_CurPos.x, m_CurPos.y, m_CurPos.x + m_Next.x, m_CurPos.y + m_Next.y);}
      inline Vec2i const& GetGridCoord() const{return m_GridPos;}

      Iterator& Up();
      Iterator& Down();
      Iterator& Left();
      Iterator& Right();

      Iterator& Move(Direction iDir);

      //On grid
      inline Iterator& Move(Vec2i const& iGridOffset){m_GridPos += iGridOffset; UpdateFromCurGrid(); return *this;}

      inline Iterator GetUp() const {return Iterator(m_Grid, m_GridPos + Vec2i(0, 1), true);}
      inline Iterator GetDown() const {return Iterator(m_Grid, m_GridPos + Vec2i(0, -1), true);}
      inline Iterator GetLeft() const {return Iterator(m_Grid, m_GridPos + Vec2i(-1, 0), true);}
      inline Iterator GetRight() const {return Iterator(m_Grid, m_GridPos + Vec2i(1, 0), true);}

      Iterator Get(Vec2i const& iGridOffset) const {return Iterator(m_Grid, m_GridPos + iGridOffset, true);}

      Iterator Get(Direction iDir) const ;

      Iterator(Iterator const& iIter) = default;
      
    protected:

      Iterator(MultiGrid const& iGrid, Vec2i const& iPos, bool iGridPos = false);

      void UpdateFromCurPos();
      void UpdateFromCurGrid();

      BoxKey m_Key;
      Vec2i m_CurPos;
      Vec2i m_GridPos;
      Vec2i m_Next;
      Vec2i m_Prev;
      MultiGrid const& m_Grid;
    };

    struct Generator
    {
      inline Generator() : period(1), phase(0), supportMin(0), supportMax(0x7FFFFFFF), flag(0), priority(0){}
      inline Generator(uint8_t iFlag, unsigned int iPeriod, unsigned int iPhase = 0, uint8_t iPriority = 0, unsigned int iSupportMin = 0, unsigned int iSupportMax = 0x7FFFFFFF)
        : period(iPeriod), phase(iPhase), supportMin(iSupportMin), supportMax(iSupportMax), flag(iFlag), priority(iPriority)
      {}
      uint32_t period;
      uint32_t phase;
      uint32_t supportMin;
      uint32_t supportMax;
      uint8_t priority;
      uint8_t flag;
    };

    class SampleVisitor
    {
    public:
      virtual void Line(Dimension iDim, unsigned int iConst, unsigned int iFrom, unsigned int iTo) const = 0;
    };

    void AddGenerator(Dimension iDim, Generator const& iGen);

    inline Vector<Generator> const& GetGeneratorsX() const {return m_Generators[0];}
    inline Vector<Generator> const& GetGeneratorsY() const {return m_Generators[1];}

    void Sample(Vec2i const& iGridDim, SampleVisitor const& iVisitor) const;

    void GetBoxes(Vec2i const& iGridDim, Map<BoxKey, unsigned int> const& iMap, Vector<AABB2Di>& oBoxes, Vector<unsigned int>& oIdent ) const;

    Vec2i GetPPCM() const;

    inline Iterator Start(Vec2i const& iPos) const {return Iterator(*this, iPos);}

    inline Iterator StartGrid(Vec2i const& iPos) const {return Iterator(*this, iPos, true);}

  protected:

    void ComputeCanonicalGrid();

    Vector<Generator> m_Generators[2];

    struct Range
    {
      bool operator < (Range const& iOther) const{return m_Begin < iOther.m_Begin;}
      unsigned int m_LowGen;
      unsigned int m_HighGen;
      unsigned int m_Begin;
      unsigned int m_End;
    };

    Vector<Range> m_CanonicalGrid[2];
  };
}

