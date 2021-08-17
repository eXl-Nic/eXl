#pragma once

#include <gen/gen_exp.hpp>
#include <core/heapobject.hpp>
#include <math/vector2.hpp>
#include <math/aabb2d.hpp>
#include <gen/voronoigr.hpp>

#include <math/geometrytraits.hpp>

typedef std::pair<eXl::AABB2Df, unsigned int> CellLoc;

typedef boost::geometry::index::rtree< CellLoc, boost::geometry::index::rstar<16, 4> /*boost::geometry::index::linear<16, 4>*/ >
CellIndex;

//namespace arma
//{
//  template<typename Real>
//  class SpMat;
//}

namespace eXl
{
  class Random;
  class LaplaceMatrix : public HeapObject
  {
  public:
    virtual ~LaplaceMatrix()
    {
    }
  };

  class EXL_GEN_API Terrain
  {
  public:

    struct CellProperties
    {
      Vector2f position;
      unsigned int neighStart;
      unsigned int neighCount;
    };

    inline Vector<CellProperties>& GetCells() {return m_Cells;}
    inline VoronoiGraph::CellGraph& GetGraph() {return m_Graph;}

    Terrain();

    void MakeCirclePacking(Random& iRand, Vector2f const& iSize, float iCellSize);
    void MakeGrid(Vector2f const& iSize, Vector2i const& iGridSize);

    bool GetClosestCell(Vector2f const& iPos, uint32_t& oCellIdx);
    void GetLineCells(Segmentf const& iSeg, float iRadius, Vector<unsigned int>& oCells);
    void GetDiskCells(Vector2f const& iCenter, float iRadius, Vector<unsigned int>& oCells);

    void BuildLaplaceMatrix(const Vector<float>& iCellConductivity, LaplaceMatrix*& oMatrix);

    //                          centerCell, neighbourCell, edgeLength, cellDist
    typedef std::function<float(uint32_t, uint32_t, float, float)> ConductivityGetter;

    void BuildSmoothingMatrix(LaplaceMatrix*& oMatrix);
    void BuildLaplaceMatrix(ConductivityGetter& iGetter, LaplaceMatrix*& oMatrix);
    void Diffusion(LaplaceMatrix const* iMatrix, float iHeatCoefficient, Vector<float>& ioValues);

    struct OutputBuffer
    {
      void* data;
      size_t stride;

      template <typename T>
      void NextItem(T*& ioPtr)
      {
        ioPtr = reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(ioPtr) + stride);
      }

      template <typename T>
      T* Item(uint32_t iIdx)
      {
        return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(data) + iIdx * stride);
      }
    };

    size_t GetNumMeshVertices();
    size_t GetNumMeshIndices();
    size_t GetNumSmoothMeshVertices();
    size_t GetNumSmoothMeshIndices();

    void BuildMesh(Vector3f const& iScale, const Vector<float>& iHeight, OutputBuffer oPositions, OutputBuffer oNormals, OutputBuffer oTexCoords, OutputBuffer oIdx);
    void BuildSmoothMesh(Vector3f const& iScale, const Vector<float>& iHeight, OutputBuffer oPositions, OutputBuffer oNormals, OutputBuffer oTexCoords, OutputBuffer oIdx);

    void ComputeFlowMap(const Vector<float>& iHeight, Vector<unsigned int>& oSummits, Vector<int>& oNext);

    //void ComputeNormals(Vector3f const& iScale, const Vector<float>& iHeight, OutputBuffer oNormals);

  protected:

    VoronoiGraph::CellGraph m_Graph;
    Vector<CellProperties>  m_Cells;
    Vector<unsigned int>    m_Neigh;
    CellIndex               m_Index;
    Vector2f                m_Size;
    AABB2Df                 m_QueryBox;
    //float                   m_CellRadius;
  };
}
