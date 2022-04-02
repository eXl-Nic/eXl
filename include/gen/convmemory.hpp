/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <gen/pattern.hpp>
#include <core/randomsetwalk.hpp>
#include <boost/container/flat_map.hpp>

namespace eXl
{
  class Random;

  template <unsigned int N, unsigned int TFieldSize, bool UseDictionnary = false>
  class ConvMemory
  {
  public:
    static const int FieldSize = TFieldSize;
    static const unsigned int RowSize = (2*(FieldSize - 1) + 1);
    static const unsigned int GridSize = RowSize * RowSize;

    ConvMemory()
    {
      if(!UseDictionnary)
      {
        m_PatternScores.resize(N, Vector<ScoreMapF>(GridSize));
        m_SymbolWheights.resize(N, 0.0);
      }
    }

    ConvMemory(Pattern<unsigned int> const& iSample)
    {
      if(!UseDictionnary)
      {
        m_PatternScores.resize(N, Vector<ScoreMapF>(GridSize));
        m_SymbolWheights.resize(N, 0.0);
      }
      AddPattern(iSample);
    }

    void AddPattern(Pattern<unsigned int> const& iSample, bool iRotateSym = true);

    void Generate(Pattern<unsigned int>& ioResult, Random& iRand, float iTemperature, int iIterations, bool iInitRand = false, Pattern<float> const* iTemp = NULL);

    typedef Map<uint32_t, float> WorkMap;
    typedef Map<Vec2i, WorkMap> OpenList;

    struct RemovedPot
    {
      Vec2i coord;
      uint32_t locOffset;
      uint32_t val;
    };

    struct InfPot
    {
      Vec2i coord;
      uint32_t locOffset;
      uint32_t val;
      float inf;
    };

    struct Operation
    {
      Vec2i m_Pos;
      uint32_t m_Val;
      WorkMap m_Pot;
      Vector<Vec2i> m_OpenedPositions;
      Vector<InfPot> m_Inf;
      Vector<RemovedPot> m_Removed;
    };

    struct ClosedList : Pattern<bool>
    {
      void SetSize(Vec2i const& iSize)
      {
        Pattern<bool>::SetSize(iSize);
        std::fill(GetBitmap().begin(), GetBitmap().end(), false);
      }
    };

    struct GenCtx
    {
      OpenList m_openList;
      Pattern<unsigned int> m_workArea;
      ClosedList m_closedList;

      Vector<Operation> m_Operations;
      Set<std::pair<Vec2i, uint32_t>> m_DeadEnds;

      void Backtrack()
      {
        if(!m_Operations.empty())
        {
          auto lastOp = std::move(m_Operations.back());
          m_Operations.pop_back();

          m_DeadEnds.insert(std::make_pair(lastOp.m_Pos, lastOp.m_Val));
          if(!m_Operations.empty())
          {
            m_openList.insert(std::make_pair(lastOp.m_Pos, std::move(lastOp.m_Pot)));
          }

          uint32_t offsetWk = m_workArea.GetOffset(lastOp.m_Pos);
          m_closedList.GetBitmap()[offsetWk] = false;

          for(auto const& inf : lastOp.m_Inf)
          {
            auto& cell = m_openList[inf.coord];

            cell[inf.val] -= inf.inf;
          }

          for(auto const& removed : lastOp.m_Removed)
          {
            auto& cell = m_openList[removed.coord];

            cell[removed.val] *= -1.0;
          }

          for(auto const& opened : lastOp.m_OpenedPositions)
          {
            m_openList.erase(opened);
          }
        }
      }

      bool ValidCandidate(ConvMemory& iMem, Vec2i const& iCoord, uint32_t iVal) const
      {
        if(m_DeadEnds.count(std::make_pair(iCoord, iVal)) > 0)
        {
          return false;
        }

        uint32_t offsetWk = m_workArea.GetOffset(iCoord);
      
        auto const& scores = iMem.m_PatternScores[iVal];
      
        unsigned int localOffset = 0;
        for(int i = -FieldSize + 1; i<FieldSize; ++i)
        {
          for(int j = -FieldSize + 1; j<FieldSize; ++j)
          {
            //Vec2i coord((iCoord.x + j + m_workArea.GetSize().x) % m_workArea.GetSize().x, 
            //  (iCoord.y + i + m_workArea.GetSize().y) % m_workArea.GetSize().y);
      
            Vec2i coord((iCoord.x + j), (iCoord.y + i));
            if(!(coord.x >= 0 && coord.x < m_workArea.GetSize().x
              && coord.y >= 0 && coord.y < m_workArea.GetSize().y))
            {
              continue;
            }
      
            uint32_t globOffset = m_workArea.GetOffset(coord);
      
            //Skip middle value;
            if(globOffset == offsetWk)
              continue;
      
            if(!m_closedList.GetBitmap()[globOffset])
            {
              auto openCellEntry = m_openList.find(coord);
              if(openCellEntry != m_openList.end())
              {
                WorkMap const& openCell = openCellEntry->second;
                bool foundCompat = false;
                for(auto iter = openCell.begin(); iter != openCell.end(); ++iter)
                {
                  auto foundPot = scores[localOffset].find(iter->first);
                  if(foundPot != scores[localOffset].end())
                  {
                    foundCompat = true;
                  }
                }
      
                if(!foundCompat)
                {
                  return false;
                }
              }
              ++localOffset;
            }
          }
        }
      
        return true;
      }


      void CollapseCell(ConvMemory& iMem, Vec2i const& iCoord, uint32_t iVal)
      {
        uint32_t offsetWk = m_workArea.GetOffset(iCoord);
        {
          Operation newOp;
          newOp.m_Pos = iCoord;
          newOp.m_Pot = std::move(m_openList[iCoord]);

          m_Operations.emplace_back(std::move(newOp));
          m_openList.erase(iCoord);

          m_closedList.GetBitmap()[offsetWk] = true;
        }

        Operation& curOp = m_Operations.back();

        m_workArea.GetBitmap()[offsetWk] = iVal;

        auto const& scores = iMem.m_PatternScores[iVal];

        unsigned int localOffset = 0;
        for(int i = -FieldSize + 1; i<FieldSize; ++i)
        {
          for(int j = -FieldSize + 1; j<FieldSize; ++j)
          {
            //Vec2i coord((iCoord.x + j + m_workArea.GetSize().x) % m_workArea.GetSize().x, 
            //  (iCoord.y + i + m_workArea.GetSize().y) % m_workArea.GetSize().y);

            Vec2i coord((iCoord.x + j), (iCoord.y + i));
            if(!(coord.x >= 0 && coord.x < m_workArea.GetSize().x
              && coord.y >= 0 && coord.y < m_workArea.GetSize().y))
            {
              continue;
            }

            uint32_t globOffset = m_workArea.GetOffset(coord);

            //Skip middle value;
            if(globOffset == offsetWk)
              continue;

            if(!m_closedList.GetBitmap()[globOffset])
            {
              auto cellInsertRes = m_openList.insert(std::make_pair(coord, WorkMap()));
              if(cellInsertRes.second)
              {
                curOp.m_OpenedPositions.push_back(coord);
                for(auto val : scores[localOffset])
                {
                  cellInsertRes.first->second.insert(val);
                }
              }
              else
              {
                WorkMap& openCell = cellInsertRes.first->second;
                auto iter = openCell.begin();
                while(iter != openCell.end())
                {
                  auto foundPot = scores[localOffset].find(iter->first);
                  if(foundPot != scores[localOffset].end() && iter->second > 0.0)
                  {
                    InfPot influenced;
                    influenced.coord = coord;
                    influenced.val = iter->first;
                    influenced.inf = foundPot->second;

                    curOp.m_Inf.push_back(influenced);
                    iter->second += foundPot->second;
                  }
                  else if(iter->second > 0.0)
                  {
                    iter->second *= -1.0;
                    //auto toDelete = iter;
                    //++iter;
                    //openCell.erase(toDelete);

                    RemovedPot removed;
                    removed.coord = coord;
                    removed.locOffset = localOffset;
                    removed.val = iter->first;
                    
                    curOp.m_Removed.push_back(removed);
                  }

                  ++iter;
                }
              }
              ++localOffset;
            }
          }
        }
      }

    };

    void Step(Pattern<unsigned int>& ioResult, Random& iRand, float iTemperature, GenCtx& ioCtx)
    {
      Vec2i coord;
      

      if(ioCtx.m_Operations.empty())
      {
        Vec2i resSize = ioResult.GetSize();
        unsigned int resVectSize = ioResult.GetBitmap().size();

        eXl_ASSERT_MSG(!UseDictionnary || m_Dict.size() >0, "Empty dict");

        ioCtx.m_workArea.SetSize(ioResult.GetSize());
        ioCtx.m_closedList.SetSize(ioResult.GetSize());

        coord = Vec2i(iRand.Generate() % ioResult.GetSize().x, iRand.Generate() % ioResult.GetSize().y);
        uint32_t bootstrapVal;
        if(UseDictionnary)
        {
          bootstrapVal = iRand.Generate() % m_Dict.size();
        }
        else
        {
          bootstrapVal = iRand.Generate() % N;
        }

        ioCtx.m_openList.insert(std::make_pair(coord, WorkMap()));
        
        ioCtx.CollapseCell(*this, coord, bootstrapVal);
      }
      else
      {
        if(ioCtx.m_openList.empty())
        {
          return ;
        }

        WorkMap potentials;
        {
          float totalScore = 0;
          Map<float,OpenList::iterator> cellToCollapseScoreMap;
          for(auto iter = ioCtx.m_openList.begin(); iter != ioCtx.m_openList.end(); ++iter)
          {
            float localScore = 0;
            for(auto& potential : iter->second)
            {
              if(potential.second > 0)
              {
                localScore += potential.second;
              }
            }

            cellToCollapseScoreMap.insert(std::make_pair(localScore, iter));
            totalScore += localScore;
          }

          OpenList::iterator cellToCollapse = cellToCollapseScoreMap.begin()->second;
        
          float choiceVal = (float(iRand() % 1000) / 1000.0) * totalScore;
          for(auto revIter = cellToCollapseScoreMap.rbegin(); revIter != cellToCollapseScoreMap.rend(); ++revIter)
          {
            if(revIter->first > choiceVal)
            {
              cellToCollapse = revIter->second;
              break;
            }
            else
            {
              choiceVal -= revIter->first;
            }
          }
          coord = cellToCollapse->first;
          potentials = cellToCollapse->second;
        }

        //if(potentials.empty())
        //{
        //  printf("Yikes_Nothing");
        //  ioCtx.Backtrack();
        //  return;
        //}

        float totalScore = 0;
        Map<float,uint32_t> potScoreMap;
        for(auto iter = potentials.begin(); iter!=potentials.end(); ++iter)
        {
          if(iter->second > 0.0 && ioCtx.ValidCandidate(*this, coord, iter->first))
          {
            potScoreMap.insert(std::make_pair(iter->second, iter->first));
            totalScore += iter->second;
          }
        }

        if(potScoreMap.empty())
        {
          //printf("Yikes_NoPot");
          ioCtx.Backtrack();
          return;
        }

        uint32_t newVal = potScoreMap.begin()->second;

        float choiceVal = (float(iRand() % 1000) / 1000.0) * totalScore;
        for(auto revIter = potScoreMap.rbegin(); revIter != potScoreMap.rend(); ++revIter)
        {
          if(revIter->first > choiceVal)
          {
            newVal = revIter->second;
            break;
          }
          else
          {
            choiceVal -= revIter->first;
          }
        }

        ioCtx.CollapseCell(*this, coord, newVal);
      }

      uint32_t globOffset = ioResult.GetOffset(coord);

      if(UseDictionnary)
      {
        for(unsigned int i = 0; i<ioResult.GetBitmap().size(); ++i)
        {
          ioResult.GetBitmap()[globOffset] = m_Dict[ioCtx.m_workArea.GetBitmap()[globOffset]];
        }
      }
      else
      {
        ioResult.GetBitmap()[globOffset] = ioCtx.m_workArea.GetBitmap()[globOffset];
      }
    }

  protected:


    typedef boost::container::flat_map<unsigned int, float> ScoreMapF;
    typedef boost::container::flat_map<unsigned int, unsigned int> ScoreMap;
    //typedef Vector<std::pair<unsigned int, float> > ScoreVect;
    Vector<Vector<ScoreMapF> > m_PatternScores;
    Vector<float> m_SymbolWheights;

    void _AddPattern(Pattern<unsigned int>const& iSample, bool iRotateSym)
    {
      uint32_t const dummyValue = UseDictionnary ? m_Dict.size() : N;

      Vector<ScoreMap> dummy(GridSize);
      Vector<Vector<ScoreMap > > patternScores(m_PatternScores.size(), dummy);
      Vector<Vector<Vector<unsigned int> > > absenceSampling;

      unsigned int const middleOffset = ((FieldSize - 1) * RowSize + FieldSize - 1);
      float totSym = iSample.GetSize().x * iSample.GetSize().y;
      unsigned int numPatterns = iRotateSym ? 8 : 1;
      Pattern<unsigned int> p[8];
      for (int y = 0; y < iSample.GetSize().y; y++) 
      {
        for (int x = 0; x < iSample.GetSize().x; x++)
        {

          ComputePatternReceptor(p[0], iSample, Vec2i(x, y));
          m_SymbolWheights[p[0].GetBitmap()[middleOffset]] += 1.0;
          if(iRotateSym)
          {
            p[1] = p[0].Rotate();
            p[2] = p[1].Rotate();
            p[3] = p[2].Rotate();
            p[4] = p[0].ReflectX();
            p[5] = p[1].ReflectX();
            p[6] = p[2].ReflectX();
            p[7] = p[3].ReflectX();
          }

          for (int k = 0; k < numPatterns; k++)
          {
            unsigned int value = p[k].GetBitmap()[middleOffset];

            if(value == dummyValue)
            {
              continue;
            }

            for (int offset = 0; offset < GridSize; ++offset)
            {
              if(offset != middleOffset)
              {
                unsigned int otherVal = p[k].GetBitmap()[offset];
                if((UseDictionnary && otherVal < m_Dict.size()) 
                  || (!UseDictionnary && otherVal < N))
                {
                  auto iter = patternScores[value][offset].insert(std::make_pair(otherVal, unsigned int(0))).first;
                  ++iter->second;
                }
              }
              else
                patternScores[value][offset].insert(std::make_pair(value, unsigned int(1)));
            }
          }
        }
      }


      float const gamma = FieldSize / sqrt(-2.0 * log(0.1));
      float const expWh = 1.0 / (gamma * sqrt(2 * Mathf::Pi()));
      float const varWh = -1.0 / (2 * gamma * gamma);
      for(unsigned int i = 0; i< patternScores.size(); ++i)
      {
        unsigned int localOffset = 0;
        for(int j = -FieldSize + 1; j< FieldSize; ++j)
        {
          for(int k = -FieldSize + 1; k< FieldSize; ++k)
          {
            float weight = expWh * expWh * exp((j * j) * varWh) * exp((k * k) * varWh);

            if(localOffset == middleOffset)
            {
              //m_SymbolWheights[i] /= totSym;
              //m_SymbolWheights[i] = 1.0;
              auto res = m_PatternScores[i][localOffset].insert(std::make_pair(i, 0.0f));
              res.first->second += weight;
            }
            else
            {
              float counter = 0.0;
              for(auto value : patternScores[i][localOffset])
              {
                //counter += value.second;
                auto res = m_PatternScores[i][localOffset].insert(std::make_pair(value.first, 0.0f));
                res.first->second += weight * value.second;
              }
              //for(auto& value : m_PatternScores[i][localOffset])
              //{
              //  value.second = (value.second / counter) * weight;
              //}
            }
            ++localOffset;
          }
        }
      }
    }

    unsigned int GetValue(unsigned int iOldValue, Random& iRand)
    {
      unsigned int newVal = iOldValue;
      if(UseDictionnary)
      {
        while(newVal == iOldValue)
          newVal = iRand.Generate() % m_Dict.size();
      }
      else
      {
        while(newVal == iOldValue)
          newVal = iRand.Generate() % N;
      }

      return newVal;
    }

    void ComputePatternReceptor(Pattern<unsigned int>& oPattern, Pattern<unsigned int> const& iSample, Vec2i const& iCoord);

    //typename WeightsMap::iterator ComputeReceptorIndex(Pattern<unsigned int> const& iReceptor, bool iCreate);

    //double ComputeEnergy(Pattern<unsigned int>& temp, Pattern<unsigned int> const& iSample, Vec2i const& iCoord);

    Vector<unsigned int> m_Dict;
    Map<unsigned int, unsigned int> m_RevDict;

  };

  template <unsigned int N, unsigned int TFieldSize, bool UseDictionnary>
  void ConvMemory<N, TFieldSize, UseDictionnary>::AddPattern(Pattern<unsigned int> const& iSample, bool iRotateSym = true)
  {
    if(UseDictionnary)
    {
      for(auto val : iSample.GetBitmap())
      {
        auto res = m_RevDict.insert(std::make_pair(val, m_Dict.size()));
        if(res.second)
        {
          m_Dict.push_back(val);
          m_PatternScores.push_back(Vector<ScoreMapF>(GridSize));
          m_SymbolWheights.push_back(0.0);
        }
      }
      Pattern<unsigned int> remSample(iSample.GetSize());
      for(unsigned int i = 0; i < iSample.GetBitmap().size(); ++i)
      {
        remSample.GetBitmap()[i] = m_RevDict[iSample.GetBitmap()[i]];
      }
      _AddPattern(remSample, iRotateSym);
    }
    else
      _AddPattern(iSample, iRotateSym);
  }

  template <unsigned int N, unsigned int TFieldSize, bool UseDictionnary>
  void ConvMemory<N, TFieldSize, UseDictionnary>::ComputePatternReceptor(Pattern<unsigned int>& oPattern, Pattern<unsigned int> const& iSample, Vec2i const& iCoord)
  {
    oPattern.SetSize(2*(Vec2i(FieldSize, FieldSize) - Vec2i::ONE) + Vec2i::ONE);
    unsigned int localOffset = 0;
    for(int i = -FieldSize + 1; i<FieldSize; ++i)
    {
      for(int j = -FieldSize + 1; j<FieldSize; ++j)
      {
        Vec2i coord((iCoord.x + j), (iCoord.y + i));
        if(coord.x >= 0 && coord.x < iSample.GetSize().x
          && coord.y >= 0 && coord.y < iSample.GetSize().y) 
        {
          oPattern.GetBitmap()[localOffset] = iSample.GetBitmap()[iSample.GetOffset(coord)];
        }
        else
        {
          oPattern.GetBitmap()[localOffset] = UseDictionnary ? m_Dict.size() : N;
        }

        //Vec2i coord((iCoord.x + j + iSample.GetSize().x) % iSample.GetSize().x, 
        //  (iCoord.y + i + iSample.GetSize().y) % iSample.GetSize().y);
        //
        //oPattern.GetBitmap()[localOffset] = iSample.GetBitmap()[iSample.GetOffset(coord)];

        ++localOffset;
      }
    }
  }

  template <unsigned int N, unsigned int TFieldSize, bool UseDictionnary>
  void ConvMemory<N, TFieldSize, UseDictionnary>::Generate(Pattern<unsigned int>& ioResult, Random& iRand, float iTemperature, int iIterations, bool iInitRand, Pattern<float> const* iTemp = NULL)
  {
    Vec2i resSize = ioResult.GetSize();
    unsigned int resVectSize = ioResult.GetBitmap().size();
    if(iTemp != NULL)
      eXl_ASSERT_MSG(iTemp->GetSize() == resSize,"Wrong size for temp map");

    eXl_ASSERT_MSG(!UseDictionnary || m_Dict.size() >0, "Empty dict");

    if(iInitRand)
    {
      if(UseDictionnary)
      {
        for(unsigned int i = 0; i<ioResult.GetBitmap().size(); ++i)
          ioResult.GetBitmap()[i] = iRand.Generate() % m_Dict.size();
      }
      else
      {
        for(unsigned int i = 0; i<ioResult.GetBitmap().size(); ++i)
          ioResult.GetBitmap()[i] = iRand.Generate() % N;
      }
    }
    else if(UseDictionnary)
    {
      for(unsigned int i = 0; i<ioResult.GetBitmap().size(); ++i)
      {
        auto iter = m_RevDict.find(ioResult.GetBitmap()[i]);
        if(iter != m_RevDict.end())
        {
          ioResult.GetBitmap()[i] = iter->second;
        }
        else
        {
          eXl_ASSERT_MSG(false, "Wrong Val");
          ioResult.GetBitmap()[i] = 0;
        }
      }
    }



    Vector<float> dummyVect(UseDictionnary ? m_Dict.size() : N, 0.0);
    Vector<Vector<float> > scoreMap(resVectSize, dummyVect);
    //Vector<Vector<float> > scoreMap(GridSize, dummyVect);

    for(unsigned int i = 0; i<scoreMap.size(); ++i)
    {
      scoreMap[i][ioResult.GetBitmap()[i]] = 1.0;
    }
    //Vector<Vector<float> > scoreMapWk(GridSize, m_SymbolWheights);
    Vector<Vector<float> > scoreMapWk(resVectSize, dummyVect);
    //dummyVect.clear();
    //dummyVect.resize(UseDictionnary ? m_Dict.size() : N, 1.0);


    Pattern<unsigned int> receptor;

    for(unsigned int iter = 0; iter<iIterations; ++iter)
    {
      //scoreMapWk.clear();
      //scoreMapWk.resize(GridSize, m_SymbolWheights);
      //scoreMapWk.resize(resVectSize, dummyVect);
      //scoreMap.clear();
      //scoreMap.resize(GridSize, dummyVect);
      //

      unsigned int scoreOffset = 0;
      Vec2i baseCoord(iRand.Generate() % resSize.x, iRand.Generate() % resSize.y);
      //
      //for(int yy = -2*FieldSize + 1; yy<2*FieldSize; ++yy)
      //{
      //  for(int xx = -2*FieldSize + 1; xx<2*FieldSize; ++xx)
      //  {
      //    for(unsigned int i = 0; i<scoreMap.size(); ++i)
      //    {
      //      scoreMap[scoreOffset][ioResult.Get(baseCoord + Vec2i(xx, yy))] = 1.0;
      //      ++scoreOffset;
      //    }
      //  }
      //}
      //
      Vector<float> score(m_SymbolWheights);
      //score[ioResult.Get(baseCoord)] = iTemperature;
      scoreOffset = 0;
      //for(int yy = -FieldSize + 1; yy<FieldSize; ++yy)
      {
        //for(int xx = -FieldSize + 1; xx<FieldSize; ++xx)
        {
          //for(unsigned int i = 0; i<scoreMap[scoreOffset].size(); ++i)
          {
            //float prob1 = scoreMap[scoreOffset][i];
            //if(val > 0.0)
            {
              Vec2i coord = baseCoord ;//+ vector2i(xx, yy);
              ComputePatternReceptor(receptor, ioResult, coord);

              unsigned int localOffset = 0;
              for(int y = -FieldSize + 1; y<FieldSize; ++y)
              {
                for(int x = -FieldSize + 1; x<FieldSize; ++x)
                {
                  Vec2i revCoord = Vec2i(FieldSize - 1 - x, FieldSize - 1 - y);
                  unsigned int revOffset = revCoord.y * RowSize + revCoord.x; 
                  //unsigned int otherScoreOffset = y * RowSize + x; 
                  //for(unsigned int j = 0; j<scoreMap[scoreOffset].size(); ++i)
                  {
                    //float prob = scoreMap[scoreOffset][j];
                    float prob = 1.0;
                    //if(prob > 0.0)
                    {
                      //unsigned int value = j;

                      unsigned int value = receptor.GetBitmap()[localOffset];
                      if((UseDictionnary && value < m_Dict.size()) 
                        || (!UseDictionnary && value < N))
                      {
                        auto const& curScore = m_PatternScores[value][revOffset];
                        for(auto const& entry : curScore)
                        {
                          //scoreMapWk[coord.x + coord.y * resSize.x][entry.first] *= curProb * entry.second;
                          //score[entry.first] *= pow((1.0 + entry.second * prob), log(m_SymbolWheights.size()));
                          score[entry.first] += entry.second;
                        }
                      }
                      ++localOffset;
                    }
                  }
                }
              }
            }
          }
          ++scoreOffset;
        }
      }

      std::multimap<float, unsigned int> choiceMap;

      scoreOffset = 0;
      //for(int yy = -FieldSize + 1; yy<FieldSize; ++yy)
      {
        //for(int xx = -FieldSize + 1; xx<FieldSize; ++xx)
        {
          //Vec2i coord = baseCoord + vector2i(xx, yy);
          float counter = 0.0;

          for(unsigned int j = 0; j < score.size(); ++j)
          {
            float locScore = score[j];
            locScore = pow(locScore, 1.0 / iTemperature);
            counter += locScore;
            if(locScore > 0.0)
            {
              choiceMap.insert(std::make_pair(locScore, j));
            }
          }
          if(!choiceMap.empty())
          {
            unsigned int oldVal = ioResult.Get(baseCoord);
            unsigned int newVal = oldVal;
            //ioResult.Set(baseCoord, choiceMap.rbegin()->second);

            float val = counter * (float(iRand.Generate() % 1000) / 1000.0);
            for(auto iter : choiceMap)
            {
              if(val < iter.first)
              {
                newVal = iter.second;
                break;
              }
              else
              {
                val -= iter.first;
              }
            }

            if(score[newVal] > score[oldVal])
            {
              ioResult.Set(baseCoord, newVal);
            }
            else
            {
              float delta = score[oldVal] / score[newVal];
              float acceptance = exp(-delta / (iTemperature));
              if(iRand.Generate() % 1000 < (acceptance * 1000))
                ioResult.Set(baseCoord, newVal);
            }
          }
          ++scoreOffset;
        }
      }
      /*
      unsigned int bootstrap = iRand.Generate() % resVectSize;

      for(unsigned int y = 0; y<resSize.y; ++y)
      {
      for(unsigned int x = 0; x<resSize.x; ++x)
      {
      Vec2i baseCoord(x,y);
      auto const& values = scoreMap[ioResult.GetOffset(baseCoord)];
      for(unsigned int value = 0; value < values.size(); ++value)
      {
      float curProb = values[value];
      if(curProb > 0.0)
      {
      Vector<ScoreVect> const& map = m_PatternScores[value];
      unsigned int localOffset = 0;
      for(int i = -FieldSize + 1; i<FieldSize; ++i)
      {
      for(int j = -FieldSize + 1; j<FieldSize; ++j)
      {
      Vec2i coord((baseCoord.x + j + resSize.x) % resSize.x, 
      (baseCoord.y + i + resSize.y) % resSize.y);
      for(auto const& entry : map[localOffset])
      {
      float& val = scoreMapWk[coord.x + coord.y * resSize.x][entry.first];
      if(val == 0.0)
      val = pow((1.0 + entry.second * curProb), log(m_SymbolWheights.size()));
      else
      val *= pow((1.0 + entry.second * curProb), log(m_SymbolWheights.size()));
      }
      ++localOffset;
      }
      }
      }
      }
      }
      }

      for(unsigned int i = 0; i<resVectSize; ++i)
      {
      //float counter = 0.0;
      //for(unsigned int j = 0; j < scoreMap[i].size(); ++j)
      //{
      //  counter += scoreMapWk[i][j];
      //}
      //if(counter > 0.0)
      {
      for(unsigned int j = 0; j < scoreMap[i].size(); ++j)
      {
      scoreMap[i][j] = scoreMapWk[i][j] ;
      }
      }
      //else
      //{
      //  //eXl_ASSERT_MSG(false, "problem");
      //  scoreMap[i] = dummyVect;
      //  scoreMap[i][ioResult.GetBitmap()[i]] = 1.0;
      //}
      }
      //scoreMapWk.clear();
      //scoreMapWk.resize(resVectSize, m_SymbolWheights); 
      //scoreMap[bootstrap] = dummyVect;
      //scoreMap[bootstrap][ioResult.GetBitmap()[bootstrap]] = 1.0;
      */
    }

    /*
    std::multimap<float, unsigned int> choiceMap;
    for(unsigned int i = 0; i<ioResult.GetBitmap().size(); ++i)
    {
    float counter = 0.0;
    choiceMap.clear();
    for(unsigned int j = 0; j < scoreMap[i].size(); ++j)
    {
    float score = scoreMap[i][j];
    score = pow(score, 1.0 / iTemperature);
    counter += score;
    if(score > 0.0)
    {
    choiceMap.insert(std::make_pair(score, j));
    }
    }
    if(!choiceMap.empty())
    {
    //ioResult.GetBitmap()[i] = choiceMap.rbegin()->second;
    unsigned int oldVal = ioResult.GetBitmap()[i];
    unsigned int newVal = oldVal;

    float val = counter * (float(iRand.Generate() % 1000) / 1000.0);
    for(auto iter : choiceMap)
    {
    if(val < iter.first)
    {
    newVal = iter.second;
    break;
    }
    else
    {
    val -= iter.first;
    }
    }

    if(scoreMap[i][newVal] > scoreMap[i][oldVal])
    {
    ioResult.GetBitmap()[i]= newVal;
    }
    else
    {
    float delta = scoreMap[i][oldVal] / scoreMap[i][newVal];
    float acceptance = exp(-delta / (iTemperature));
    if(iRand.Generate() % 1000 < (acceptance * 1000))
    ioResult.GetBitmap()[i] = newVal;
    }
    }
    }
    */
    if(UseDictionnary)
    {
      for(unsigned int i = 0; i<ioResult.GetBitmap().size(); ++i)
      {
        ioResult.GetBitmap()[i] = m_Dict[ioResult.GetBitmap()[i]];
      }
    }
  }

}
