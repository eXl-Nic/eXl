/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <gen/multigrid.hpp>
#include <algorithm>

namespace eXl
{
  void MultiGrid::AddGenerator(Dimension iDim, Generator const& iGen)
  {
    if(iGen.period != 0)
    {
      m_Generators[iDim].push_back(iGen);
      ComputeCanonicalGrid();
    }
  }

  void MultiGrid::Sample(Vec2i const& iGridDim, SampleVisitor const& iVisitor) const
  {
    for(unsigned int i = 0; i<iGridDim.x; ++i)
    {
      for(auto const & gen : m_Generators[0])
      {
        if( (i + gen.phase) % gen.period == 0)
        {
          iVisitor.Line(XDim, i, Mathi::Max(0, gen.supportMin), Mathi::Min(iGridDim.y, gen.supportMax));
        }
      }
    }
    for(unsigned int i = 0; i<iGridDim.y; ++i)
    {
      for(auto const & gen : m_Generators[1])
      {
        if( (i + gen.phase) % gen.period == 0)
        {
          iVisitor.Line(YDim, i, Mathi::Max(0, gen.supportMin), Mathi::Min(iGridDim.x, gen.supportMax));
        }
      }
    }
  }

  void MultiGrid::GetBoxes(Vec2i const& iGridDim, Map<BoxKey, unsigned int> const& iMap, Vector<AABB2Di>& oBoxes, Vector<unsigned int>& oIdent ) const
  {
    Vec2i XRange;
    int curPosX = 0;

    BoxKey curKey;

    for(unsigned int i = 0; i<iGridDim.x; ++i)
    {
      for(auto const & gen : m_Generators[0])
      {
        unsigned int curXPrio = 0;
        bool initDone = false;
        if( (i + gen.phase) % gen.period == 0)
        {
          if(!initDone)
          {
            curXPrio = gen.priority;
            initDone = true;
            XRange[curPosX] = i;
            ++curPosX;
            curKey.data[curPosX - 1] = gen.flag;
            initDone = true;
          }
          else
          {
            if(gen.priority > curXPrio)
            {
              curXPrio = gen.priority;
              curKey.data[curPosX - 1] = gen.flag;
            }
          }
        }
      }
      if(curPosX == 2 )
      {
        Vec2i YRange;
        int curPosY = 0;

        for(unsigned int j = 0; j<iGridDim.y; ++j)
        {
          for(auto const & gen : m_Generators[0])
          {
            if( (j + gen.phase) % gen.period == 0)
            {
              unsigned int curYPrio = 0;
              bool initDone = false;
              
              if(!initDone)
              {
                curYPrio = gen.priority;
                initDone = true;
                YRange[curPosY] = j;
                ++curPosY;
                curKey.data[2 + (curPosY - 1)] = gen.flag;
                initDone = true;
              }
              else
              {
                if(gen.priority > curYPrio)
                {
                  curYPrio = gen.priority;
                  curKey.data[2 + (curPosY - 1)] = gen.flag;
                }
              }
            }

            if(curPosY == 2 )
            {
              oBoxes.push_back(AABB2Di(XRange.x, YRange.x, XRange.y, YRange.y));
              auto iter = iMap.find(curKey);
              if(iter != iMap.end())
              {
                oIdent.push_back(iter->second);
              }
              else
              {
                oIdent.push_back(0);
              }
              curKey.down = curKey.up;
              YRange[0] = YRange[1];
              curPosY = 1;
            }
          }
        }
        curKey.left = curKey.right;
        XRange[0] = XRange[1];
        curPosX = 1;
      }
    }
  }

  Vec2i MultiGrid::GetPPCM() const
  {
    Vec2i ret;
    for(unsigned int coord = 0; coord < 2; ++coord)
    {
      if(!m_Generators[coord].empty())
      {
        unsigned int curPPCM = m_Generators[coord][0].period;
        for(unsigned int i = 1; i<m_Generators[coord].size(); ++i)
        {
          unsigned int pgcd = eXl::Mathi::PGCD(curPPCM, m_Generators[coord][i].period);
          curPPCM = (curPPCM * m_Generators[coord][i].period) / pgcd;
        }
        ret[coord] = curPPCM;
      }
    }
    return ret;
  }

  MultiGrid::Iterator& MultiGrid::Iterator::Up()
  {
    m_GridPos.y += 1;
    UpdateFromCurGrid();
    return *this;
  }

  MultiGrid::Iterator& MultiGrid::Iterator::Down()
  {
    m_GridPos.y -= 1;
    UpdateFromCurGrid();
    return *this;
  }

  MultiGrid::Iterator& MultiGrid::Iterator::Left()
  {
    m_GridPos.x -= 1;
    UpdateFromCurGrid();
    return *this;
  }

  MultiGrid::Iterator& MultiGrid::Iterator::Right()
  {
    m_GridPos.x += 1;
    UpdateFromCurGrid();
    return *this;
  }

  MultiGrid::Iterator& MultiGrid::Iterator::Move(Direction iDir)
  {
    switch(iDir)
    {
    case LeftDir:
      return Left();
    case RightDir:
      return Right();
    case DownDir:
      return Down();
    case UpDir:
      return Up();
    }
    return *this;
  }

  MultiGrid::Iterator MultiGrid::Iterator::Get(Direction iDir) const
  {
    switch(iDir)
    {
    case LeftDir:
      return GetLeft();
    case RightDir:
      return GetRight();
    case DownDir:
      return GetDown();
    case UpDir:
      return GetUp();
    }
    return *this;
  }

  MultiGrid::Iterator::Iterator(MultiGrid const& iGrid, Vec2i const& iPos, bool iGridPos)
    :m_Grid(iGrid)
  {
    if(!iGridPos)
    {
      Vec2i prev(0x7FFFFFFF, 0x7FFFFFFF);
      for(unsigned int dim = 0; dim<2; ++dim)
      {
        for(auto const & gen : iGrid.m_Generators[dim])
        {
          int pos = iPos[dim] + gen.phase;
          unsigned int rem = Mathi::Mod(pos, gen.period);
          if(rem < prev[dim])
          {
            prev[dim] = rem;
          }
        }
      }

      m_CurPos = iPos - prev;
      UpdateFromCurPos();
    }
    else
    {
      m_GridPos = iPos;
      UpdateFromCurGrid();
    }
  }

  void MultiGrid::Iterator::UpdateFromCurGrid()
  {
    Vec2i ppcm = m_Grid.GetPPCM();
    for(unsigned int dim = 0; dim<2; ++dim)
    {
      int curPos = Mathi::Mod(m_GridPos[dim], m_Grid.m_CanonicalGrid[dim].size());
      int offset = (m_GridPos[dim] - curPos) / (int)(m_Grid.m_CanonicalGrid[dim].size()); 

      m_Key.data[2 * dim + 0] = m_Grid.m_Generators[dim][m_Grid.m_CanonicalGrid[dim][curPos].m_LowGen].flag;
      m_Key.data[2 * dim + 1] = m_Grid.m_Generators[dim][m_Grid.m_CanonicalGrid[dim][curPos].m_HighGen].flag;

      m_CurPos[dim] = offset * ppcm[dim] + m_Grid.m_CanonicalGrid[dim][curPos].m_Begin;
      m_Next[dim] = m_Grid.m_CanonicalGrid[dim][curPos].m_End - m_Grid.m_CanonicalGrid[dim][curPos].m_Begin;
      if(curPos == 0)
      {
        m_Prev[dim] = m_Grid.m_CanonicalGrid[dim].back().m_End - m_Grid.m_CanonicalGrid[dim].back().m_Begin;
      }
      else
      {
        m_Prev[dim] = m_Grid.m_CanonicalGrid[dim][curPos - 1].m_End - m_Grid.m_CanonicalGrid[dim][curPos - 1].m_Begin;
      }
    }
  }

  void MultiGrid::Iterator::UpdateFromCurPos()
  {
    Vec2i ppcm = m_Grid.GetPPCM();
    m_Prev = Vec2i(0x7FFFFFFF, 0x7FFFFFFF);
    m_Next = Vec2i(0x7FFFFFFF, 0x7FFFFFFF);

    for(unsigned int dim = 0; dim<2; ++dim)
    {
      int curPrioPrev = -1;
      int curPrioNext = -1;
      for(auto const & gen : m_Grid.m_Generators[dim])
      {
        int pos = m_CurPos[dim] + gen.phase;
        unsigned int rem = Mathi::Mod(pos, gen.period);
        if(rem == 0)
        {
          if(curPrioPrev < gen.priority)
          {
            if(dim == 0)
              m_Key.left = gen.flag;
            else
              m_Key.down = gen.flag;
            curPrioPrev = gen.priority;
          }

          if(gen.period < m_Prev[dim])
          {
            m_Prev[dim] = gen.period;
          }
          if(gen.period < m_Next[dim])
          {
            m_Next[dim] = gen.period;
            if(dim == 0)
              m_Key.right = gen.flag;
            else
              m_Key.up = gen.flag;
            curPrioNext = gen.priority;
          }
          else if(gen.period == m_Next[dim] && gen.priority > curPrioNext)
          {
            if(dim == 0)
              m_Key.right = gen.flag;
            else
              m_Key.up = gen.flag;
            curPrioNext = gen.priority;
          }
        }
        else
        {
          if(rem < m_Prev[dim])
          {
            m_Prev[dim] = rem;
          }
          if((gen.period - rem) < m_Next[dim])
          {
            m_Next[dim] = gen.period - rem;
            if(dim == 0)
              m_Key.right = gen.flag;
            else
              m_Key.up = gen.flag;
            curPrioNext = gen.priority;
          }
          else if((gen.period - rem) == m_Next[dim] && gen.priority > curPrioNext)
          {
            if(dim == 0)
              m_Key.right = gen.flag;
            else
              m_Key.up = gen.flag;
            curPrioNext = gen.priority;
          }
        }
      }
      Range tstRange;
      tstRange.m_Begin = Mathi::Mod(m_CurPos[dim], ppcm[dim]);
      int offset = (m_CurPos[dim] - tstRange.m_Begin);
      eXl_ASSERT_MSG(Mathi::Mod(offset, ppcm[dim]) == 0, ""); 
      offset /= ppcm[dim];
      auto iter = std::lower_bound(m_Grid.m_CanonicalGrid[dim].begin(), m_Grid.m_CanonicalGrid[dim].end(), tstRange);
      eXl_ASSERT_MSG(iter != m_Grid.m_CanonicalGrid[dim].end() && iter->m_Begin == tstRange.m_Begin, "");
      int gridCoord = iter - m_Grid.m_CanonicalGrid[dim].begin();
      m_GridPos[dim] = gridCoord + offset * m_Grid.m_CanonicalGrid[dim].size();
    }
  }

  void MultiGrid::ComputeCanonicalGrid()
  {
    m_CanonicalGrid[0].clear();
    m_CanonicalGrid[1].clear();

    Vec2i ppcm = GetPPCM();
    for(unsigned int dim = 0; dim<2; ++dim)
    {
      std::map<unsigned int, unsigned int> lines;
      for(unsigned int curGen = 0; curGen < m_Generators[dim].size(); ++curGen)
      {
        auto const & gen = m_Generators[dim][curGen];
        unsigned int numLn = ppcm[dim] / gen.period;
        unsigned int offset = Mathi::Mod(gen.period - Mathi::Mod(gen.phase, gen.period), gen.period);
        for(unsigned int coord = 0; coord < numLn; ++coord)
        {
          unsigned int pos = coord * gen.period + offset;
          auto iter = lines.find(pos);
          if(iter != lines.end())
          {
            if(m_Generators[dim][iter->second].priority > gen.priority)
            {
              iter->second = curGen;
            }
          }
          else
          {
            lines.insert(std::make_pair(pos, curGen));
          }
        }
      }
      if(!lines.empty())
      {
        auto iter = lines.begin();
        auto iterEnd = lines.end();
        Range curRange;

        curRange.m_Begin = iter->first;
        curRange.m_LowGen = iter->second;
        ++iter;
        for(; iter != iterEnd; ++iter)
        {
          curRange.m_End = iter->first;
          curRange.m_HighGen = iter->second;
          m_CanonicalGrid[dim].push_back(curRange);
          curRange.m_Begin = iter->first;
          curRange.m_LowGen = iter->second;
        }

        curRange.m_End = lines.begin()->first + ppcm[dim];
        curRange.m_HighGen = lines.begin()->second;
        m_CanonicalGrid[dim].push_back(curRange);
      }
    }
  }
}