#pragma once

#include "engineexp.hpp"
#include <core/type/typetraits.hpp>

#define DEFINE_ENGINE_TYPE_EX(type, friendlyname) DEFINE_TYPE_EX(type, friendlyname, EXL_ENGINE_API)
#define DEFINE_ENGINE_TYPE(type) DEFINE_ENGINE_TYPE_EX(type, type)

namespace eXl
{
  class World;

  DEFINE_ENGINE_TYPE(World)
}