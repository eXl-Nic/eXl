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

  void PolyMesh::UpdateSmallerPoint(std::pair<Vector2i, uint32_t> const& iNewPoint)
  {
    if (!smallerPoint)
    {
      smallerPoint = iNewPoint;
    }
    else
    {
      if (iNewPoint.first < smallerPoint->first)
      {
        smallerPoint = iNewPoint;
      }
    }
  }

  bool PolyMesh::FindInsertionPoint(uint32_t srcVtx, uint32_t dstVtx, uint32_t firstEdge, Vector2f const& iOutgoingDir, int32_t& prevInsertPt, int32_t& nextInsertPt)const
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
        Vector2f coneMid;
        float coneRange;
        Vector2f coneExts[2] = { -incomingEdge->normDir, edges[incomingEdge->nextEdge].normDir };
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

  boost::optional<uint32_t> PolyMesh::InsertEdge(Segmenti const& iSeg, Segmentf const& iFltSeg, int32_t edgeIdx)
  {
    if(iSeg.m_Ext1 == iSeg.m_Ext2)
    {
      return boost::none;
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

    Vector2f halfEdgeSegDir = iFltSeg.m_Ext2 - iFltSeg.m_Ext1;

    uint32_t curEdgeIdx = edges.size();
    uint32_t siblingEdgeIdx = edges.size() + 1;

    int32_t prevInsertPt1 = -1;
    int32_t nextInsertPt1 = -1;
    int32_t prevInsertPt2 = -1;
    int32_t nextInsertPt2 = -1;

    Vector2f outgoingDir = vertices[vtx2].positionf - vertices[vtx1].positionf;
    float segLength = outgoingDir.Normalize();

    bool goodEdge = FindInsertionPoint(vtx1, vtx2, vertices[vtx1].firstEdge, outgoingDir, prevInsertPt1, nextInsertPt1);
    goodEdge |= FindInsertionPoint(vtx2, vtx1, vertices[vtx2].firstEdge, outgoingDir * -1, prevInsertPt2, nextInsertPt2);

    if(!goodEdge)
    {
      return boost::none;
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
    siblingEdge.normDir = outgoingDir * -1;
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