/*
Copyright 2022 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once

#include <math/math.hpp>
#include <core/containers.hpp>
#include <engine/gfx/model.hpp>

namespace eXl
{
  class Model;
  class Material;

  struct EXL_ENGINE_API ImporterContext
  {
    ~ImporterContext();
    Mat4 importTransform = Identity<Mat4>();
    IntrusivePtr<Material const> baseMaterial;
    bool bakeTransforms = false;
    bool keepShadowCopy = false;
    UnorderedMap<String, IntrusivePtr<Material const>> materialMapping;
  };

  struct Scene
  {
    Vector<Mat4> m_Transforms;
    Vector<IntrusivePtr<Model>> m_Models;
    Box3D m_SceneBox;
  };

  EXL_ENGINE_API Scene ImportScene(ImporterContext const& iCtx, String const& iPath);
}