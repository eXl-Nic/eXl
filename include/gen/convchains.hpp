/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <gen/pattern.hpp>
#include <core/randomsetwalk.hpp>
#include <boost/optional.hpp>

namespace eXl
{
  class Random;

  template <unsigned int N>
  struct NearestPow2;

  template <>
  struct NearestPow2<1>
  {
    static const unsigned int Pow = 1;
  };

  template <unsigned int N>
  struct NearestPow2
  {
    static const unsigned int Pow = NearestPow2<N / 2>::Pow + 1;
  };

  template <unsigned int N, unsigned int Size>
  struct CompactStorage
  {
    static const unsigned int ArraySize = (NearestPow2<N>::Pow * Size) / 32 + ((NearestPow2<N>::Pow * Size) % 32 == 0 ? 0 : 1);
    unsigned int Data[ArraySize];

    void Build(Pattern<unsigned int> const& iSample);
    void Extract(Pattern<unsigned int>& oSample) const;
  };

  template <unsigned int N, unsigned int FieldSize, bool UseDictionnary = false>
  class ConvChains
  {
  public:

    static const unsigned int GridSize = FieldSize * FieldSize;
    typedef CompactStorage<N - 1, GridSize> Storage;

    struct iequal_to : std::binary_function<Storage,Storage, bool>
    {
      bool operator()(Storage const& x, Storage const& y) const;
    };

    struct ihash : std::unary_function<Storage, std::size_t>
    {
      std::size_t operator()(Storage const& x) const;
    };

    ConvChains(bool iToroidal = false) : m_Toroidal(iToroidal) 
    {
      if(UseDictionnary)
      {
        m_Dict.push_back(0xFFFFFFFF);
        m_RevDict[0xFFFFFFFF] = 0;
      }
    }

    ConvChains(Pattern<unsigned int> const& iSample, bool iToroidal = false) : m_Toroidal(iToroidal) 
    { 
      if(UseDictionnary)
      {
        m_Dict.push_back(0xFFFFFFFF);
        m_RevDict[0xFFFFFFFF] = 0;
      }
      AddPattern(iSample); 
    }

    void AddPattern(Pattern<unsigned int> const& iSample, bool iRotateSym = true);

    void Generate(Pattern<unsigned int>& ioResult, Random& iRand, float iTemperature, int iIterations, bool iInitRand = false, Pattern<float> const* iTemp = nullptr);

    void Evaluate(Pattern<unsigned int> const& iPattern, Vector<double>& oRef, Vector<double>& oScore, bool iRotateSym = true) const;
    
    struct PatternInfo
    {
      uint32_t weight = 0;
      bool isBorder = false;
    };

    typedef UnorderedMap<Storage, PatternInfo, ihash, iequal_to> WeightsMap;


    struct WaveState
    {
      struct CellState
      { 
        Vector<uint32_t> comp[4];
        Vector<bool> possible;

        int sumOfOnes;
        float sumOfWeights;
        float sumOfWeightsLogWeight;
        float entropy;
      };

      bool Outside(Vector2i& ioPos)
      {
        auto const& wkAreaSize = m_State.GetSize();

        if(m_Toroidal)
        {
          Vector2i coord((ioPos.X() + wkAreaSize.X()) % wkAreaSize.X(), 
            (ioPos.Y() + wkAreaSize.Y()) % wkAreaSize.Y());
          ioPos = coord;
          return false;
        }
        else
        {
          if(ioPos.X() < 0 || ioPos.X() >= (wkAreaSize.X() /*- FieldSize*/)
          || ioPos.Y() < 0 || ioPos.Y() >= (wkAreaSize.Y() /*- FieldSize*/))
          {
            return true;
          }
          return false;
        }
      }

      void InitPropagator()
      {
        auto agrees = [](Pattern<uint32_t> const& iP1, Pattern<uint32_t> const& iP2, int dx, int dy)
        {
          int xmin = dx < 0 ? 0 : dx;
          int xmax = dx < 0 ? dx + FieldSize : FieldSize;
          int ymin = dy < 0 ? 0 : dy;
          int ymax = dy < 0 ? dy + FieldSize : FieldSize;
          for (int y = ymin; y < ymax; y++) 
          {
            for (int x = xmin; x < xmax; x++) 
            {
              if (iP1[Vector2i(x, y)] != iP2[Vector2i(x - dx, (y - dy))]) 
              {
                return false;
              }
            }
          }
          return true;
        };

        int DX[4] = {1, -1, 0, 0};
        int DY[4] = {0, 0, 1, -1};

        Vector<Pattern<uint32_t>> extracted(numSymbols, Pattern<uint32_t> (Vector2i(FieldSize, FieldSize)));

        for (int sym = 0; sym < numSymbols; ++sym)
        {
          m_PatternDict[sym]->first.Extract(extracted[sym]);
        }

        for (int d = 0; d < 4; d++)
        {
          for (int sym = 0; sym < numSymbols; ++sym)
          {
            Pattern<uint32_t> const& p1 = extracted[sym];
            //List<int> list = new List<int>();
            Vector<uint32_t> list;
            uint32_t count = 0;
            for (int sym2 = 0; sym2 < numSymbols; ++sym2) 
            {
              Pattern<uint32_t> const& p2 = extracted[sym2];

              if (agrees(p1, p2, DX[d], DY[d]))
              {
                ++count;
                list.push_back(sym2);
              }
            }
            //propagator[d][t] = new int[list.Count];
            //for (int c = 0; c < list.Count; c++) 
            //{
            //  propagator[d][t][c] = list[c];
            //}
            m_Propagator[d][sym] = std::move(list);
          }
        }
      }


      void Init(ConvChains const& iMem, Vector2i const& iSize)
      {
        m_Toroidal = iMem.m_Toroidal;

        numSymbols = iMem.m_WeightsMap.size();

        CellState defaultCell;
        defaultCell.possible.resize(numSymbols, true);
        for(uint32_t i = 0; i<4; ++i)
        {
          defaultCell.comp[i].resize(numSymbols, 0);
          m_Propagator[i].resize(numSymbols);
        }


        m_State.SetSize(iSize, defaultCell);
        m_CollapsedState.SetSize(iSize, numSymbols);

      
        for(auto iter = iMem.m_WeightsMap.begin(); iter != iMem.m_WeightsMap.end(); ++iter)
        {
          m_PatternDict.push_back(iter);

          m_SumOfWeights += iter->second.weight;
          m_WeightLogWeights.push_back(iter->second.weight * log(iter->second.weight));
          m_SumOfWeightLogWeights += m_WeightLogWeights.back();
        }

        InitPropagator();

        m_StartingEntropy = log(m_SumOfWeights) - m_SumOfWeightLogWeights / m_SumOfWeights;

      }

      boost::optional<bool> Observe(Random& iRand)
      {
        uint32_t const stateSize = m_State.GetSize().X() * m_State.GetSize().Y();

        float min = 1.0e3;
        int argmin = -1;
        Vector2i selectedPos;
        uint32_t offset = 0;
        for (int y = 0; y < m_State.GetSize().Y(); ++y)
        {
          for (int x = 0; x < m_State.GetSize().X(); ++x)
          {
            if (Outside(Vector2i(x, y)))
            {
              ++offset;
              continue;
            }

            auto const& cellState = m_State[offset];

            int amount = cellState.sumOfOnes;

            if (amount == 0) 
            {
              return false;
            }

            float entropy = cellState.entropy;
            if (amount > 1 && entropy <= min)
            {
              double noise = 1.0e-11 * (iRand() % 10000);
              if (entropy + noise < min)
              {
                min = entropy + noise;
                selectedPos = Vector2i(x, y);
                argmin = offset;
              }
            }
            ++offset;
          }
        }

        //if (argmin == -1)
        {
          for (int i = 0; i < stateSize; ++i) 
          {
            for (int sym = 0; sym < numSymbols; ++sym) 
            {
              if (m_State.GetBitmap()[i].possible[sym]) 
              { 
                m_CollapsedState.GetBitmap()[i] = sym; 
                break; 
              }
            }
          }
          if (argmin == -1)
          {
            return true;
          }
        }

        float distTotSum = 0;
        Multimap<float, uint32_t> distribution;
        for (int sym = 0; sym < numSymbols; sym++)
        {
          if(m_State.GetBitmap()[argmin].possible[sym])
          {
            distribution.insert(std::make_pair(m_PatternDict[sym]->second.weight, sym));
            distTotSum += m_PatternDict[sym]->second.weight;
          }
        }

        eXl_ASSERT(!distribution.empty());

        uint32_t selectedSym = distribution.begin()->second;
        //int r = distribution.Random(random.NextDouble());
        float val = (float(iRand() % 1000) / 1000.0) * distTotSum;
        for(auto revIter = distribution.rbegin(); revIter != distribution.rend(); ++revIter)
        {
          if(revIter->first > val)
          {
            selectedSym = revIter->second;
            break;
          }
          else
          {
            val -= revIter->first;
          }
        }

        auto const& possibilities = m_State.GetBitmap()[argmin].possible;
        for (int sym = 0; sym < numSymbols; sym++)
        {
          if (possibilities[sym] != (sym == selectedSym)) 
          {
            Ban(selectedPos, sym);
          }
        }

        return boost::none;
      }

      void Ban(Vector2i pos, uint32_t sym)
      {
        auto& cellState = m_State[pos];
        cellState.possible[sym] = false;
        
        for (int dir = 0; dir < 4; dir++)
        {
          cellState.comp[dir][sym] = 0;
        }

        m_Stack.push_back(std::make_pair(pos, sym));

        cellState.sumOfOnes -= 1;
        if(cellState.sumOfOnes == 0)
        {
          printf("Termination reached at (%i,%i)", pos.X(), pos.Y());
        }
        cellState.sumOfWeights -= m_PatternDict[sym]->second.weight;
        cellState.sumOfWeightsLogWeight -= m_WeightLogWeights[sym];

        float sum = cellState.sumOfWeights;
        cellState.entropy = log(sum) - cellState.sumOfWeightsLogWeight / sum;
      }

      void Propagate()
      {
        while (!m_Stack.empty())
        {
          auto cell = m_Stack.back();
          m_Stack.pop_back();

          Vector2i const& pos = cell.first;

          uint32_t const& sym = cell.second;

          for (int dim = 0; dim < 2; ++dim)
          {
            for (int signIdx = 0; signIdx < 2; ++signIdx)
            {
              Vector2i otherPos = pos;
              otherPos.m_Data[dim] += 1 - 2*signIdx;

              if(Outside(otherPos))
              {
                continue;
              }
              else
              {
                //int i2 = x2 + y2 * FMX;

                uint32_t dirIdx = dim * 2 + signIdx;

                auto& p = m_Propagator[dirIdx][sym];

                CellState& otherCell = m_State[otherPos];

                for (uint32_t otherSym : p)
                {
                  uint32_t& comp = otherCell.comp[dirIdx][otherSym];

                  comp--;

                  if (comp == 0)
                  {
                    Ban(otherPos, otherSym);
                  }
                }
              }
            }
          }
        }
      }

      void Clear()
      {
        uint32_t localOffset = 0;
        for (int y = 0; y < m_State.GetSize().Y(); ++y)
        {
          for (int x = 0; x < m_State.GetSize().X(); ++x)
          {
            CellState& state = m_State.GetBitmap()[localOffset];

            for (int sym = 0; sym < numSymbols; ++sym)
            {
              state.possible[sym] = true;

              for (int d = 0; d < 4; d++)
              {
                state.comp[d][sym] = state.possible[sym] ? m_Propagator[s_Opposite[d]][sym].size() : 0;
              }

            }

            state.sumOfOnes = numSymbols;
            state.sumOfWeights = m_SumOfWeights;
            state.sumOfWeightsLogWeight = m_SumOfWeightLogWeights;
            state.entropy = m_StartingEntropy;

            ++localOffset;
          }
        }
        if(!m_Toroidal)
        {
          for (int y = 0; y < (int)m_State.GetSize().Y() - FieldSize; ++y)
          {
            for (int x = 0; x < (int)m_State.GetSize().X() - FieldSize; ++x)
            {
              for (int sym = 0; sym < numSymbols; ++sym)
              {
                if(m_PatternDict[sym]->second.isBorder)
                {
                  Ban(Vector2i(x,y), sym);
                }
              }
            }
          }
          Propagate();
        }
      }

      uint32_t s_Opposite[4] = {1, 0, 3, 2};

      Vector<Vector<uint32_t>> m_Propagator[4];

      Pattern<CellState> m_State;
      Pattern<uint32_t> m_CollapsedState;

      Vector<double> m_WeightLogWeights;

      uint32_t numSymbols;
      bool m_Toroidal;
      
      float m_SumOfWeights = 0;
      float m_SumOfWeightLogWeights = 0;
      float m_StartingEntropy = 0;

      uint32_t m_RestartCounter = 0;

      Vector<typename WeightsMap::const_iterator> m_PatternDict;
      Vector<std::pair<Vector2i, uint32_t>> m_Stack;
    };

    void Wave(Pattern<unsigned int>& ioResult, Random& iRand, uint32_t numIter, WaveState& ioState, bool iInit = false)
    {
      if(iInit)
      {
        ioState.Clear();
      }

      eXl_ASSERT(ioResult.GetSize() == ioState.m_State.GetSize());

      for (uint32_t l = 0; l < numIter; l++)
      {
        auto result = ioState.Observe(iRand);
        if (!result)
        {
          ioState.Propagate();
        }
        else if(!result.get())
        {
          ioState.Clear();
          ioState.m_RestartCounter++;
          printf("Restarted %i \n", ioState.m_RestartCounter);
        }
      }

      uint32_t globOffset = 0;
      for (int y = 0; y < ioState.m_State.GetSize().Y(); ++y)
      {
        for (int x = 0; x < ioState.m_State.GetSize().X(); ++x)
        {
          Vector2i curPos(x,y);

          uint32_t sym = ioState.m_CollapsedState.GetBitmap()[globOffset];
          if(sym != ioState.numSymbols)
          {
            Pattern<uint32_t> pattern(Vector2i(FieldSize, FieldSize));
            ioState.m_PatternDict[sym]->first.Extract(pattern);
            //ioResult[curPos] = m_Dict[pattern[Vector2i::ZERO]];
            for (int ly = 0; ly < FieldSize; ++ly) 
            {
              for (int lx = 0; lx < FieldSize; ++lx) 
              {
                Vector2i localPos(lx, ly);
                Vector2i otherPos = curPos;
                otherPos += Vector2i (lx, ly);
                if(otherPos.X() >=0 && otherPos.X() < ioState.m_State.GetSize().X()
                && otherPos.Y() >=0 && otherPos.Y() < ioState.m_State.GetSize().Y())
                {
                  ioResult[otherPos] = m_Dict[pattern[localPos]];
                }
              }
            }
          }

          ++globOffset;
        }
      }

      //return true;
    }

    Image GetPatternImage()
    {
      uint32_t dimX = Mathf::Round(sqrt(m_WeightsMap.size()));
      uint32_t dimY = m_WeightsMap.size() / dimX;
      if(m_WeightsMap.size() % dimX != 0)
      {
        ++dimX;
      }

      Image::Size imageSize(dimX * (FieldSize + 1) - 1, dimY * (FieldSize + 1) - 1);

      Vector<WeightsMap::iterator> patternDict;
      Multimap<uint32_t, uint32_t> patternSort;
      
      for(auto iter = m_WeightsMap.begin(); iter != m_WeightsMap.end(); ++iter)
      {
        patternDict.push_back(iter);
        patternSort.insert(std::make_pair(iter->second.weight, patternDict.size() - 1));
      }

      Image outImg(nullptr, imageSize, Image::RGBA, Image::Char, 1);
      memset(outImg.GetImageData(), 0, outImg.GetByteSize());

      Vector2i curPos;

      Pattern<uint32_t> pattern(Vector2i::ONE * FieldSize);
      for(auto rIter = patternSort.rbegin(); rIter != patternSort.rend(); ++rIter)
      {
        patternDict[rIter->second]->first.Extract(pattern);

        for(uint32_t y = 0; y<FieldSize; ++y)
        {
          for(uint32_t x = 0; x<FieldSize; ++x)
          {
            Vector2i locPos(x,y);
            Vector2i globPos = curPos + locPos;
            *((uint32_t*)outImg.GetPixel(globPos.Y(), globPos.X())) = pattern[locPos] == 1 ? 0 : 0xFFFFFFFF;
          }
        }

        if(curPos.X() + FieldSize < imageSize.X())
        {
          curPos.X() += FieldSize + 1;
        }
        else
        {
          curPos.X() = 0;
          curPos.Y() += FieldSize + 1;
        }
      }

      return outImg;
    }

  protected:

    Vector<unsigned int> m_TempBuffer;

    WeightsMap m_WeightsMap;

    void _AddPattern(Pattern<unsigned int> const& iSample, bool iRotateSym);
    
    unsigned int GetValue(unsigned int iOldValue, Random& iRand);

    void ComputePatternReceptor(Pattern<unsigned int>& oPattern, Pattern<unsigned int> const& iSample, Vector2i const& iCoord) const;

    typename WeightsMap::iterator ComputeReceptorIndex(Pattern<unsigned int> const& iReceptor, bool iCreate);

    typename WeightsMap::iterator ComputeReceptorIndex(Pattern<unsigned int> const& iReceptor) const;

    double ComputeEnergy(Pattern<unsigned int>& temp, Pattern<unsigned int> const& iSample, Vector2i const& iCoord);
    
    Vector<unsigned int> m_Dict;
    Map<unsigned int, unsigned int> m_RevDict;
    bool m_Toroidal;

  };

#include <gen/convchains.inl>

}