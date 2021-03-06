/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <gen/multigrid.hpp>
#include <core/refcobject.hpp>

namespace eXl
{
  class EXL_GEN_API GridRule
  {
  public:

    inline static MultiGrid::Direction NextDirection(MultiGrid::Direction iDir)
    {
      static MultiGrid::Direction nextDir[4] = {MultiGrid::DownDir, MultiGrid::UpDir, MultiGrid::RightDir, MultiGrid::LeftDir};
      return nextDir[iDir];
    }

    inline static MultiGrid::Direction LocalToWorld(MultiGrid::Direction iDir, MultiGrid::Direction iBaseDir)
    {
      static MultiGrid::Direction rotateDir[4][4] = {{MultiGrid::RightDir, MultiGrid::LeftDir, MultiGrid::UpDir, MultiGrid::DownDir},
                                                     {MultiGrid::LeftDir, MultiGrid::RightDir, MultiGrid::DownDir, MultiGrid::UpDir},
                                                     {MultiGrid::UpDir, MultiGrid::DownDir, MultiGrid::LeftDir, MultiGrid::RightDir},
                                                     {MultiGrid::DownDir, MultiGrid::UpDir, MultiGrid::RightDir, MultiGrid::LeftDir}};
      return rotateDir[iBaseDir][iDir];
    }

    inline static MultiGrid::Direction WorldToLocal(MultiGrid::Direction iDir, MultiGrid::Direction iBaseDir)
    {
      static MultiGrid::Direction rotateDir[4][4] = {{MultiGrid::RightDir, MultiGrid::LeftDir, MultiGrid::UpDir, MultiGrid::DownDir},
                                                     {MultiGrid::LeftDir, MultiGrid::RightDir, MultiGrid::DownDir, MultiGrid::UpDir},
                                                     {MultiGrid::DownDir, MultiGrid::UpDir, MultiGrid::RightDir, MultiGrid::LeftDir},
                                                     {MultiGrid::UpDir, MultiGrid::DownDir, MultiGrid::LeftDir, MultiGrid::RightDir}};
      return rotateDir[iBaseDir][iDir];
    }

    inline static MultiGrid::BoxKey LocalToWorld(MultiGrid::BoxKey const& iKey, MultiGrid::Direction iBaseDir)
    {
      MultiGrid::BoxKey newKey;
      for(unsigned int i = 0; i<4; ++i)
      {
        newKey.data[LocalToWorld(static_cast<MultiGrid::Direction>(i), iBaseDir)] = iKey.data[i];
      }
      return newKey;
    }

    inline static MultiGrid::BoxKey WorldToLocal(MultiGrid::BoxKey const& iKey, MultiGrid::Direction iBaseDir)
    {
      MultiGrid::BoxKey newKey;
      for(unsigned int i = 0; i<4; ++i)
      {
        newKey.data[WorldToLocal(static_cast<MultiGrid::Direction>(i), iBaseDir)] = iKey.data[i];
      }
      return newKey;
    }

    inline static Vec2i LocalToWorld(Vec2i const& iPos, MultiGrid::Direction iBaseDir)
    {
      static const unsigned int numRot[] = {2, 0, 3, 1};
      Vec2i res = iPos;
      for(unsigned int i = 0; i<numRot[iBaseDir]; ++i)
      {
        res = Vec2i(-res.y, res.x);
      }
      return res;
    }

    inline static Vec2i WorldToLocal(Vec2i const& iPos, MultiGrid::Direction iBaseDir)
    {
      static const unsigned int numRot[] = {2, 0, 3, 1};
      Vec2i res = iPos;
      for(unsigned int i = 0; i<numRot[iBaseDir]; ++i)
      {
        res = Vec2i(res.y, -res.x);
      }
      return res;
    }

    inline static AABB2Di LocalToWorld(AABB2Di const& iBox, MultiGrid::Direction iBaseDir)
    {
      AABB2Di ret;
      ret.m_Data[0] = LocalToWorld(iBox.m_Data[0], iBaseDir);
      ret.m_Data[1] = LocalToWorld(iBox.m_Data[1], iBaseDir);
      return ret;
    }

    inline static AABB2Di WorldToLocal(AABB2Di const& iBox, MultiGrid::Direction iBaseDir)
    {
      AABB2Di ret;
      ret.m_Data[0] = WorldToLocal(iBox.m_Data[0], iBaseDir);
      ret.m_Data[1] = WorldToLocal(iBox.m_Data[1], iBaseDir);
      return ret;
    }

    static MultiGrid::Iterator MoveIter(MultiGrid::Iterator const& iIter, Vec2i const& iOffset, MultiGrid::Direction iDir);

    template <typename Identifier>
    struct PatternItem
    {
      Vec2i localPosition;
      Identifier Id;
    };

    template <typename Identifier>
    class Pattern
    {
    public:
      class Builder
      {
        friend class Pattern;
      public:
        inline Builder& Add(Vec2i const& iPos, Identifier const& iId)
        {
          PatternItem<Identifier> newPattern = {iPos, iId};
          m_Items.push_back(newPattern);
          return *this;
        }
        inline Pattern End()
        {
          Pattern ret;
          for(auto const& item : m_Items)
          {
            if(ret.m_Box.Empty())
            {
              ret.m_Box = AABB2Di(item.localPosition, Vec2i::ONE);
            }
            else
            {
              ret.m_Box.Absorb(AABB2Di(item.localPosition, Vec2i::ONE));
            }
          }
          ret.m_Items.swap(m_Items);
          return ret;
        }

      protected:
        Vector<PatternItem<Identifier> > m_Items;
        inline Builder(){};
      };

      inline static Builder Create(){return Builder();}

      inline Vector<PatternItem<Identifier> > const& GetItems() const {return m_Items;} 
      inline AABB2Di const& GetBox() const {return m_Box;} 

      inline Pattern(Pattern const& iOther):m_Box(iOther.m_Box), m_Items(iOther.m_Items){}
      inline Pattern(Pattern&& iOther):m_Box(iOther.m_Box){m_Items.swap(iOther.m_Items);}

    protected:
      inline Pattern(){};
      AABB2Di m_Box;
      Vector<PatternItem<Identifier> > m_Items;
    };

    template <typename Identifier, typename IdGetter>
    static bool Apply(MultiGrid::Iterator const& iBox, Pattern<Identifier> const& iPattern,  MultiGrid::Direction iBaseDir, IdGetter& iGetter);
  };

  template <typename Identifier, typename IdGetter>
  bool GridRule::Apply(MultiGrid::Iterator const& iBox, Pattern<Identifier> const& iPattern, MultiGrid::Direction iBaseDir, IdGetter& iGetter)
  {
    unsigned int numItems = iPattern.GetItems().size();
    unsigned int item = 0;
    for(; item<numItems; ++item)
    {
      PatternItem<Identifier> const& curItem = iPattern.GetItems()[item];
      
      MultiGrid::Iterator curBox = iBox.Get(LocalToWorld(curItem.localPosition, iBaseDir));
      if(!iGetter.CheckId(curBox, iBaseDir, curItem.Id))
        break;
    }
    if(item == numItems)
    {
      return true;
    }
    return false;
  }

}