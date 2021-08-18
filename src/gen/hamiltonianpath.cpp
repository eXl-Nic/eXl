/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <gen/hamiltonianpath.hpp>
#include <core/random.hpp>
#include <core/randomsetwalk.hpp>
#include <gen/multigrid.hpp>
#include <gen/floodfill.hpp>


namespace eXl
{
  namespace 
  {
    bool CheckCoord(Vector2i const& iPos, AABB2Di const& iBox)
    {
      return (iPos.X() >= iBox.m_Data[0].X() && iPos.X() < iBox.m_Data[1].X())
          && (iPos.Y() >= iBox.m_Data[0].Y() && iPos.Y() < iBox.m_Data[1].Y());
    }

    unsigned int GetOffset(Vector2i const& iPos, AABB2Di const& iBox)
    {
      return iPos.X() - iBox.m_Data[0].X() + (iPos.Y() - iBox.m_Data[0].Y())*(iBox.m_Data[1].X() - iBox.m_Data[0].X() );
    }
  }

  HamiltonianPath::HamiltonianPath()
  {
  }

  HamiltonianPath::HamiltonianPath(Polygoni const& iPoly, unsigned int iGridSize)
  {
    m_NumPoints = 0;
    Vector<Polygoni> temp;
    iPoly.Shrink(Mathf::Ceil((iGridSize / 2) * Mathf::Sqrt(2.0)) , temp);

    if(!temp.empty() && !temp[0].Border().empty())
    {
      Polygoni const& wkPoly = temp[0];
      //Vector2i startPt = wkPoly.Border()[0];
      Vector2i startPt = wkPoly.GetAABB().GetCenter();
      Vector2i startNeigh;
      if(wkPoly.ContainsPoint(startPt + Vector2i::UNIT_X * iGridSize))
      {
        startNeigh = startPt + Vector2i::UNIT_X * iGridSize;
      }
      else if(wkPoly.ContainsPoint(startPt - Vector2i::UNIT_X * iGridSize))
      {
        startNeigh = startPt - Vector2i::UNIT_X * iGridSize;
      }
      else if(wkPoly.ContainsPoint(startPt + Vector2i::UNIT_Y * iGridSize))
      {
        startNeigh = startPt + Vector2i::UNIT_Y * iGridSize;
      }
      else if(wkPoly.ContainsPoint(startPt - Vector2i::UNIT_Y * iGridSize))
      {
        startNeigh = startPt - Vector2i::UNIT_Y * iGridSize;
      }
      else
        return;

      startPt = startPt / iGridSize;
      startNeigh = startNeigh / iGridSize;

      m_Offset = wkPoly.GetAABB().GetCenter() - startPt * iGridSize;

      m_GridBox = iPoly.GetAABB();
      m_GridSize = iGridSize;
      m_GridBox.m_Data[0] = m_GridBox.m_Data[0] / iGridSize - Vector2i::ONE;
      m_GridBox.m_Data[1] = m_GridBox.m_Data[1] / iGridSize + Vector2i::ONE;
      Vector2i gridSize = m_GridBox.GetSize();

      m_Grid.resize((gridSize.X()) * (gridSize.Y()), -1);
      std::list<Segment> toConsider;
      {
        m_Start = 0;
        m_Points.push_back(Point(startPt));
        m_End = 1;
        m_Points.push_back(Point(startNeigh));
        m_NumPoints = 2;
        m_Points[m_Start].neigh1 = m_End;
        m_Points[m_Start].neigh2 = -1;
        m_Points[m_End].neigh1 = m_Start;
        m_Points[m_End].neigh2 = -1;
        Segment origSeg = {m_Start, m_End};
        m_Grid[GetOffset(startPt, m_GridBox)] = m_Start;
        m_Grid[GetOffset(startNeigh, m_GridBox)] = m_End;

      
        toConsider.push_back(origSeg);
      }

      while(!toConsider.empty())
      {
        Segment curSeg = toConsider.front();
        toConsider.pop_front();

        int dim = m_Points[curSeg.pt1].pos.X() == m_Points[curSeg.pt2].pos.X() ? 0 : 1;

        Vector2i candidate1 = m_Points[curSeg.pt1].pos;
        Vector2i candidate2 = m_Points[curSeg.pt2].pos;

        bool validCand = false;

        ++candidate1.m_Data[dim];
        ++candidate2.m_Data[dim];
        if(CheckCoord(candidate1, m_GridBox) && m_Grid[GetOffset(candidate1, m_GridBox)] == -1
        && CheckCoord(candidate1, m_GridBox) && m_Grid[GetOffset(candidate2, m_GridBox)] == -1
        && wkPoly.ContainsPoint(candidate1 * iGridSize + m_Offset)
        && wkPoly.ContainsPoint(candidate2 * iGridSize + m_Offset))
        {
          validCand = true;
        }
        else
        {
          candidate1.m_Data[dim] -= 2;
          candidate2.m_Data[dim] -= 2;
          if(CheckCoord(candidate1, m_GridBox) && m_Grid[GetOffset(candidate1, m_GridBox)] == -1
          && CheckCoord(candidate2, m_GridBox) && m_Grid[GetOffset(candidate2, m_GridBox)] == -1
          && wkPoly.ContainsPoint(candidate1 * iGridSize + m_Offset)
          && wkPoly.ContainsPoint(candidate2 * iGridSize + m_Offset))
          {
            validCand = true;
          }
        }
        if(validCand)
        {
          int point = m_Points.size();
          m_Points.push_back(Point(candidate1));
          int point2 = m_Points.size();
          m_Points.push_back(Point(candidate2));
          m_NumPoints += 2;
          m_Points[point].neigh1 = point2;
          m_Points[point].neigh2 = curSeg.pt1;
          m_Points[point2].neigh1 = point;
          m_Points[point2].neigh2 = curSeg.pt2;

          if(m_Points[curSeg.pt1].neigh1 == curSeg.pt2)
            m_Points[curSeg.pt1].neigh1 = point;
          else
            m_Points[curSeg.pt1].neigh2 = point;

          if(m_Points[curSeg.pt2].neigh1 == curSeg.pt1)
            m_Points[curSeg.pt2].neigh1 = point2;
          else
            m_Points[curSeg.pt2].neigh2 = point2;

          m_Grid[GetOffset(candidate1, m_GridBox)] = point;
          m_Grid[GetOffset(candidate2, m_GridBox)] = point2;

          Segment seg1 = {point, point2};
          Segment seg2 = {curSeg.pt1, point};
          Segment seg3 = {curSeg.pt2, point2};
          toConsider.push_back(seg1);
          toConsider.push_back(seg2);
          toConsider.push_back(seg3);
        }
      }
      
    }
  }
#if 0
  struct BBoxVisitor : GraphVisitor::DefaultVisitor
  {
    AABB2Di box;
    void Mark(LevelGrammar_Old::Graph const& iGraph, LevelGrammar_Old::Graph::vertex_descriptor iVtx, MultiGrid::Iterator const& iIter, MultiGrid::Direction iPrevDir)
    {
      if(box.Empty())
      {
        box = AABB2Di(iIter.GetGridCoord(), Vector2i::ONE);
      }
      else
      {
        box.Absorb(AABB2Di(iIter.GetGridCoord(), Vector2i::ONE));
      }
    }
  };

  struct Vtx
  {
    Vtx(LevelGrammar_Old::Graph::vertex_descriptor iVtx, Vector2i const& iPos, MultiGrid::Direction iDirPrev) 
      : vtx(iVtx)
      , smallGridPos(iPos)
      , dirPrev(iDirPrev)
      {}
    LevelGrammar_Old::Graph::vertex_descriptor vtx;
    Vector2i smallGridPos;
    MultiGrid::Direction dirPrev;
  };

  HamiltonianPath::HamiltonianPath(LevelGrammar_Old::Graph const& iGraph, unsigned int iGridSize)
  {
    m_NumPoints = 0;
    m_Start = -1;
    m_End = -1;
    if(boost::num_vertices(iGraph) > 0)
    {
      MultiGrid grid;
      grid.AddGenerator(MultiGrid::XDim, MultiGrid::Generator(1, 1, 0));
      grid.AddGenerator(MultiGrid::YDim, MultiGrid::Generator(1, 1, 0));
      BBoxVisitor visitor;
      visitor.box = AABB2Di();
      GraphVisitor::Visit(visitor, iGraph, *boost::vertices(iGraph).first, grid.StartGrid(Vector2i::ZERO));

      m_GridBox = visitor.box;
      m_GridBox.m_Data[0] *= 2;
      m_GridBox.m_Data[1] *= 2;
      m_GridSize = iGridSize;
      Vector2i gridSize = m_GridBox.GetSize();
      m_Grid.resize(gridSize.X() * gridSize.Y(), -1);

      m_Start = 0;

      //std::set<LevelGrammar_Old::Graph::vertex_descriptor> visitedSet;
      std::set<Vector2i> visitedSet;
      std::list<Vtx> visitList;
      visitedSet.insert(Vector2i::ZERO);
      visitList.push_back(Vtx(*boost::vertices(iGraph).first, Vector2i::ZERO, MultiGrid::RightDir));
      while(!visitList.empty())
      {
        Vtx curVtx = visitList.front();
        visitList.pop_front();

        Vector2i offsets[4] = {Vector2i::ZERO, Vector2i::UNIT_X, Vector2i::UNIT_Y, Vector2i::ONE};
        for(unsigned int i = 0; i<4; ++i)
        {
          m_Points.push_back(Point(curVtx.smallGridPos * 2 + offsets[i]));
          m_Grid[GetOffset(curVtx.smallGridPos * 2 + offsets[i], m_GridBox)] = m_NumPoints + i;
        }
        
        m_NumPoints += 4;

        std::list<Vtx> localList;
        for(auto edges = boost::out_edges(curVtx.vtx, iGraph); edges.first != edges.second; ++edges.first)
        {
          unsigned int dir = boost::get(boost::edge_name, iGraph, *edges.first);
          eXl_ASSERT_MSG(0 == (dir & ~3) || (1<<2) == (dir & ~3), "Unsupported");
          Vector2i nextDir = curVtx.smallGridPos;
          nextDir.m_Data[(dir & 2) / 2] += 2*(int(dir) & 1) - 1;

          if(visitedSet.count(nextDir) == 0)
          {
            visitedSet.insert(nextDir);
            MultiGrid::Direction prevDir =  (MultiGrid::Direction)((dir & 2) | (1 - (int(dir) & 1)));
            localList.push_back(Vtx(edges.first->m_target, nextDir, prevDir));
          }
        }
        for(auto edges = boost::in_edges(curVtx.vtx, iGraph); edges.first != edges.second; ++edges.first)
        {
          unsigned int dir = boost::get(boost::edge_name, iGraph, *edges.first);
          eXl_ASSERT_MSG(0 == (dir & ~3) || (1<<2) == (dir & ~3), "Unsupported");
          Vector2i nextDir = curVtx.smallGridPos;
          nextDir.m_Data[(dir & 2) / 2] += 2*((1 - int(dir)) & 1) - 1;
          if(visitedSet.count(nextDir) == 0)
          {
            visitedSet.insert(nextDir);
            MultiGrid::Direction prevDir = (MultiGrid::Direction)(dir);
            localList.push_back(Vtx(edges.first->m_source, nextDir, prevDir));
          }
        }

        unsigned int const linkToDo[8] = {0,2, 1,3, 0,1, 2,3};

        //visitList.insert(visitList.end(), localList.begin(), localList.end());
        for(auto iter : localList)
          visitList.push_back(iter);

        switch(localList.size())
        {
        case 0:
        case 1:
        case 2:
        case 3:
        {  
          unsigned int dir = 0;
          for(auto iter : localList) 
          {
            int revDir = iter.dirPrev & 2 | (1 - (int(iter.dirPrev) & 1));
            dir |= 1<<revDir;
          }

          if(m_NumPoints > 4)
          {
            dir |= 1<<curVtx.dirPrev;
          }

          for(unsigned int i = 0; i<4; ++i)
          {
            if(!(dir & 1<<i))
            {
              int pt1Idx = m_Grid[GetOffset(curVtx.smallGridPos * 2 + offsets[linkToDo[2*i]], m_GridBox)];
              int pt2Idx = m_Grid[GetOffset(curVtx.smallGridPos * 2 + offsets[linkToDo[2*i + 1]], m_GridBox)];

              Point& pt1 = m_Points[pt1Idx];
              Point& pt2 = m_Points[pt2Idx];
              if(pt1.neigh1 == -1) pt1.neigh1 = pt2Idx; else if(pt1.neigh2 == -1) pt1.neigh2 = pt2Idx; else eXl_ASSERT_MSG(false,"");
              if(pt2.neigh1 == -1) pt2.neigh1 = pt1Idx; else if(pt2.neigh2 == -1) pt2.neigh2 = pt1Idx; else eXl_ASSERT_MSG(false,"");
            }
          }
        }
          break;
        case 4:
          //Nothing to do.
          break;
        default:
          eXl_ASSERT_MSG(false, "Wrong situation");
          break;
        }
        if(m_NumPoints > 4)
        {
          Vector2i offset = curVtx.smallGridPos;
          offset.m_Data[(curVtx.dirPrev / 2) & 1] += 2*(int(curVtx.dirPrev) & 1) - 1;

          int reverseDir = (curVtx.dirPrev & 2) | (1 - (int(curVtx.dirPrev) & 1));

          int pt1Idx = m_Grid[GetOffset(curVtx.smallGridPos * 2 + offsets[linkToDo[2*(curVtx.dirPrev)]], m_GridBox)];
          int pt2Idx = m_Grid[GetOffset(curVtx.smallGridPos * 2 + offsets[linkToDo[2*(curVtx.dirPrev) + 1]], m_GridBox)];

          int otherPt1Idx = m_Grid[GetOffset(offset * 2 + offsets[linkToDo[2*(reverseDir)]], m_GridBox)];
          int otherPt2Idx = m_Grid[GetOffset(offset * 2 + offsets[linkToDo[2*(reverseDir) + 1]], m_GridBox)];

          Point& pt1 = m_Points[pt1Idx];
          Point& pt2 = m_Points[pt2Idx];
          Point& otherPt1 = m_Points[otherPt1Idx];
          Point& otherPt2 = m_Points[otherPt2Idx];
          if(pt1.neigh1 == -1) pt1.neigh1 = otherPt1Idx; else if(pt1.neigh2 == -1) pt1.neigh2 = otherPt1Idx; else eXl_ASSERT_MSG(false,"");
          if(pt2.neigh1 == -1) pt2.neigh1 = otherPt2Idx; else if(pt2.neigh2 == -1) pt2.neigh2 = otherPt2Idx; else eXl_ASSERT_MSG(false,"");
          if(otherPt1.neigh1 == -1) otherPt1.neigh1 = pt1Idx; else if(otherPt1.neigh2 == -1) otherPt1.neigh2 = pt1Idx; else eXl_ASSERT_MSG(false,"");
          if(otherPt2.neigh1 == -1) otherPt2.neigh1 = pt2Idx; else if(otherPt2.neigh2 == -1) otherPt2.neigh2 = pt2Idx; else eXl_ASSERT_MSG(false,"");
        }
      }
      m_End = m_Points[m_Start].neigh2;
      m_Points[m_Start].neigh2 = -1;
      if(m_Points[m_End].neigh1 == m_Start)
        m_Points[m_End].neigh1 = -1;
      else if(m_Points[m_End].neigh2 == m_Start)
        m_Points[m_End].neigh2 = -1;
      else
        eXl_ASSERT_MSG(false, "");
    }
  }
#endif

  struct ValidPos
  {

    ValidPos(Vector<HamiltonianPath::Point> const& iPts):m_Points(iPts){}

    inline bool operator()(int iVal) const {return iVal >= 0 &&
      (m_Points[iVal].neigh1 != -1 || m_Points[iVal].neigh2 != -1);}

    Vector<HamiltonianPath::Point> const& m_Points;
  };

  void HamiltonianPath::MakePoly(AABB2DPolygoni& oCycle, bool iFilled) const
  {
    if(!iFilled)
    {
      Vector<Vector2i> poly;
      int prevPt = -1;
      int curPt = m_Start;
      while(curPt != -1)
      {
        int nextPt = m_Points[curPt].neigh1 == prevPt ? m_Points[curPt].neigh2 : m_Points[curPt].neigh1;
        poly.push_back(m_Points[curPt].pos * m_GridSize + m_Offset);
        prevPt = curPt;
        curPt = nextPt;
      }
      //poly.push_back(startPoint->pos);
      oCycle = AABB2DPolygoni(poly);
    }
    else
    {
      FloodFill::MakePolygon(m_Grid, m_GridBox, ValidPos(m_Points), oCycle);
      oCycle.Scale(m_GridSize, 2);
      oCycle.Translate(m_Offset);
    }
  }

  void HamiltonianPath::BackbiteMove(Random& iRand)
  {
    int& extremity = iRand.Generate() % 2 == 0 ? m_Start : m_End;
    int& otherExtremity = extremity == m_Start ? m_End : m_Start;

    Vector2i gridPos = m_Points[extremity].pos;
    int neigh[4] = {-1, -1, -1, -1};
    bool gotNeigh = false;
    for(unsigned int i = 0; i<4; ++i)
    {
      Vector2i offsetPos = gridPos;
      offsetPos.m_Data[i / 2] += (2*(i % 2) - 1);
      if(CheckCoord(offsetPos, m_GridBox))
      {
        neigh[i] = m_Grid[GetOffset(offsetPos, m_GridBox)];
        if(neigh[i] == m_Points[extremity].neigh1 
        || neigh[i] == m_Points[extremity].neigh2)
          neigh[i] = -1;
        gotNeigh |= neigh[i] != -1;
      }
    }
    if(gotNeigh)
    {
      int choosenNeigh = -1;
      while(choosenNeigh == -1)
      {
        choosenNeigh = neigh[iRand.Generate() % 4];
      }

      if(choosenNeigh == otherExtremity)
      {
        if(m_Points[extremity].neigh1 == -1)
        {
          otherExtremity = m_Points[extremity].neigh2;
          m_Points[extremity].neigh2 = choosenNeigh;
        }
        else
        {
          otherExtremity = m_Points[extremity].neigh1;
          m_Points[extremity].neigh1 = choosenNeigh;
        }

        if(m_Points[otherExtremity].neigh1 == extremity)
        {
          m_Points[otherExtremity].neigh1 = -1;
        }
        else
        {
          m_Points[otherExtremity].neigh2 = -1;
        }

        if(m_Points[choosenNeigh].neigh1 == -1)
        {
          m_Points[choosenNeigh].neigh1 = extremity;
        }
        else
        {
          m_Points[choosenNeigh].neigh2 = extremity;
        }
      }
      else
      {
        int prevPt = choosenNeigh;
        int curPt = m_Points[choosenNeigh].neigh1;
        while(curPt != -1 && curPt != extremity)
        {
          if(m_Points[curPt].neigh1 == prevPt)
          {
            prevPt = curPt;
            curPt = m_Points[curPt].neigh2;
          }
          else
          {
            prevPt = curPt;
            curPt = m_Points[curPt].neigh1;
          }
        }

        int newExt = -1;
        if(curPt == -1)
        {
          //neigh1 lead to other extremity
          //neigh2 lead to current extremity
          newExt = m_Points[choosenNeigh].neigh2;
          m_Points[choosenNeigh].neigh2 = extremity;
        }
        else
        {
          //neigh1 lead to current extremity
          newExt = m_Points[choosenNeigh].neigh1;
          m_Points[choosenNeigh].neigh1 = extremity;
        }
        if(m_Points[newExt].neigh1 == choosenNeigh)
          m_Points[newExt].neigh1 = -1;
        else
          m_Points[newExt].neigh2 = -1;

        if(m_Points[extremity].neigh1 == -1)
          m_Points[extremity].neigh1 = choosenNeigh;
        else
          m_Points[extremity].neigh2 = choosenNeigh;

        extremity = newExt;
      }
    }
  }

  bool HamiltonianPath::ConnectedPoint(int numPt) const
  {
    return ( m_Points[numPt].neigh1 != -1 || m_Points[numPt].neigh2 != -1);
  }

  void HamiltonianPath::Cull(Random& iRand, float iReduc, Vector2f iPathRange, Vector<HamiltonianPath>* oPathes)
  {
    if(oPathes)
      oPathes->clear();

    if(iReduc > 0.0 && iReduc < 1.0)
    {
      Vector<int> distances(m_Points.size(), -1);
      {
        int curDist = 0;
        int prevPt = -1;
        int curPt = m_Start;
        while(curPt != -1)
        {
          distances[curPt] = curDist;
          int nextPt = m_Points[curPt].neigh1 == prevPt ? m_Points[curPt].neigh2 : m_Points[curPt].neigh1;
          prevPt = curPt;
          curPt = nextPt;
          ++curDist;
        }
      }

      unsigned int origNumPoints = m_NumPoints;
      unsigned int maxPathSize = Mathi::Clamp(iPathRange.Y() * m_NumPoints, 4, m_NumPoints / 2);
      unsigned int minPathSize = Mathi::Clamp(iPathRange.X() * m_NumPoints, 4, m_NumPoints / 2);
      unsigned int numTries = 0;
      while(numTries < m_NumPoints
         && float(m_NumPoints) / float(origNumPoints) > iReduc)
      {
        if(maxPathSize - 4 <= minPathSize)
        {
          maxPathSize = minPathSize + 4;
        }
        int ptNum = iRand.Generate() % m_Points.size();
        if(ptNum != m_Start && ptNum != m_End && ConnectedPoint(ptNum))
        {
          Vector2i curPos = m_Points[ptNum].pos;
          int curPtDist = distances[ptNum];
          eXl_ASSERT_MSG(curPtDist >= 0, "");
          ++numTries;
          int candidate = -1;
          int numNeigh = -1;
          int minDist;
          int maxDist;
          unsigned int curDir = 4;
          RandomSetWalk walk(4, iRand);
          while(candidate == -1 && walk.Next(curDir))
          {
            Vector2i dir;
            dir.m_Data[curDir/2] = 2*(curDir % 2) - 1;
            Vector2i neighCoord = curPos + dir;
            //while(CheckCoord(neighCoord, m_GridBox) 
            //  && m_Grid[GetOffset(neighCoord, m_GridBox)] != -1
            //  && !ConnectedPoint(m_Grid[GetOffset(neighCoord, m_GridBox)]))
            //neighCoord += dir;

            if(CheckCoord(neighCoord, m_GridBox))
            {
              int ptId = m_Grid[GetOffset(neighCoord, m_GridBox)];
              if(ptId >= 0 && ConnectedPoint(ptId))
              {
                int neighPtDist = distances[ptId];
                eXl_ASSERT_MSG(neighPtDist >= 0, "");
                unsigned int dist = Mathi::Abs(curPtDist - neighPtDist);
                if(dist >= minPathSize && dist < maxPathSize)
                {
                  candidate = ptId;
                  if(neighPtDist > curPtDist)
                  {
                    maxDist = neighPtDist;
                    minDist = curPtDist;
                    if(distances[m_Points[ptNum].neigh1] > curPtDist)
                      numNeigh = 0;
                    else if(distances[m_Points[ptNum].neigh2] > curPtDist)
                      numNeigh = 1;
                    else
                      eXl_ASSERT_MSG(false, "");
                  }
                  else
                  {
                    maxDist = curPtDist;
                    minDist = neighPtDist;
                    if(distances[m_Points[ptNum].neigh1] < curPtDist)
                      numNeigh = 0;
                    else if(distances[m_Points[ptNum].neigh2] < curPtDist)
                      numNeigh = 1;
                    else
                      eXl_ASSERT_MSG(false, "");
                  }
                }
              }
            }
          }

          
          ////Connected point
          
          
          //while(numNeigh <2 && candidate == -1)
          //{
          //  unsigned int curPathLength = 1;
          //  ++numNeigh;
          //  int prevPt = ptNum;
          //  int curPt = numNeigh == 0 ? m_Points[ptNum].neigh1 : m_Points[ptNum].neigh2;
          //  while(candidate == -1 && curPt != -1 && (curPathLength < maxPathSize))
          //  {
          //    if(curPathLength >= minPathSize)
          //    {
          //      Vector2i posNeigh = m_Points[curPt].pos;
          //      int equCoord = posNeigh.X() == curPos.X() ? 0 : (posNeigh.Y() == curPos.Y() ? 1 : -1);
          //      if(equCoord >= 0)
          //      {
          //        int sign = posNeigh.m_Data[1-equCoord] - curPos.m_Data[1-equCoord];
          //        unsigned int length = Mathi::Abs(sign);
          //        sign = sign / length;
          //        int j = 1;
          //
          //        if(length < curPathLength)
          //        {
          //          for(; j<length; ++j)
          //          {
          //            Vector2i offset = curPos;
          //            offset.m_Data[1-equCoord] += j * sign;
          //            int ptToCheck;
          //            if(!CheckCoord(offset, m_GridBox)
          //            || (ptToCheck = m_Grid[GetOffset(offset, m_GridBox)] ) == -1
          //            || ConnectedPoint(ptToCheck))
          //            {
          //              break;
          //            }
          //          }
          //          if(j == length)
          //          {
          //            eXl_ASSERT_MSG(curPt != (numNeigh == 0 ? m_Points[ptNum].neigh1 : m_Points[ptNum].neigh2), "");
          //            candidate = curPt;
          //            break;
          //          }
          //        }
          //      }
          //    }
          //    ++curPathLength;
          //    int nextPt = m_Points[curPt].neigh1 == prevPt ? m_Points[curPt].neigh2 : m_Points[curPt].neigh1;
          //    prevPt = curPt;
          //    curPt = nextPt;
          //  }
          //}

          if(candidate != -1 && candidate != m_Start && candidate != m_End)
          {

            numTries = 0;
            int prevPt = ptNum;
            int curPt = numNeigh == 0 ? m_Points[ptNum].neigh1 : m_Points[ptNum].neigh2;
            HamiltonianPath newPath;
            newPath.m_GridSize = m_GridSize;
            newPath.m_Offset = m_Offset;
            newPath.m_Start = 0;
            //Point newPt(m_Points[curPt].pos);
            //newPath.m_GridBox = AABB2Di(newPt.pos, Vector2i::ONE);
            //newPath.m_Points.push_back(newPt);
            newPath.m_NumPoints = 0;

            while(curPt != candidate)
            {

              int nextPt = m_Points[curPt].neigh1 == prevPt ? m_Points[curPt].neigh2 : m_Points[curPt].neigh1;
              if(oPathes)
              {
                Point newPt(m_Points[curPt].pos);
                newPt.neigh1 = newPath.m_NumPoints - 1;
                newPt.neigh2 = -1;
                if(!newPath.m_Points.empty())
                {
                  newPath.m_GridBox.Absorb(newPt.pos);
                  newPath.m_Points.back().neigh2 = newPath.m_NumPoints;
                }
                else
                {
                  newPath.m_GridBox = AABB2Di(newPt.pos, Vector2i::ZERO);
                }
                ++newPath.m_NumPoints;
                newPath.m_Points.push_back(newPt);
              }

              distances[curPt] = -1;
              m_Points[curPt].neigh1 = -1;
              m_Points[curPt].neigh2 = -1;

              --m_NumPoints;

              prevPt = curPt;
              curPt = nextPt;
            }

            if(oPathes)
            {
              newPath.m_GridBox.m_Data[1] += Vector2i::ONE;
              Vector2i newGridSize = newPath.m_GridBox.GetSize();
              newPath.m_Grid.resize(newGridSize.X() * newGridSize.Y(), -1);
              //newPath.m_Offset += newPath.m_GridBox.m_Data[0];
              for(unsigned int i = 0; i<newPath.m_Points.size(); ++i)
              {
                auto& pt = newPath.m_Points[i];
                //pt.pos -= newPath.m_GridBox.m_Data[0];
                newPath.m_Grid[GetOffset(pt.pos, newPath.m_GridBox)] = i;
              }

              oPathes->emplace_back(std::move(newPath));
            }

            int neighCand = -1;
            if(m_Points[candidate].neigh1 == prevPt)
              neighCand = 0;
            else
              neighCand = 1;

            Vector2i posNeigh = m_Points[candidate].pos;
            int equCoord = posNeigh.X() == curPos.X() ? 0 : (posNeigh.Y() == curPos.Y() ? 1 : -1);
            if(equCoord >= 0)
            {
              int sign = posNeigh.m_Data[1-equCoord] - curPos.m_Data[1-equCoord];
              int length = Mathi::Abs(sign);
              sign = sign / length;
              if(length == 1)
              {
                if(numNeigh == 0)
                  m_Points[ptNum].neigh1 = candidate;
                else
                  m_Points[ptNum].neigh2 = candidate;

                if(neighCand == 0)
                  m_Points[candidate].neigh1 = ptNum;
                else
                  m_Points[candidate].neigh2 = ptNum;
              }
              else
              {
                int j = 1;
                int prevLink = ptNum;
                for(; j<length; ++j)
                {
                  Vector2i offset;
                  offset.m_Data[1-equCoord] = j * sign;
                  int newLink = m_Grid[GetOffset(curPos + offset, m_GridBox)];
                  if(j == 1)
                  {
                    if(numNeigh == 0)
                      m_Points[prevLink].neigh1 = newLink;
                    else
                      m_Points[prevLink].neigh2 = newLink;
                  }
                  else
                  {
                    m_Points[prevLink].neigh2 = newLink;
                  }
                  m_Points[newLink].neigh1 = prevLink;

                  prevLink = newLink;
                  ++m_NumPoints;
                }
                
                if(m_Points[prevLink].neigh1 == -1)
                  m_Points[prevLink].neigh1 = candidate;
                else
                  m_Points[prevLink].neigh2 = candidate;

                if(neighCand == 0)
                  m_Points[candidate].neigh1 = prevLink;
                else
                  m_Points[candidate].neigh2 = prevLink;
              }

              if(minDist == distances[candidate])
              {
                int prevPt = candidate;
                int curPt = neighCand == 0 ? m_Points[candidate].neigh1 : m_Points[candidate].neigh2;
                while(curPt != -1)
                {
                  int nextPt = m_Points[curPt].neigh1 == prevPt ? m_Points[curPt].neigh2 : m_Points[curPt].neigh1;
                  distances[curPt] = distances[prevPt] + 1;
                  prevPt = curPt;
                  curPt = nextPt;
                }
              }
              else
              {
                int prevPt = ptNum;
                int curPt = numNeigh == 0 ? m_Points[ptNum].neigh1 : m_Points[ptNum].neigh2;
                while(curPt != -1)
                {
                  int nextPt = m_Points[curPt].neigh1 == prevPt ? m_Points[curPt].neigh2 : m_Points[curPt].neigh1;
                  distances[curPt] = distances[prevPt] + 1;
                  prevPt = curPt;
                  curPt = nextPt;
                }
              }
              //{
              //  int curDist = 0;
              //  int prevPt = -1;
              //  int curPt = m_Start;
              //  while(curPt != -1)
              //  {
              //    eXl_ASSERT_MSG(distances[curPt] == curDist, "");
              //    int nextPt = m_Points[curPt].neigh1 == prevPt ? m_Points[curPt].neigh2 : m_Points[curPt].neigh1;
              //    prevPt = curPt;
              //    curPt = nextPt;
              //    ++curDist;
              //  }
              //}
            }
            else
              eXl_ASSERT_MSG(false, "Could not find eqCoord");
          }
        } 
      }
    }
  }
}