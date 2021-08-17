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

#ifndef OGLINCLUDE_INCLUDED
#define OGLINCLUDE_INCLUDED

#ifdef __ANDROID__
#include <GLES2/gl2.h>
#define GL_GLEXT_PROTOTYPES = 1
#include <GLES2/gl2ext.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#endif

#endif
