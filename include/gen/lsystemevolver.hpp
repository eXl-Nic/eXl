/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once
#if 0
#include <gen/lsysteminterpreter.hpp>
#include <gen/gridagent.hpp>

namespace eXl
{
  class Random;
  class EXL_GEN_API LSystemEvolver
  {
  public:

    struct Parameters
    {

      inline Parameters()
      {
        genSize = 20;
        numRules = 3;
        matchSize = 1;
        replSize = 10;
        numIterations = 8;
        coverageTarget = 0.5;
        weightSquarity = 1.0;
        weightCoverage = 0.5;
        weightSymetry = 1.0;
        weighthSymetry = 1.0;
        weightvSymetry = 1.0;
        weightcSymetry = 0.0;
        weightComplexity = 0.0;
        weightSize = 0.5;
        weightEfficiency = 0.0;
        weightGrowing = 1.0;
      }

      unsigned int genSize;
      unsigned int numRules;
      unsigned int matchSize;
      unsigned int replSize;
      unsigned int numIterations;
      float coverageTarget;
      float weightSquarity;
      float weightCoverage;
      float weightSymetry;
      float weighthSymetry;
      float weightvSymetry;
      float weightcSymetry;
      float weightComplexity;
      float weightSize;
      float weightEfficiency;
      float weightGrowing;
    };
    
    LSystemEvolver(Random& iRand, Parameters const& iParams);

    void EvolutionStep();

    void LoadRules(Vector<LSystemInterpreter::Rule> const& iRules);

    //LevelGrammar_Old const* GetBestGrammar() const{return m_BestGrammar;}
    //GridAgentContext const& GetBestCtx() const{return m_Ctx;}
    //Vector<bool> const& GetBestGrid() const{return m_Grid;}
    Vector<Vector<LSystemInterpreter::Rule> > const& GetRules() const{return m_Rules;}

    std::multimap<float, unsigned int> const& GetScores() const {return m_ScoreMap;}

    float EvaluateString(LSystemInterpreter::Pattern const& , GridAgentContext& agent, Vector<bool>& grid, int& oSize) const;

  protected:

    void MorphRules();
    
    Vector<Vector<LSystemInterpreter::Rule> > m_Rules;

    int m_MaxArea;

    Random&      m_Rand;
    Parameters   m_Parameters;

    std::multimap<float, unsigned int> m_ScoreMap;
    GridAgentContext m_CtxCache;
    Vector<bool> m_GridCache;
    LSystemInterpreter::Pattern m_StringCache;
  };
}

#endif