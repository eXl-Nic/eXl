/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <engine/enginelib.hpp>
#include <core/containers.hpp>
#include <core/heapobject.hpp>
#include <engine/common/object.hpp>
#include <math/math.hpp>

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

    virtual void AddObject(ObjectHandle iObj, Vec3 const& iBoxDim, bool iPopulateNeigh) = 0;

    virtual void Run(Vec3 const& iForwardOffset, float iRadiusSearch) = 0;

    Vector<Neigh> const& GetNeigh() const { return m_ObjectsNeigh; }
    UnorderedMap<ObjectHandle, uint32_t> const& GetObjects() const { return m_Objects; }
    Vector<ObjectHandle> const& GetHandles() const { return m_Handles; }
  protected:

    Vector<Neigh> m_ObjectsNeigh;
    UnorderedMap<ObjectHandle, uint32_t> m_Objects;
    Vector<ObjectHandle> m_Handles;
  };
}