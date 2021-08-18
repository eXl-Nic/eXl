/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <gen/lsystemevolver.hpp>
#include <core/random.hpp>
#include <core/log.hpp>
#include <math/math.hpp>

#if 0

#include <armadillo>

namespace eXl
{
  LSystemEvolver::LSystemEvolver(Random& iRand, Parameters const& iParams)
    :m_Rand(iRand)
    , m_Parameters(iParams)
    , m_MaxArea(0)
  {
    m_Parameters.coverageTarget = Mathf::Clamp(m_Parameters.coverageTarget, 0.001, 1.0);
    m_Parameters.numIterations = Mathi::Clamp(m_Parameters.numIterations, 1, 10);
    m_Parameters.weightComplexity = Mathf::Clamp(m_Parameters.weightComplexity, 0.0, 1.0);
    m_Parameters.weightCoverage = Mathf::Clamp(m_Parameters.weightCoverage, 0.0, 1.0);
    m_Parameters.weightEfficiency = Mathf::Clamp(m_Parameters.weightEfficiency, 0.0, 1.0);
    m_Parameters.weightSize = Mathf::Clamp(m_Parameters.weightSize, 0.0, 1.0);
    m_Parameters.weightSquarity = Mathf::Clamp(m_Parameters.weightSquarity, 0.0, 1.0);
    m_Parameters.weightSymetry = Mathf::Clamp(m_Parameters.weightSymetry, 0.0, 1.0);
    //m_GridCache.resize(m_Parameters.genSize);
    m_StringCache.resize(m_Parameters.genSize);
    //m_CtxCache.resize(m_Parameters.genSize);
  }

  void LSystemEvolver::LoadRules(Vector<LSystemInterpreter::Rule> const& iRules)
  {
    m_Parameters.numRules = iRules.size();
    Vector<LSystemInterpreter::Rule> rules = iRules;
    for(auto& rule : rules)
    {
      m_Parameters.matchSize = Mathi::Max(m_Parameters.matchSize, rule.m_ToMatch.size());
      m_Parameters.replSize = Mathi::Max(m_Parameters.replSize, rule.m_Replacement.size());
    }

    for(auto& rule : rules)
    {
      rule.m_ToMatch.resize(m_Parameters.matchSize, LSystemInterpreter::Stop);
      rule.m_Replacement.resize(m_Parameters.replSize, LSystemInterpreter::Stop);
    }
    m_Rules.clear();
    m_Rules.resize(m_Parameters.genSize, iRules);
  }

  void LSystemEvolver::MorphRules()
  {
    //Evolution.
    //Morph
    //curGen[0] = keptCand[0];
    if(m_ScoreMap.size() != m_Parameters.genSize)
      return;

    Vector<Vector<LSystemInterpreter::Rule> > keptCand(m_Parameters.genSize / 2);
    auto iter = m_ScoreMap.rbegin();
    for(unsigned int cand = 0; cand < m_Parameters.genSize / 2; ++cand, ++iter)
    {
      keptCand[cand] = m_Rules[iter->second];
    }

    for(unsigned int cand = 0; cand < m_Parameters.genSize / 2; ++cand)
    {
      m_Rules[cand] = keptCand[cand];
    }

    //Crossover
    for(unsigned int cand = m_Parameters.genSize / 2; cand < m_Parameters.genSize ; ++cand)
    {
      //m_Rules[cand].clear();
      //Set<typename OutGraph::vertex_descriptor> usedVtx;
      for(unsigned int i = 0; i<m_Parameters.numRules; ++i)
      {
        unsigned int parents[2] = {m_Rand.Generate() % (m_Parameters.genSize / 2), m_Rand.Generate() % (m_Parameters.genSize / 2)};
        for(unsigned int j = 1; j<m_Parameters.matchSize; ++j)
        {
          bool selParent = m_Rand.Generate() % 2 == 0 ;
          m_Rules[cand][i].m_ToMatch[j] = keptCand[parents[selParent]][i].m_ToMatch[j];
        }
        for(unsigned int j = 0; j<m_Parameters.replSize; ++j)
        {
          bool selParent = m_Rand.Generate() % 2 == 0 ;
          m_Rules[cand][i].m_Replacement[j] = keptCand[parents[selParent]][i].m_Replacement[j];
        }
      }
    }

    for(unsigned int cand = 1; cand < m_Parameters.genSize ; ++cand)
    {
      //Set<typename OutGraph::vertex_descriptor> usedVtx(curGen[cand].begin(), curGen[cand].end());

      unsigned int numMorph = 2 + Mathi::Max(0, int(m_Parameters.genSize) - 10);
      //unsigned int numMorph = dist(wrapper);
      //numMorph = Mathi::Max(1, Mathi::Min(numMorph, levVtx.size()));
      for(unsigned int morph = 0; morph < numMorph; ++morph)
      {
        unsigned int morphKind = m_Rand.Generate() % 3;
        switch(morphKind)
        {
        case 0:
          //Do nothing
          break;
         //Morph pattern
        //case 1:
        {
          unsigned int morphedRule = m_Rand.Generate() % (m_Parameters.numRules - 1);
          unsigned int morphedItem = m_Rand.Generate() % (m_Parameters.matchSize);
           
          m_Rules[cand][1 + morphedRule].m_ToMatch[morphedItem] = (LSystemInterpreter::Symbols)(m_Rand.Generate() % LSystemInterpreter::NumSymbols);
        }
        break;
          //MorphRepl
        case 1:
        case 2:
        {
          unsigned int morphedRule = m_Rand.Generate() % (m_Parameters.numRules);
          unsigned int morphedItem = m_Rand.Generate() % (m_Parameters.replSize);

          m_Rules[cand][morphedRule].m_Replacement[morphedItem] = (LSystemInterpreter::Symbols)(m_Rand.Generate() % LSystemInterpreter::NumSymbols);
        }
        break;
        //Swap
        //case 3:
        //{
        //  unsigned int morphedIdx1 = m_Rand.Generate() % m_NumItems;
        //  unsigned int morphedIdx2 = m_Rand.Generate() % m_NumItems;
        //  if(morphedIdx1 != morphedIdx2)
        //  {
        //    unsigned int temp = rules[cand][2 * morphedIdx1 + 0];
        //    rules[cand][2 * morphedIdx1 + 0] = rules[cand][2 * morphedIdx2 + 0];
        //    rules[cand][2 * morphedIdx2 + 0] = temp;
        //    temp = rules[cand][2 * morphedIdx1 + 1];
        //    rules[cand][2 * morphedIdx1 + 1] = rules[cand][2 * morphedIdx2 + 1];
        //    rules[cand][2 * morphedIdx2 + 1] = temp;
        //  }
        //}
        //break;
        //Move
        //case 2:
        //{
        //  unsigned int numTries = 4;
        //  for(unsigned int curTry = 0; curTry < numTries; ++curTry)
        //  {
        //    unsigned int morphedIdx = evolRand->Generate() % curGen[cand].size();
        //    typename OutGraph::vertex_descriptor oldVtx = curGen[cand][morphedIdx];
        //
        //    for(auto edges = boost::out_edges(oldVtx, iOutGraph); edges.first != edges.second; ++edges.first)
        //    {
        //      typename OutGraph::vertex_descriptor newVtx = edges.first->m_source == oldVtx ? edges.first->m_target : edges.first->m_source;
        //      if(usedVtx.count(newVtx) == 0)
        //      {
        //        usedVtx.erase(usedVtx.find(oldVtx));
        //        curGen[cand][morphedIdx] = newVtx;
        //        usedVtx.insert(newVtx);
        //        curTry = numTries;
        //        break;
        //      }
        //    }
        //  }
        //}
        //break;
        }
      }
    }
  }

  

  float LSystemEvolver::EvaluateString(LSystemInterpreter::Pattern const& iString, GridAgentContext& agent, Vector<bool>& grid, int& oSize) const
  {
    GridGrammar gridGrammar;
    {
      Vector<AgentGrammar::Item> moves;
      unsigned int numSym = 0;
      unsigned int numTurns = 0;
      unsigned int numPop = 0;
      unsigned int curveSize = 0;
      for(auto sym : iString)
      {
        AgentGrammar::Item newItem;
        switch(sym)
        {
        case LSystemInterpreter::SymA:
        case LSystemInterpreter::SymB:
        case LSystemInterpreter::SymC:
          numSym++;
          break;
        case LSystemInterpreter::Forward:
          newItem.m_Action = TurtleGrammar::Advance;
          newItem.m_ParamValue = 0;
          moves.push_back(newItem);
          ++curveSize;
          break;
        case LSystemInterpreter::Turn90:
          newItem.m_Action = TurtleGrammar::Turn;
          newItem.m_ParamValue = 1;
          moves.push_back(newItem);
          ++numTurns;
          break;
        case LSystemInterpreter::Turn270:
          newItem.m_Action = TurtleGrammar::Turn;
          newItem.m_ParamValue = 3;
          moves.push_back(newItem);
          ++numTurns;
          break;
        case LSystemInterpreter::Push:
          newItem.m_Action = TurtleGrammar::Push;
          moves.push_back(newItem);
          break;
        case LSystemInterpreter::Pop:
          newItem.m_Action = TurtleGrammar::Pop;
          moves.push_back(newItem);
          ++numPop;
          break;
        }
      }

      gridGrammar.Run(agent, moves);

      Vector2i sizeBox = agent.GetBox().GetSize();
      int globArea = sizeBox.X() * sizeBox.Y();
      if(globArea == 0)
      {
        oSize = 0;
        return 0.0;
      }
      else
      {
        //float errorCoeff = 1.0 - float(errors[j]) / float(m_Parameters.numRules * m_Parameters.matchSize);
        //float areaEfficiency = float(totArea) / float(globArea);
        //float turnCoeff = float(numTurns) / float(moves.size());
        //int effectiveArea = totArea - ctx[j].ComputeOverlap();
        //float coverage = ctx[j].ComputeCoverage();
        float coverage = agent.FillGrid(grid);

        oSize = coverage;
        
        float coveragePercent =  coverage / float(globArea);

        int symetryScores[3] = {0, 0, 0};
        
        unsigned int offset = 0;
        unsigned int offsetc = globArea - 1;
        for(unsigned int yC = 0; yC < sizeBox.Y(); ++yC)
        {
          unsigned int offseth = yC * sizeBox.X() + sizeBox.X() - 1;
          unsigned int offsetv = (sizeBox.Y() - 1 - yC) * sizeBox.X();
          for(unsigned int xC = 0; xC < sizeBox.X(); ++xC)
          {
            if(grid[offset])
            {
              if(grid[offset] == grid[offseth])
                ++symetryScores[0];
              if(grid[offset] == grid[offsetv])
                ++symetryScores[1];
              if(grid[offset] == grid[offsetc])
                ++symetryScores[2];
            }
            ++offset;
            --offsetc;
            --offseth;
            ++offsetv;
          }
        }
        float symIntCoeff[3] = {m_Parameters.weighthSymetry * symetryScores[0],
                                m_Parameters.weightvSymetry * symetryScores[1],
                                m_Parameters.weightcSymetry * symetryScores[2]};

        float maxSym = Mathf::Max(Mathf::Max(symIntCoeff[0],symIntCoeff[1]),symIntCoeff[2]);
        float symetryCoeff = 0.0;
        for(unsigned int i = 0; i<3; ++i)
        {
          if(symIntCoeff[i] == maxSym)
            symetryCoeff = float(symetryScores[i]) / float(coverage);
        }

        float structuralComplexity = float(numTurns + numPop) / float(moves.size()) /*+ numSym / string.size()*//*boost::num_vertices(newGraph)*/;
        float squarity = float(Mathi::Min(sizeBox.X() , sizeBox.Y()));
        float efficiency = float(coverage) / float(curveSize);
        //squarity *= squarity;
        squarity /= float(Mathi::Max(sizeBox.X() , sizeBox.Y()));

        float coverageScore = 1.0 / (1.0 + exp(20 *((m_Parameters.coverageTarget - 0.05) - coveragePercent))) * 1.0 /(1.0 + exp(20 *(coveragePercent - (m_Parameters.coverageTarget + 0.05))));
        //float coverageScore = (0.25*exp(-1.0 * (coveragePercent - 0.45) * (coveragePercent - 0.45) / sigma1) 
        //  + exp(-1.0 * (coveragePercent - 0.80) * (coveragePercent - 0.80) / sigma2));

        oSize = coverage;
        return (m_Parameters.weightComplexity * structuralComplexity
          + m_Parameters.weightCoverage * coverageScore
          + m_Parameters.weightEfficiency * efficiency
          + m_Parameters.weightSymetry * symetryCoeff
          + m_Parameters.weightSquarity * squarity);
      }
    }
  }

  void LSystemEvolver::EvolutionStep()
  {
    //unsigned int ppcmSample = 1;
    //for(unsigned int i = 0; i<gridGrammar.GetNumActions(); ++i)
    //{
    //  Vector2i paramRange = gridGrammar.GetAction(i)->GetRange();
    //  unsigned int range = paramRange.Y() - paramRange.X();
    //  unsigned int pgcd = Mathi::PGCD(ppcmSample, range);
    //  ppcmSample = (ppcmSample * range) / pgcd;
    //}
    if(m_Rules.empty())
    {
      for(unsigned int i = 0; i<m_Parameters.genSize; ++i)
      {
        Vector<LSystemInterpreter::Rule> rules;
        for(unsigned int j = 0; j<m_Parameters.numRules; ++j)
        {
          LSystemInterpreter::Rule curRule;
          //if(j == 0)
          {
            curRule.m_ToMatch.push_back((LSystemInterpreter::Symbols)(LSystemInterpreter::SymA + j));
            for(unsigned int k = 1; k<m_Parameters.matchSize; ++k)
              curRule.m_ToMatch.push_back(LSystemInterpreter::Stop);
          }
          //else
          //{
          //  for(unsigned int k = 0; k< m_Parameters.m_MatchSize; ++k)
          //  {
          //   curRule.m_ToMatch.push_back((LSystemInterpreter::Symbols)(m_Rand.Generate() % LSystemInterpreter::NumSymbols));
          //  }
          //}
          for(unsigned int k = 0; k< m_Parameters.replSize; ++k)
          {
            curRule.m_Replacement.push_back((LSystemInterpreter::Symbols)(m_Rand.Generate() % LSystemInterpreter::NumSymbols));
          }
          rules.push_back(curRule);
        }
        m_Rules.emplace_back(std::move(rules));
      }
    }
    else
    {
      MorphRules();
    }
    
    Vector<float> prevScore(m_Parameters.genSize);
    Vector<float> curScore(m_Parameters.genSize);
    Vector<int> prevSize(m_Parameters.genSize, 0);
    Vector<int> curSize(m_Parameters.genSize, 0);
    Vector<unsigned int> errors(m_Parameters.genSize);
    Vector<LevelGrammar_Old*> grammars(m_Parameters.genSize, NULL);
    std::multimap<float, unsigned int> scoreMap;
    //for(unsigned int numIter = 0; numIter < 500 && search; ++numIter)
    {
      scoreMap.clear();
      for(unsigned int j = 0; j<m_Parameters.genSize; ++j)
      {
        curScore[j] = 0;

        Vector<LSystemInterpreter::Rule> localRules = m_Rules[j];

        for(auto& rule : localRules)
        {
          LSystemInterpreter::SanitizeRule(rule);
        }
        //grammars[j] = LSystemInterpreter::MakeGrammar(m_Rules[j], errors[j]);

        //if(grammars[j] != NULL)
        {
          //LevelGrammar_Old::Graph newGraph;
          //auto vtx = boost::add_vertex(newGraph);
          //boost::put(boost::vertex_name, newGraph, vtx, LSystemInterpreter::SymA);

          LSystemInterpreter::Pattern& string = m_StringCache;
          string.clear();
          string.push_back(LSystemInterpreter::SymA);

          for(unsigned int iter = 0; iter < m_Parameters.numIterations - 1; ++iter)
          {
            //LevelGrammar_Old::Graph temp;
            //LevelGrammar_Old::VertexMatching matching;
            //grammars[j]->Apply(newGraph, temp, matching);
            //newGraph.swap(temp);
            LSystemInterpreter::ApplyRules(localRules, string);
          }

          if(string.size() > 500000)
          {
            prevScore[j] = 0;
            curScore[j] = 0;
          }
          else
          {
            m_CtxCache.Reset();
            prevScore[j] = EvaluateString(string, m_CtxCache, m_GridCache, prevSize[j]);

            LSystemInterpreter::ApplyRules(localRules, string);
            if(string.size() > 500000)
            {
              prevScore[j] = 0;
              curScore[j] = 0;
            }
            else
            {
              m_CtxCache.Reset();
              curScore[j] = EvaluateString(string, m_CtxCache, m_GridCache, curSize[j]);

              
              if(curSize[j] > m_MaxArea)
                m_MaxArea = curSize[j];
            }
          }
        }
        //else
        //{
        //  scoreMap.insert(std::make_pair(0.0, j));
        //}
      }

      if(m_MaxArea > 0)
      {

        for(unsigned int i = 0; i<m_Parameters.genSize; ++i)
        {
          prevScore[i] *= pow(float(prevSize[i]), m_Parameters.weightSize);
          curScore[i] *= pow(float(curSize[i]), m_Parameters.weightSize);

          float coeffCur = (1.0 - (m_Parameters.weightGrowing / 2.0));
          float coeffPrev = m_Parameters.weightGrowing / 2.0;

          curScore[i] = coeffCur * curScore[i] + coeffPrev * prevScore[i];
          scoreMap.insert(std::make_pair(curScore[i], i));
        }
      }
      else
      {
        for(unsigned int i = 0; i<m_Parameters.genSize; ++i)
        {
          scoreMap.insert(std::make_pair(0, i));
        }
      }
      if(!scoreMap.empty())
      {
        grammars.clear();

        //m_BestCtx = m_CtxCache[scoreMap.rbegin()->second];
        LOG_INFO<<EXL_TEXT("Current best score :")<<scoreMap.rbegin()->first<<EXL_TEXT("\n");
        m_ScoreMap.swap(scoreMap);
      }
    }
  }
}

#endif