/**
  Copyright Nicolas Colombe
  2009-2014

  This file is part of eXl.

  eXl is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  eXl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with eXl.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <math/quaternion.hpp>
#include <math/vector3.hpp>
#include <ogl/oglexp.hpp>

#define CHECKOGL() OGLUtils::CheckOGLState(__FILE__,__LINE__)

namespace eXl
{
  class EXL_OGL_API OGLUtils
  {
  public:

    static void CheckOGLState(char const* iFile, int iLine);

    //static void MakeOGLMatrix(float (&oMatrix) [16], Quaternionf const& iOrient, Vector3f const& iPos);
    static void MakeOGLMatrix(float* oMatrix, Quaternionf const& iOrient, Vector3f const& iPos);

    static unsigned int CompileShader(unsigned int iType,char const* iSource);

    static unsigned int LinkProgram(unsigned int iVS, unsigned int iPS);
  };
}