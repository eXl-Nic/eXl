
#include <gtest/gtest.h>
#include <dunatk/pathfinding/navmesh.hpp>
#include <math/mathtools.hpp>

using namespace eXl;

namespace
{
  void checkEdge(NavMesh const& iMesh, uint32_t iFace1, uint32_t iFace2, Segmentf const& iSeg)
  {
    
    for(auto& edge : iMesh.GetEdges(0))
    {
      if((edge.face1 == iFace1 && edge.face2 == iFace2)
      || (edge.face2 == iFace1 && edge.face1 == iFace2))
      {
        ASSERT_EQ(edge.segment, iSeg);
        return;
      }
    }
    ASSERT_TRUE(false) << "Edge "<< iFace1 << " to "<< iFace2 << " not found";
  }
}

TEST(NavMesh, Building)
{
  Vector<AABB2Di> boxes({
    AABB2Di(Vector2i(0,0), Vector2i(10, 10)),
    AABB2Di(Vector2i(5,10), Vector2i(2, 2)),
    AABB2Di(Vector2i(5,12), Vector2i(10, 10)),
    AABB2Di(Vector2i(13,10), Vector2i(2, 2)),
    AABB2Di(Vector2i(13,0), Vector2i(10, 10)),
  });

  NavMesh navMesh = NavMesh::MakeFromBoxes(boxes);

  ASSERT_EQ(navMesh.GetComponents().size(), 1);

  Vector<uint32_t> faceMapping(5);

  auto const& dstFaces = navMesh.GetFaces(0);

  for(uint32_t srcFace = 0; srcFace < 5; ++srcFace)
  {
    for(uint32_t dstFace = 0; dstFace < dstFaces.size(); ++dstFace)
    {
      AABB2Di faceI;
      faceI.m_Data[0] = MathTools::ToIVec(dstFaces[dstFace].m_Box.m_Data[0]);
      faceI.m_Data[1] = MathTools::ToIVec(dstFaces[dstFace].m_Box.m_Data[1]);

      if(faceI == boxes[srcFace])
      {
        faceMapping[srcFace] = dstFace;
      }
    }
  }

  checkEdge(navMesh, faceMapping[0], faceMapping[1], Segmentf({Vector2f(5.0, 10.0), Vector2f(7.0, 10.0)}));
  checkEdge(navMesh, faceMapping[1], faceMapping[2], Segmentf({Vector2f(5.0, 12.0), Vector2f(7.0, 12.0)}));
  checkEdge(navMesh, faceMapping[2], faceMapping[3], Segmentf({Vector2f(13.0, 12.0), Vector2f(15.0, 12.0)}));
  checkEdge(navMesh, faceMapping[3], faceMapping[4], Segmentf({Vector2f(13.0, 10.0), Vector2f(15.0, 10.0)}));
}

#include <math/halfedge.hpp>
#include <math/seginter.hpp>

void DoFaceExtraction(Vector<Segmenti> const& iSegments)
{
  Vector<std::pair<uint32_t, Segmenti>> result;

  Intersector inter;
  inter.IntersectSegments(iSegments, result, Intersector::Parameters());

  PolyMesh mesh;
  for (auto segment : result)
  {
    Segmentf segf;
    segf.m_Ext1 = MathTools::ToFVec(segment.second.m_Ext1) * 0.01;
    segf.m_Ext2 = MathTools::ToFVec(segment.second.m_Ext2) * 0.01;
    mesh.InsertEdge(segment.second, segf, segment.first);
  }

  Vector<int> visitedSeg(mesh.edges.size(), -1);
  Vector<List<uint32_t>> faces;

  for (uint32_t i = 0; i < mesh.edges.size(); ++i)
  {
    if (visitedSeg[i] != -1)
    {
      continue;
    }

    uint32_t faceIdx = faces.size();
    faces.push_back(List<uint32_t>());
    List<uint32_t>& curFace = faces.back();

    uint32_t checkCounter = mesh.edges.size();

    uint32_t firstEdge = i;
    uint32_t curEdge = firstEdge;

    while (checkCounter != 0 && (curFace.empty() || curEdge != firstEdge))
    {
      visitedSeg[curEdge] = faceIdx;
      curFace.push_back(curEdge);
      curEdge = mesh.edges[curEdge].nextEdge;
      checkCounter--;
    }
    eXl_ASSERT(curEdge == firstEdge);
  }
#if 0
  for (uint32_t i = 0; i < faces.size(); ++i)
  {
    if(faces[i].size() > 2)
    {
      printf("Face %i :", i);
      for (auto edgeIdx : faces[i])
      {
        PolyHalfEdge const& edge = mesh.edges[edgeIdx];
        Vector2f const& srcVtx = mesh.vertices[edge.srcVtx].positionf;
        printf(" (%f, %f)", srcVtx.X(), srcVtx.Y());
      }
      printf("\n");
    }
  }
#endif
}

void TrianglesTest(std::vector<Vector2f> const& iTri1, std::vector<Vector2f> const& iTri2)
{
  for(int32_t orderSwap = 0; orderSwap<2; ++orderSwap)
  {
    Vector<Segmenti> segments;
    for(int32_t i = 0; i<2; ++i)
    {
      auto const& curTri = i == 1 - orderSwap ? iTri2 : iTri1;
      for(uint32_t j = 0; j<3; ++j)
      {
        Segmenti seg;
        seg.m_Ext1 = Vector2i(curTri[j].X() * 100, curTri[j].Y() * 100);
        seg.m_Ext2 = Vector2i(curTri[(j + 1) % 3].X() * 100, curTri[(j + 1) % 3].Y() * 100);
        segments.push_back(seg);
      }
    }
    DoFaceExtraction(segments);
  } 
}

TEST(NavMesh, TriInter)
{
  Vector<Segmenti> triangles = 
  {
    Segmenti({Vector2i(0, 0), Vector2i(500, 0)}),
    Segmenti({Vector2i(500, 0), Vector2i(250, 750)}),
    Segmenti({Vector2i(250, 750), Vector2i(0, 0)}),
    Segmenti({Vector2i(0, 500), Vector2i(500, 500)}),
    Segmenti({Vector2i(500, 500), Vector2i(250, -250)}),
    Segmenti({Vector2i(250, -250), Vector2i(0, 500)}),
  };

  Vector<Segmenti> triangles2 =
  {
    Segmenti({Vector2i(0, 0), Vector2i(500, 0)}),
    Segmenti({Vector2i(500, 0), Vector2i(250, 750)}),
    Segmenti({Vector2i(250, 750), Vector2i(0, 0)}),
    Segmenti({Vector2i(0, 250), Vector2i(500, 250)}),
    Segmenti({Vector2i(500, 250), Vector2i(250, 900)}),
    Segmenti({Vector2i(250, 900), Vector2i(0, 250)}),
  };

  std::vector<Vector2f> t1;
  std::vector<Vector2f> t2;
    //Intersection is a triangle
      //same triangles
    t1 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 1), Vector2f(1, 0)});
    t2 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 1), Vector2f(1, 0)});
    TrianglesTest(t1, t2);
    
    //three vertices of t2 on edges of t1, two edges of t2 included into edges of t1
    t1 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 1), Vector2f(1, 0)});
    t2 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 2), Vector2f(2, 0)});
    TrianglesTest(t1, t2);
    //obj = CGAL::intersection(t1, t2);
    //assert(CGAL::object_cast<Triangle>(&obj) != NULL);
    //obj = CGAL::intersection(t2, t1);
    //assert(CGAL::object_cast<Triangle>(&obj) != NULL);

    //two vertices of t2 on edges of t1
    t1 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 1), Vector2f(1, 0)});
    t2 = std::vector<Vector2f>({Vector2f(0.25, 0.25), Vector2f(0, 0.25), Vector2f(0.25, 0)});
    TrianglesTest(t1, t2);
    //obj = CGAL::intersection(t1, t2);
    //assert(CGAL::object_cast<Triangle>(&obj) != NULL);
    //obj = CGAL::intersection(t2, t1);
    //assert(CGAL::object_cast<Triangle>(&obj) != NULL);

    //three vertices of t2 on edges of t1 (no inclusion of edges)
    t1 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 1), Vector2f(1, 0)});
    t2 = std::vector<Vector2f>({Vector2f(0.5f, 0.5f), Vector2f(0, 0.25), Vector2f(0.25, 0)});
    TrianglesTest(t1, t2);
    //obj = CGAL::intersection(t1, t2);
    //assert(CGAL::object_cast<Triangle>(&obj) != NULL);
    //obj = CGAL::intersection(t2, t1);
    //assert(CGAL::object_cast<Triangle>(&obj) != NULL);

    //t2 is in the interior of t1
    t1 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 1), Vector2f(1, 0)});
    t2 = std::vector<Vector2f>({Vector2f(0.25, 0.25), Vector2f(0.25, 0.3), Vector2f(0.3, 0.25)});
    TrianglesTest(t1, t2);
    //obj = CGAL::intersection(t1, t2);
    //assert(CGAL::object_cast<Triangle>(&obj) != NULL);
    //obj = CGAL::intersection(t2, t1);
    //assert(CGAL::object_cast<Triangle>(&obj) != NULL);

    //one edge is common
    t1 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 1), Vector2f(1, 0)});
    t2 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 1), Vector2f(0.1, 0.1)});
    TrianglesTest(t1, t2);
    //obj = CGAL::intersection(t1, t2);
    //assert(CGAL::object_cast<Triangle>(&obj) != NULL);
    //obj = CGAL::intersection(t2, t1);
    //assert(CGAL::object_cast<Triangle>(&obj) != NULL);

    //one edge of t2 included into an edge of t1, one common vertex
    t1 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 1), Vector2f(1, 0)});
    t2 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 0.9), Vector2f(0.1, 0.1)});
    TrianglesTest(t1, t2);
    //obj = CGAL::intersection(t1, t2);
    //assert(CGAL::object_cast<Triangle>(&obj) != NULL);
    //obj = CGAL::intersection(t2, t1);
    //assert(CGAL::object_cast<Triangle>(&obj) != NULL);

    //one edge of t2 included into an edge of t1, no common point
    t1 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 1), Vector2f(1, 0)});
    t2 = std::vector<Vector2f>({Vector2f(0, 0.1), Vector2f(0, 0.9), Vector2f(0.1, 0.1)});
    TrianglesTest(t1, t2);
    //obj = CGAL::intersection(t1, t2);
    //assert(CGAL::object_cast<Triangle>(&obj) != NULL);
    //obj = CGAL::intersection(t2, t1);
    //assert(CGAL::object_cast<Triangle>(&obj) != NULL);

    //only one vertex of t2 included by t1
    t1 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 1), Vector2f(1, 0)});
    t2 = std::vector<Vector2f>({Vector2f(0, -1), Vector2f(0.25, 0.25), Vector2f(1, -1)});
    TrianglesTest(t1, t2);
    //obj = CGAL::intersection(t1, t2);
    //assert(CGAL::object_cast<Triangle>(&obj) != NULL);
    //obj = CGAL::intersection(t2, t1);
    //assert(CGAL::object_cast<Triangle>(&obj) != NULL);

    //only one vertex of t2 included by t1 and one vertex of t1 included in t2
    t1 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 1), Vector2f(1, 0)});
    t2 = std::vector<Vector2f>({Vector2f(0, -1), Vector2f(0, 0.25), Vector2f(1, -1)});
    TrianglesTest(t1, t2);
    //obj = CGAL::intersection(t1, t2);
    //assert(CGAL::object_cast<Triangle>(&obj) != NULL);
    //obj = CGAL::intersection(t2, t1);
    //assert(CGAL::object_cast<Triangle>(&obj) != NULL);

    //one vertex of t1 on edges of t2
    t1 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 1), Vector2f(1, 0)});
    t2 = std::vector<Vector2f>({Vector2f(-1, -1), Vector2f(0.25, 0.25), Vector2f(0.25, -1)});
    TrianglesTest(t1, t2);
    //obj = CGAL::intersection(t1, t2);
    //assert(CGAL::object_cast<Triangle>(&obj) != NULL);
    //obj = CGAL::intersection(t2, t1);
    //assert(CGAL::object_cast<Triangle>(&obj) != NULL);

    //two vertices of t1 on edges of t2
    t1 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0.5, 1), Vector2f(1, 0)});
    t2 = std::vector<Vector2f>({Vector2f(-1, -1), Vector2f(0.5, 0.5), Vector2f(2, -1)});
    TrianglesTest(t1, t2);
    //obj = CGAL::intersection(t1, t2);
    //assert(CGAL::object_cast<Triangle>(&obj) != NULL);
    //obj = CGAL::intersection(t2, t1);
    //assert(CGAL::object_cast<Triangle>(&obj) != NULL);

    //Intersection is a point  
      //edges  are collinear, one vertex in common
    t1 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 1), Vector2f(1, 0)});
    t2 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(-0.25, 0), Vector2f(0, -0.25)});
    TrianglesTest(t1, t2);
    //obj = CGAL::intersection(t1, t2);
    //assert(CGAL::object_cast<Point>(&obj) != NULL);
    //obj = CGAL::intersection(t2, t1);
    //assert(CGAL::object_cast<Point>(&obj) != NULL);

    //edges  are non-collinear, one vertex in common
    t1 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0.1, 1), Vector2f(1, 0.1)});
    t2 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(-0.25, 0), Vector2f(0, -0.25)});
    TrianglesTest(t1, t2);
    //obj = CGAL::intersection(t1, t2);
    //assert(CGAL::object_cast<Point>(&obj) != NULL);
    //obj = CGAL::intersection(t2, t1);
    //assert(CGAL::object_cast<Point>(&obj) != NULL);

    //one vertex of a triangle on an edge of another
    t1 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 1), Vector2f(1, 0)});
    t2 = std::vector<Vector2f>({Vector2f(0, 0.1), Vector2f(-0.25, 0.1), Vector2f(-0.25, -0.1)});
    TrianglesTest(t1, t2);
    //obj = CGAL::intersection(t1, t2);
    //assert(CGAL::object_cast<Point>(&obj) != NULL);
    //obj = CGAL::intersection(t2, t1);
    //assert(CGAL::object_cast<Point>(&obj) != NULL);

    //Intersection is a segment
      //triangles have a common edge
    t1 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 1), Vector2f(1, 0)});
    t2 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 1), Vector2f(-1, 0)});
    TrianglesTest(t1, t2);
    //obj = CGAL::intersection(t1, t2);
    //assert(CGAL::object_cast<Segment>(&obj) != NULL);
    //obj = CGAL::intersection(t2, t1);
    //assert(CGAL::object_cast<Segment>(&obj) != NULL);

    //one triangle edge is included into an edge of the other triangle
    t1 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 1), Vector2f(1, 0)});
    t2 = std::vector<Vector2f>({Vector2f(0, 0.1), Vector2f(0, 0.9), Vector2f(-1, 0)});
    TrianglesTest(t1, t2);
    //obj = CGAL::intersection(t1, t2);
    //assert(CGAL::object_cast<Segment>(&obj) != NULL);
    //obj = CGAL::intersection(t2, t1);
    //assert(CGAL::object_cast<Segment>(&obj) != NULL);

    //one triangle edge is included into an edge of the other triangle + share a vertex
    t1 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 1), Vector2f(1, 0)});
    t2 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 0.9), Vector2f(-1, 0)});
    TrianglesTest(t1, t2);
    //obj = CGAL::intersection(t1, t2);
    //assert(CGAL::object_cast<Segment>(&obj) != NULL);
    //obj = CGAL::intersection(t2, t1);
    //assert(CGAL::object_cast<Segment>(&obj) != NULL);

    //exactly one vertex of each triangle contributes
    t1 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 1), Vector2f(1, 0)});
    t2 = std::vector<Vector2f>({Vector2f(0, -0.1), Vector2f(0, 0.9), Vector2f(-1, 0)});
    TrianglesTest(t1, t2);
    //obj = CGAL::intersection(t1, t2);
    //assert(CGAL::object_cast<Segment>(&obj) != NULL);
    //obj = CGAL::intersection(t2, t1);
    //assert(CGAL::object_cast<Segment>(&obj) != NULL);

    t1 = std::vector<Vector2f>({Vector2f(-10, 0), Vector2f(10, 0), Vector2f(0, -3)});
    t2 = std::vector<Vector2f>({Vector2f(-8, 0), Vector2f(12, 0), Vector2f(1, 5)});
    TrianglesTest(t1, t2);
    //obj = CGAL::intersection(t1, t2);
    //assert(CGAL::object_cast<Segment>(&obj) != NULL);
    //obj = CGAL::intersection(t2, t1);
    //assert(CGAL::object_cast<Segment>(&obj) != NULL);

    //Intersection is a polygon  
      //David's star
    t1 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(1, 0), Vector2f(0.5, 1.5)});
    t2 = std::vector<Vector2f>({Vector2f(0, 1), Vector2f(1, 1), Vector2f(0.5, -0.5)});
    TrianglesTest(t1, t2);
    //obj = CGAL::intersection(t1, t2);
    //assert(CGAL::object_cast<Polygon2>(&obj) != NULL);
    //assert(CGAL::object_cast<Polygon2>(&obj)->size() == 6);
    //obj = CGAL::intersection(t2, t1);
    //assert(CGAL::object_cast<Polygon2>(&obj) != NULL);
    //assert(CGAL::object_cast<Polygon2>(&obj)->size() == 6);

    //intersection of two triangle corners
    t1 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(1, 0), Vector2f(0.5, 1)});
    t2 = std::vector<Vector2f>({Vector2f(0, 1), Vector2f(1, 1), Vector2f(0.5, 0)});
    TrianglesTest(t1, t2);
    //obj = CGAL::intersection(t1, t2);
    //assert(CGAL::object_cast<Polygon2>(&obj) != NULL);
    //assert(CGAL::object_cast<Polygon2>(&obj)->size() == 4);
    //obj = CGAL::intersection(t2, t1);
    //assert(CGAL::object_cast<Polygon2>(&obj) != NULL);
    //assert(CGAL::object_cast<Polygon2>(&obj)->size() == 4);

    //t2 pierces two edges of t1
    t1 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(1, 0), Vector2f(0, 1)});
    t2 = std::vector<Vector2f>({Vector2f(-0.1, 0.1), Vector2f(-0.1, 0.2), Vector2f(0.5, 0.8)});
    TrianglesTest(t1, t2);
    //obj = CGAL::intersection(t1, t2);
    //assert(CGAL::object_cast<Polygon2>(&obj) != NULL);
    //assert(CGAL::object_cast<Polygon2>(&obj)->size() == 4);
    //obj = CGAL::intersection(t2, t1);
    //assert(CGAL::object_cast<Polygon2>(&obj) != NULL);
    //assert(CGAL::object_cast<Polygon2>(&obj)->size() == 4);

    //Intersection is empty
    t1 = std::vector<Vector2f>({Vector2f(0, 0), Vector2f(0, 1), Vector2f(1, 0)});
    t2 = std::vector<Vector2f>({Vector2f(-0.1, -0.1), Vector2f(-0.1, -0.9), Vector2f(-1, -0.1)});
    TrianglesTest(t1, t2);
    //obj = CGAL::intersection(t1, t2);
    //assert(obj.empty());
    //obj = CGAL::intersection(t2, t1);
    //assert(obj.empty());

  DoFaceExtraction(triangles2);
}