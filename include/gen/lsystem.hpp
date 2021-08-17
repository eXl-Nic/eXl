#pragma once

#include <gen/graphrewriter.hpp>

namespace eXl
{
  class EXL_GEN_API LSystem : public GraphRewriter
  {
  public:

    LSystem();

    void Apply(Graph& ioGraph) const;

  protected:
    LevelGrammar_Old* m_Grammar[2];
  };
}