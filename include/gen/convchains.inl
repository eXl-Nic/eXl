template <unsigned int N, unsigned int Size>
void CompactStorage<N, Size>::Build(Pattern<unsigned int> const& iSample)
{
  memset(Data, 0, sizeof(Data));
  unsigned int offset32 = 0;
  unsigned int offsetArray = 0;

  for(unsigned int i = 0; i<iSample.GetBitmap().size(); ++i)
  {
    unsigned int val = iSample.GetBitmap()[i];
    Data[offsetArray] |= val << offset32;

    offset32 += NearestPow2<N>::Pow;

    if(offset32 >= 32)
    {
      offset32 -= 32;
      ++offsetArray;
      if(offset32 != 0)
      {
        Data[offsetArray] |= val >> (NearestPow2<N>::Pow - offset32);
      }
    }
  }
}

template <unsigned int N, unsigned int Size>
void CompactStorage<N, Size>::Extract(Pattern<unsigned int>& oSample) const
{
  unsigned int offset32 = 0;
  unsigned int offsetArray = 0;

  for(unsigned int i = 0; i<oSample.GetBitmap().size(); ++i)
  {
    oSample.GetBitmap()[i] = (Data[offsetArray] >> offset32) & ((1<<NearestPow2<N>::Pow) - 1);

    offset32 += NearestPow2<N>::Pow;

    if(offset32 >= 32)
    {
      offset32 -= 32;
      ++offsetArray;
      if(offset32 != 0)
      {
        oSample.GetBitmap()[i] |= ((Data[offsetArray] & ((1 << offset32) - 1)) << (NearestPow2<N>::Pow - offset32)) ;
      }
    }
  }
}

template <unsigned int N, unsigned int FieldSize, bool UseDictionnary>
bool ConvChains<N, FieldSize, UseDictionnary>::iequal_to::operator()(typename ConvChains<N, FieldSize, UseDictionnary>::Storage const& x, typename ConvChains<N, FieldSize, UseDictionnary>::Storage const& y) const
{
  for(unsigned int i = 0; i<Storage::ArraySize; ++i)
  {
    if(x.Data[i] != y.Data[i])
    {
      return false;
    }
  }

  return true;
}

template <unsigned int N, unsigned int FieldSize, bool UseDictionnary>
std::size_t ConvChains<N, FieldSize, UseDictionnary>::ihash::operator()(typename ConvChains<N, FieldSize, UseDictionnary>::Storage const& x) const
{
  std::size_t seed = 0;

  for(unsigned int i = 0; i<Storage::ArraySize; ++i)
  {
    boost::hash_combine(seed, x.Data[i]);
  }

  return seed;
}

template <unsigned int N, unsigned int FieldSize, bool UseDictionnary>
void ConvChains<N, FieldSize, UseDictionnary>::AddPattern(Pattern<unsigned int> const& iSample, bool iRotateSym)
{
  if(UseDictionnary)
  {
    for(auto val : iSample.GetBitmap())
    {
      auto res = m_RevDict.insert(std::make_pair(val, m_Dict.size()));
      if(res.second)
      {
        m_Dict.push_back(val);
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
  {
    _AddPattern(iSample, iRotateSym);
  }
}

template <unsigned int N, unsigned int FieldSize, bool UseDictionnary>
void ConvChains<N, FieldSize, UseDictionnary>::_AddPattern(Pattern<unsigned int> const& iSample, bool iRotateSym)
{
  unsigned int numPatterns = iRotateSym ? 8 : 1;

  int patternEndX = iSample.GetSize().x /*- (m_Toroidal ? 0 : FieldSize)*/;
  int patternEndY = iSample.GetSize().y /*- (m_Toroidal ? 0 : FieldSize)*/;

  for (int y = 0; y < patternEndY; y++) 
  {
    for (int x = 0; x < patternEndX; x++)
    {
      Pattern<unsigned int> p[8];

      p[0] = Pattern<unsigned int>(Vec2i(FieldSize, FieldSize));
      ComputePatternReceptor(p[0], iSample, Vec2i(x, y));

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
        auto iter = ComputeReceptorIndex(p[k], true);
        iter->second.weight += 1;
        //weights[ComputeReceptorIndex(p[k])] += 1;
      }
    }
  }
}

template <unsigned int N, unsigned int FieldSize, bool UseDictionnary>
unsigned int ConvChains<N, FieldSize, UseDictionnary>::GetValue(unsigned int iOldValue, Random& iRand)
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

template <unsigned int N, unsigned int FieldSize, bool UseDictionnary>
void ConvChains<N, FieldSize, UseDictionnary>::ComputePatternReceptor(Pattern<unsigned int>& oPattern, Pattern<unsigned int> const& iSample, Vec2i const& iCoord) const
{
  oPattern.SetSize(Vec2i(FieldSize, FieldSize));
  unsigned int localOffset = 0;
  for(unsigned int i = 0; i<FieldSize; ++i)
  {
    for(unsigned int j = 0; j<FieldSize; ++j)
    {
      Vec2i coord;
      if(m_Toroidal)
      {
        coord = Vec2i((iCoord.x + j + iSample.GetSize().x) % iSample.GetSize().x, 
            (iCoord.y + i + iSample.GetSize().y) % iSample.GetSize().y);

        oPattern.GetBitmap()[localOffset] = iSample.GetBitmap()[iSample.GetOffset(coord)];
      }
      else
      {
        coord = iCoord + Vec2i(j, i);
        if(coord.x < 0 || coord.x >= iSample.GetSize().x
        || coord.y < 0 || coord.y >= iSample.GetSize().y)
        {
          //coord.x = Mathi::Clamp(coord.x, 0, iSample.GetSize().x - 1);
          //coord.y = Mathi::Clamp(coord.y, 0, iSample.GetSize().y - 1);
          oPattern.GetBitmap()[localOffset] = 0;
        }
        else
        {
          oPattern.GetBitmap()[localOffset] = iSample.GetBitmap()[iSample.GetOffset(coord)];  
        }
      }
      
      
      ++localOffset;
    }
  }
}

template <unsigned int N, unsigned int FieldSize, bool UseDictionnary>
typename ConvChains<N, FieldSize, UseDictionnary>::WeightsMap::iterator ConvChains<N, FieldSize, UseDictionnary>::ComputeReceptorIndex(Pattern<unsigned int> const& iReceptor, bool iCreate)
{
  Storage myStorage;
  myStorage.Build(iReceptor);

  bool border = false;

  for(auto& sym : iReceptor.GetBitmap())
  {
    if(sym == 0)
    {
      border = true;
    }
  }

  auto testPattern = iReceptor;
  testPattern.GetBitmap()[0] = 0xFFFFFFFF;
  myStorage.Extract(testPattern);
  
  eXl_ASSERT(iReceptor.GetBitmap() == testPattern.GetBitmap());

  auto iter = m_WeightsMap.find(myStorage);
  if(iter == m_WeightsMap.end() && iCreate)
  {
    iter = m_WeightsMap.insert(std::make_pair(myStorage, PatternInfo())).first;
    if(border)
    {
      iter->second.isBorder = true;
    }
  }
  return iter;
  //unsigned int localOffset = 0;
  //unsigned int index = 0;
  //for (unsigned int y = 0; y < iReceptor.GetSize().y; y++) 
  //{
  //  for (unsigned int x = 0; x < iReceptor.GetSize().x; x++) 
  //  {
  //    index += iReceptor.GetBitmap()[localOffset] ? 1 << localOffset : 0;
  //    ++localOffset;
  //  }
  //}
  //return index;
}

template <unsigned int N, unsigned int FieldSize, bool UseDictionnary>
typename ConvChains<N, FieldSize, UseDictionnary>::WeightsMap::iterator ConvChains<N, FieldSize, UseDictionnary>::ComputeReceptorIndex(Pattern<unsigned int> const& iReceptor) const
{
  return const_cast<ConvChains*>(this)->ComputeReceptorIndex(iReceptor, false);
}

template <unsigned int N, unsigned int FieldSize, bool UseDictionnary>
double ConvChains<N, FieldSize, UseDictionnary>::ComputeEnergy(Pattern<unsigned int>& temp, Pattern<unsigned int> const& iSample, Vec2i const& iCoord)
{
  double value = 1.0;
  for (int y = iCoord.y - FieldSize + 1; y <= iCoord.y + FieldSize - 1; ++y) 
  {
    for (int x = iCoord.x - FieldSize + 1; x <= iCoord.x + FieldSize - 1; ++x) 
    {
      ComputePatternReceptor(temp, iSample, Vec2i(x,y));
      auto iter = ComputeReceptorIndex(temp, false);
      if(iter == m_WeightsMap.end())
        value *= 0.1;
      else
        value*= iter->second.weight;
      //value *= iWeights[ComputeReceptorIndex(temp)];
    }
  }
  return value;
}

template <unsigned int N, unsigned int FieldSize, bool UseDictionnary>
void ConvChains<N, FieldSize, UseDictionnary>::Evaluate(Pattern<unsigned int> const& iPattern, Vector<double>& oRef, Vector<double>& oScore, bool iRotateSym) const
{
  Vec2i inSize = iPattern.GetSize();

  Pattern<unsigned int> tempPattern(inSize);
  if(UseDictionnary)
  {
    for(unsigned int i = 0; i<iPattern.GetBitmap().size(); ++i)
    {
      auto iter = m_RevDict.find(iPattern.GetBitmap()[i]);
      if(iter != m_RevDict.end())
      {
        tempPattern.GetBitmap()[i] = iter->second;
      }
      else
      {
        eXl_ASSERT_MSG(false, "Wrong Val");
        tempPattern.GetBitmap()[i] = 0;
      }
    }
  }
  else
  {
    tempPattern = iPattern;
  }

  double num = 0.0;

  oScore.clear();
  oRef.clear();
  std::map<unsigned int, unsigned int> patternIndex;

  for(auto patternIter = m_WeightsMap.begin(), patternIterEnd = m_WeightsMap.end(); patternIter != patternIterEnd; ++patternIter)
  {
    patternIndex.insert(std::make_pair(patternIter->second, (unsigned int)oScore.size()));
    oScore.push_back(0);
    oRef.push_back(patternIter->second);
    num += patternIter->second;
  }
  num = Mathd::Max(1.0, num);
  for(unsigned int i = 0;i<oRef.size(); ++i)
  {
    oRef[i] /= num;
  }
  num = 0.0;
  unsigned int numPatterns = iRotateSym ? 8 : 1;
  Pattern<unsigned int> p[8];

  for(unsigned int y = 0; y<inSize.y; ++y)
  {
    for(unsigned int x = 0; x<inSize.x; ++x)
    {
      ComputePatternReceptor(p[0], tempPattern, Vec2i(x, y));

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
        auto iter = ComputeReceptorIndex(p[k]);
        if(iter != m_WeightsMap.end())
        {
          auto iterIndex = patternIndex.find(iter->second);
          if(iterIndex != patternIndex.end())
          {
            oScore[iterIndex->second] += 1.0;
          }
        }
        num += 1.0;
      }
    }
  }
  num = Mathd::Max(1.0, num);
  for(unsigned int i = 0;i<oScore.size(); ++i)
  {
    oScore[i] /= num;
  }
}

template <unsigned int N, unsigned int FieldSize, bool UseDictionnary>
void ConvChains<N, FieldSize, UseDictionnary>::Generate(Pattern<unsigned int>& ioResult, Random& iRand, float iTemperature, int iIterations, bool iInitRand, Pattern<float> const* iTemp = nullptr)
{
  if(iTemp != nullptr)
    eXl_ASSERT_MSG(iTemp->GetSize() == ioResult.GetSize(),"Wrong size for temp map");

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

  Pattern<unsigned int> temp;
  for (int k = 0; k < iIterations; k++)
  {
    Vec2i coord(iRand.Generate() % ioResult.GetSize().x, iRand.Generate() % ioResult.GetSize().y);
    unsigned int offset = ioResult.GetOffset(coord);


    double p = ComputeEnergy(temp, ioResult, coord);

    unsigned int curVal = ioResult.GetBitmap()[offset];
    //unsigned int newVal = GetValue(curVal, iRand);

    RandomSetWalk walk(UseDictionnary ? m_Dict.size() : N, iRand);
    unsigned int newVal;
    unsigned int curNewVal = curVal;
    double oldProb = 1.0;
    while(walk.Next(newVal))
    {
      if(newVal == curVal)
        continue;

      ioResult.GetBitmap()[offset] = newVal;

      double q = ComputeEnergy(temp, ioResult, coord);
      double prob = pow(q / p, 1.0 / iTemperature);
      double threshold = double(iRand.Generate() % 1000) / 1000.0;

      if(iTemp)
        prob *= iTemp->Get(coord);

      if(iTemperature * (prob / oldProb - 0.5) > threshold)
      {
        curNewVal = newVal;
        oldProb = prob;
      }

    }
    ioResult.GetBitmap()[offset] = curNewVal;
  }

  if(UseDictionnary)
  {
    for(unsigned int i = 0; i<ioResult.GetBitmap().size(); ++i)
    {
      ioResult.GetBitmap()[i] = m_Dict[ioResult.GetBitmap()[i]];
    }
  }
}