/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <math/halfedge.hpp>
#include <math/mathtools.hpp>

namespace eXl
{

  PolyHalfEdge const* PolyMesh::GetNextIncomingEdge(uint32_t curVtx, PolyHalfEdge const* curEdge) const
  {
    PolyHalfEdge const* nextEdge = &edges[curEdge->nextEdge];
    PolyHalfEdge const* nextIncoming = &edges[nextEdge->sibling];
    //eXl_ASSERT(nextIncoming->dstVtx == curVtx);
    return nextIncoming;
  }

  bool operator < (Vec2i const& iPt1, Vec2i const& iPt2)
  {
    if (iPt1.x == iPt2.x)
    {
      return iPt1.y < iPt2.y;
    }
    return iPt1.x < iPt2.x;
  }

  void PolyMesh::UpdateSmallerPoint(std::pair<Vec2i, uint32_t> const& iNewPoint)
  {
    if (!smallerPoint)
    {
      smallerPoint = iNewPoint;
    }
    else
    {
      if (LexicographicCompare(iNewPoint.first, smallerPoint->first))
      {
        smallerPoint = iNewPoint;
      }
    }
  }

  bool PolyMesh::FindInsertionPoint(uint32_t srcVtx, uint32_t dstVtx, uint32_t firstEdge, Vec2 const& iOutgoingDir, int32_t& prevInsertPt, int32_t& nextInsertPt)const
  {
    //Find closest prev dir. Has to be an incoming edge.
    if(firstEdge == -1)
    {
      prevInsertPt = -1;
      nextInsertPt = -1;
      return true;
    }
    else
    {
      PolyHalfEdge const* curIncoming = &edges[firstEdge];


      if(curIncoming->dstVtx != srcVtx)
      {
        curIncoming = &edges[curIncoming->sibling];
      }
      //eXl_ASSERT(curIncoming->dstVtx == srcVtx);

      if(curIncoming->srcVtx == dstVtx)
      {
        //Edge already exist.
        return false;
      }

      PolyHalfEdge const* firstIncoming = curIncoming;
      prevInsertPt = curIncoming - edges.data();
      nextInsertPt = curIncoming->nextEdge;

      bool isInCone;

      auto checkInCone = [&](PolyHalfEdge const* incomingEdge)
      {
        Vec2 coneMid;
        float coneRange;
        Vec2 coneExts[2] = { -incomingEdge->normDir, edges[incomingEdge->nextEdge].normDir };
        if (coneExts[0] == coneExts[1])
        {
          isInCone = true;
        }
        else
        {
          MathTools::ConeUpdateRange(coneExts, coneMid, coneRange);
          isInCone = MathTools::IsInCone(iOutgoingDir, coneMid, coneRange);
        }
      };

      checkInCone(curIncoming);

      curIncoming = GetNextIncomingEdge(srcVtx, curIncoming);
      while(curIncoming != firstIncoming && !isInCone)
      {
        if(curIncoming->srcVtx == dstVtx)
        {
          //Edge already exist.
          return false;
        }
        checkInCone(curIncoming);

        if(isInCone)
        {
          prevInsertPt = curIncoming - edges.data();
          nextInsertPt = curIncoming->nextEdge;
        }
        curIncoming = GetNextIncomingEdge(srcVtx, curIncoming);
      }
      //eXl_ASSERT(isInCone);
      if(!isInCone)
      {
        return false;
      }
      return true;
    }
  }

  Optional<uint32_t> PolyMesh::InsertEdge(Segmenti const& iSeg, Segmentf const& iFltSeg, int32_t edgeIdx)
  {
    if(iSeg.m_Ext1 == iSeg.m_Ext2)
    {
      return {};
    }

    auto insertRes = pointMap.insert(std::make_pair(iSeg.m_Ext1, static_cast<uint32_t>(vertices.size())));
    if(insertRes.second)
    {
      vertices.push_back(PolyVertex(iSeg.m_Ext1, iFltSeg.m_Ext1));
    }
    auto vtx1 = insertRes.first->second;
    UpdateSmallerPoint(*insertRes.first);

    insertRes = pointMap.insert(std::make_pair(iSeg.m_Ext2, static_cast<uint32_t>(vertices.size())));
    if(insertRes.second)
    {
      vertices.push_back(PolyVertex(iSeg.m_Ext2, iFltSeg.m_Ext2));
    }
    auto vtx2 = insertRes.first->second;
    UpdateSmallerPoint(*insertRes.first);

    Vec2 halfEdgeSegDir = iFltSeg.m_Ext2 - iFltSeg.m_Ext1;

    uint32_t curEdgeIdx = edges.size();
    uint32_t siblingEdgeIdx = edges.size() + 1;

    int32_t prevInsertPt1 = -1;
    int32_t nextInsertPt1 = -1;
    int32_t prevInsertPt2 = -1;
    int32_t nextInsertPt2 = -1;

    Vec2 outgoingDir = vertices[vtx2].positionf - vertices[vtx1].positionf;
    float segLength = length(outgoingDir);
    outgoingDir *= 1.f / segLength;

    bool goodEdge = FindInsertionPoint(vtx1, vtx2, vertices[vtx1].firstEdge, outgoingDir, prevInsertPt1, nextInsertPt1);
    goodEdge |= FindInsertionPoint(vtx2, vtx1, vertices[vtx2].firstEdge, outgoingDir * -1.f, prevInsertPt2, nextInsertPt2);

    if(!goodEdge)
    {
      return {};
    }

    edges.push_back(PolyHalfEdge());
    edges.push_back(PolyHalfEdge());

    auto& newEdge = edges[curEdgeIdx];
    newEdge.srcVtx = vtx1;
    newEdge.dstVtx = vtx2;
    newEdge.sibling = siblingEdgeIdx;
    newEdge.normDir = outgoingDir;
    newEdge.length = segLength;
    newEdge.userId = edgeIdx;

    auto& siblingEdge = edges[siblingEdgeIdx];
    siblingEdge.srcVtx = vtx2;
    siblingEdge.dstVtx = vtx1;
    siblingEdge.normDir = outgoingDir * -1.f;
    siblingEdge.length = segLength;
    siblingEdge.sibling = curEdgeIdx;
    siblingEdge.userId = edgeIdx;

    if(prevInsertPt1 != -1)
    {
      edges[prevInsertPt1].nextEdge = curEdgeIdx;
      siblingEdge.nextEdge = nextInsertPt1;
    }
    else
    {
      vertices[vtx1].firstEdge = curEdgeIdx;
      siblingEdge.nextEdge = curEdgeIdx;
    }

    if(prevInsertPt2 != -1)
    {
      edges[prevInsertPt2].nextEdge = siblingEdgeIdx;
      newEdge.nextEdge = nextInsertPt2;
    }
    else
    {
      vertices[vtx2].firstEdge = siblingEdgeIdx;
      newEdge.nextEdge = siblingEdgeIdx;
    }
    return curEdgeIdx;
  }

  void PolyMesh::Clear()
  {
    vertices.clear();
    edges.clear();
    pointMap.clear();
    smallerPoint.reset();
  }
}