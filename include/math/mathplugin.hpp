#pragma once

#include <core/plugin.hpp>

namespace eXl
{
  class MathPlugin : public Plugin
  {
  public:
    MathPlugin();
  protected:
    void _Load();
  };
}
