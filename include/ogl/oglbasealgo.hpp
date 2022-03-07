/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <math/matrix4.hpp>
#include <math/vector4.hpp>
#include <ogl/oglexp.hpp>
#include <ogl/renderer/oglsemanticmanager.hpp>

namespace eXl
{
  class OGLProgram;
  class OGLCompiledProgram;

  struct EXL_OGL_API CameraMatrix
  {
    EXL_REFLECT;

    Matrix4f viewMatrix;
    Matrix4f viewInverseMatrix;
    Matrix4f projMatrix;
  };

  class EXL_OGL_API OGLBaseAlgo
  {
  public:

    static void Init(OGLSemanticManager& iManager);

    static AttributeName GetPosAttrib();
    static AttributeName GetTexCoordAttrib();

    static UniformName GetWorldMatUniform();
    static UniformName GetCameraUniform();
    static TextureName GetDiffuseTexture();
  };


  class EXL_OGL_API OGLLineAlgo
  {
  public:

    static void Init(OGLSemanticManager& iManager);

    static OGLCompiledProgram const* CreateProgram(OGLSemanticManager& iSemantics);

    static UniformName GetColor();
  };

}
