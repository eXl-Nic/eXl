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
#pragma once

//#include <core/name.hpp>
#include <core/string.hpp>

namespace eXl
{
  struct TypeNameTag{};

  //using TypeName = Name<TypeNameTag>;
  using TypeName = String;

  struct TypeFieldNameTag{};

  //using TypeFieldName = Name<TypeFieldNameTag>;
  using TypeFieldName = String;

  struct TypeEnumNameTag{};

  //using TypeEnumName = Name<TypeEnumNameTag>;
  using TypeEnumName = String;
}