#pragma once

#include <engine/enginelib.hpp>
#include <core/containers.hpp>
#include <core/heapobject.hpp>
#include <engine/common/object.hpp>
#include <math/vector3.hpp>

namespace eXl
{
  class EXL_ENGINE_API NeighborhoodExtraction : public HeapObject
  {
  public:
    static const uint32_t s_NumNeigh = 8;

    struct Neigh
    {
      enum Shape
      {
        Sphere,
        Box
      };

      uint32_t numNeigh = 0;
      uint32_t neighbors[s_NumNeigh];
      float    neighSqDist[s_NumNeigh];
      float    neighRelPos[s_NumNeigh];
      Shape m_Shape;
      bool populateNeigh;
    };

    virtual void AddObject(ObjectHandle iObj, float iRadius, bool iPopulateNeigh) = 0;

    virtual void AddObject(ObjectHandle iObj, Vector3f const& iBoxDim, bool iPopulateNeigh) = 0;

    virtual void Run(Vector3f const& iForwardOffset, float iRadiusSearch) = 0;

    Vector<Neigh> const& GetNeigh() const { return m_ObjectsNeigh; }
    UnorderedMap<ObjectHandle, uint32_t> const& GetObjects() const { return m_Objects; }
    Vector<ObjectHandle> const& GetHandles() const { return m_Handles; }
  protected:

    Vector<Neigh> m_ObjectsNeigh;
    UnorderedMap<ObjectHandle, uint32_t> m_Objects;
    Vector<ObjectHandle> m_Handles;
  };
}