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

#include <ogl/oglplugin.hpp>
#include <core/log.hpp>
#include <core/lua/luamanager.hpp>

namespace eXl
{
  void Register_OGL_Types();

  OGLPlugin::OGLPlugin()
    : Plugin("eXl_OGL")
  {
    m_Dependencies.push_back("eXl_Math");
  }
  
  void OGLPlugin::_Load()
  {
    Register_OGL_Types();
    LOG_INFO<<"Loaded OGL Plugin"<<"\n";
  }
  PLUGIN_FACTORY(OGLPlugin) 
}
