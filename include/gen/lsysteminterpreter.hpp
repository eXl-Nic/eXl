#pragma once

#include <gen/multigrid.hpp>

namespace eXl
{
  class Streamer;
  class Unstreamer;

  namespace TurtleGrammar
  {
    enum Symbol
    {
      Push = 0,
      Pop = 1,
      Turn = 2,
      Advance = 3,
    };
  }


  class EXL_GEN_API LSystemInterpreter
  {
  public:

    enum Symbols
    {
      Forward = 0,
      Push = 1,
      Pop  = 2,
      Turn90 = 3,
      Turn270 = 4,
      SymA = 5,
      SymB = 6,
      SymC = 7,
      Stop = 8,
      NumSymbols = 9
    };
    
    typedef Vector<Symbols> Pattern;

    struct Item
    {
      uint32_t m_Action;
      uint32_t m_ParamValue;
    };

    struct EXL_GEN_API Rule
    {
      Pattern m_ToMatch;
      Pattern m_Replacement;

      void Stream(Streamer& iStreamer) const;
      void Unstream(Unstreamer& iUnstreamer);
    };

    static void SanitizeRule(Rule& ioRule);

    static void ApplyRules(Vector<Rule> const& iRules, Vector<Symbols>& ioString);

    static void FillMoves(LSystemInterpreter::Pattern const& iString, Vector<Item>& oMoves);
  };
}
