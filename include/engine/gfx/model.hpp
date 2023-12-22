/*
Copyright 2022 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once

#include <engine/gfx/gfxcomponent.hpp>
#include <engine/gfx/bounds3d.hpp>

namespace eXl
{
  class Material;
  class MaterialInterface;

  class OGLDisplayList;

  /**
  * Model, loaded from a file.
  * Aggregates a geometry, optionaly made of individual parts with different materials.
  */
  class EXL_ENGINE_API Model : public GfxResource
  {
    struct private_ctor;
  public:

    Model(private_ctor);

    class EXL_ENGINE_API Builder
    {
    public:
      Builder& AddPart(IntrusivePtr<Material const> const& iMat, uint32_t iCount, uint32_t iStart = 0);
      IntrusivePtr<Model> End();
    private:
      Builder(IntrusivePtr<Geometry> iGeom, Box3D const& iBox);
      friend Model;
      IntrusivePtr<Model> m_Model;
    };

    [[nodiscard]] static Builder Create(IntrusivePtr<Geometry> iGeom, Box3D const& iBox);

    struct Part
    {
      IntrusivePtr<Material const> m_Mat;
      uint32_t m_Start;
      uint32_t m_Num;
    };

    Vector<Part> const& GetParts() const { return m_Parts; }
    IntrusivePtr<Geometry> const& GetGeom() const { return m_Geometry; }
    Box3D const& GetBoundingBox() const { return m_Box; }
    BoundingSphere const& GetBoundingSphere() const { return m_Sphere; }

    void Draw(OGLDisplayList&) const;

  private:
    struct private_ctor{};

    BoundingSphere m_Sphere;
    Box3D m_Box;

    IntrusivePtr<Geometry> m_Geometry;
    Vector<Part> m_Parts;
  };

  /**
  * Model instance, allows one to override a given part's material
  */
  class EXL_ENGINE_API ModelInstance : public GfxResource
  {
    struct private_ctor;
  public:

    static IntrusivePtr<ModelInstance> Create(IntrusivePtr<Model const> const& iModel);

    ModelInstance(IntrusivePtr<Model const> const& iModel, private_ctor);

    IntrusivePtr<Model const> const m_Model;
    
    bool OverridePartMaterial(uint32_t iPart, IntrusivePtr<MaterialInterface> const& iMat);

    Vector<IntrusivePtr<MaterialInterface>>& GetOverrides() { return m_Materials; }
    Vector<IntrusivePtr<MaterialInterface>> const& GetOverrides() const { return m_Materials; }

    void Draw(OGLDisplayList&) const;

  private:
    struct private_ctor {};
    Vector<IntrusivePtr<MaterialInterface>> m_Materials;
  };
}