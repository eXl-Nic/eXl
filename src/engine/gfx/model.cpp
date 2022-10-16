/*
Copyright 2022 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/gfx/model.hpp>
#include <ogl/renderer/ogldisplaylist.hpp>
//#include <renderer/material.hpp>

namespace eXl
{
  Model::Model(private_ctor)
  {}

  Model::Builder& Model::Builder::AddPart(IntrusivePtr<Material const> const& iMat, uint32_t iCount, uint32_t iStart)
  {
    m_Model->m_Parts.push_back(Part{ iMat, iStart, iCount });
    return *this;
  }

  IntrusivePtr<Model> Model::Builder::End()
  {
    return std::move(m_Model);
  }
  
  Model::Builder::Builder(IntrusivePtr<Geometry> iGeom, Box3D const& iBox)
  {
    m_Model = MakeRefCounted<Model>(private_ctor());
    m_Model->m_Geometry = std::move(iGeom);
    m_Model->m_Box = iBox;
    m_Model->m_Sphere.m_Center = iBox.m_Center;
    m_Model->m_Sphere.m_Radius = length(iBox.m_HalfExtent);
  }
  
  Model::Builder Model::Create(IntrusivePtr<Geometry> iGeom, Box3D const& iBox)
  {
    return Builder(std::move(iGeom), iBox);
  }


  void Model::Draw(OGLDisplayList& iList) const
  {
    iList.SetVAssembly(&m_Geometry->m_Assembly);
    for (auto& draw : m_Parts)
    {
      iList.PushDraw(2, OGLDraw::TriangleList, draw.m_Num, draw.m_Start, 0);
    }
    //DrawCommand cmd;
    //cmd.m_Connectivity = Connectivity::Triangles;
    //cmd.m_Geom = GetGeom().get();
    //
    //for (auto const& part : GetParts())
    //{  
    //  cmd.m_Mat = part.m_Mat.get();
    //  cmd.m_Start = part.m_Start;
    //  cmd.m_VertexNumber = part.m_Num;
    //
    //  iCtx.Draw(cmd);
    //}
  }
  
  IntrusivePtr<ModelInstance> ModelInstance::Create(IntrusivePtr<Model const> const& iModel)
  {
    if (!iModel)
    {
      return nullptr;
    }

    return MakeRefCounted<ModelInstance>(iModel, private_ctor());
  }

  ModelInstance::ModelInstance(IntrusivePtr<Model const> const& iModel, private_ctor)
    : m_Model(iModel)
  {
    m_Materials.resize(iModel->GetParts().size());
  }

  void ModelInstance::Draw(OGLDisplayList& iList) const
  {
    iList.SetVAssembly(&m_Model->GetGeom()->m_Assembly);
    for (uint32_t i = 0; i < m_Materials.size(); ++i)
    {
      Model::Part const& part = m_Model->GetParts()[i];
      iList.PushDraw(0, OGLDraw::TriangleList, part.m_Num, part.m_Start, 0);
    }
    //DrawCommand cmd;
    //cmd.m_Connectivity = Connectivity::Triangles;
    //cmd.m_Geom = m_Model->GetGeom().get();
    //for (uint32_t i = 0; i<m_Materials.size(); ++i)
    //{
    //  Model::Part const& part = m_Model->GetParts()[i];
    //  
    //  cmd.m_Mat = m_Materials[i] ? m_Materials[i].get() : part.m_Mat.get();
    //  cmd.m_Start = part.m_Start;
    //  cmd.m_VertexNumber = part.m_Num;
    //
    //  iCtx.Draw(cmd);
    //}
  }

  bool ModelInstance::OverridePartMaterial(uint32_t iPart, IntrusivePtr<MaterialInterface> const& iMat)
  {
    if (iPart < m_Materials.size())
    {
      m_Materials[iPart] = iMat;
      return true;
    }
    return false;
  }
}