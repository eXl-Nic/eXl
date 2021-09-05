/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <gen/mcmcsynthesis.hpp>

#include <math/geometrytraits.hpp>
#include <math/mathtools.hpp>
#include <math/segment.hpp>
#include <core/random.hpp>
#include <gen/voronoigr.hpp>

//#include <boost/pool/pool_alloc.hpp>

#include <boost/geometry/index/rtree.hpp>
#include <boost/random.hpp>

#include <core/stream/streamer.hpp>
#include <core/stream/unstreamer.hpp>

#include <fstream>

#include <core/stream/jsonstreamer.hpp>
#include <core/stream/jsonunstreamer.hpp>

#include <core/image/image.hpp>
#include <core/image/imagestreamer.hpp>

namespace eXl
{
  IMPLEMENT_SERIALIZE_METHODS(MCMC2D::PlacedElement);
  IMPLEMENT_SERIALIZE_METHODS(MCMC2D::Element);
  IMPLEMENT_SERIALIZE_METHODS(MCMC2D::LearnExample);
  IMPLEMENT_SERIALIZE_METHODS(MCMC2D::LearnParams);

  MCMC2D::FullInteraction MCMC2D::FullInteraction::Make(InputBuilder& iBuilder, SamplingCell const& iCell, ElementInter const& iInter)
  {
    FullInteraction fullInter;
    unsigned int oneHotIdx;
    Vector2d dir1 = iInter.dir1;
    Vector2d dir2 = iInter.dir2;

    if (iInter.elem2 == -1)
    {
      oneHotIdx = iBuilder.BuildOneHot(iCell.elem1, -1, MathTools::GetAngleFromVec(dir2));
    }
    else if (iCell.elem1 <= iInter.elem2)
    {
      oneHotIdx = iBuilder.BuildOneHot(iCell.elem1, iInter.elem2 - iCell.elem1, MathTools::GetAngleFromVec(dir2));
    }
    else
    {
      dir2 = MathTools::GetLocal(iInter.dir2, Vector2d::UNIT_X);
      dir1 = MathTools::GetLocal(iInter.dir2, iInter.dir1 * -1);

      oneHotIdx = iBuilder.BuildOneHot(iInter.elem2, iCell.elem1 - iInter.elem2, MathTools::GetAngleFromVec(dir2));
    }

    fullInter.dist = iInter.distance;
    fullInter.dir1 = dir1;
    fullInter.dir2 = dir2;
    fullInter.oneHotIdx = oneHotIdx;

    return fullInter;
  }

  void MCMC2D::FullInteraction::Perturbate(eXl::Random& iRand, float iRadius, float iAngRange)
  {
    boost::random::normal_distribution<float> distrib;
    RandomWrapper randw(&iRand);
    float const randAngle = Mathf::Clamp(distrib(randw) * (iAngRange * 0.33), -iAngRange, iAngRange);
    float const randDist = Mathf::Clamp(Mathf::Abs(distrib(randw)) * iRadius * 0.33, -iRadius, iRadius);

    Vector2f newDir(dir1.X() * Mathf::Cos(randAngle) - dir1.Y() * Mathf::Sin(randAngle), dir1.X() * Mathf::Sin(randAngle) + dir1.Y() * Mathf::Cos(randAngle));

    dist = Mathf::Max(randDist + dist, 0);
    dir1 = MathTools::ToDVec(newDir);
  }

  Err MCMC2D::PlacedElement::Serialize(Serializer iStreamer)
  {
    iStreamer.BeginStruct();
    iStreamer.PushKey("Position");
    iStreamer &= m_Pos;
    iStreamer.PopKey();
    iStreamer.PushKey("Angle");
    iStreamer &= m_Angle;
    iStreamer.PopKey();
    iStreamer.PushKey("Element");
    iStreamer &= m_Element;
    iStreamer.PopKey();
    iStreamer.PushKey("SubElement");
    iStreamer &= m_ShapeNum;
    iStreamer.PopKey();
    iStreamer.EndStruct();

    return Err::Success;
  }

  Err MCMC2D::LearnExample::Serialize(Serializer iStreamer)
  {
    iStreamer.BeginStruct();
    iStreamer.PushKey("Shape");
    iStreamer &= m_Shape;
    iStreamer.PopKey();
    iStreamer.PushKey("Elements");
    iStreamer &= m_Elements;
    iStreamer.PopKey();
    iStreamer.EndStruct();

    return Err::Success;
  }

  Err MCMC2D::LearnParams::Serialize(Serializer iStreamer)
  {
    iStreamer.BeginStruct();
    iStreamer.PushKey("QuantileCull");
    iStreamer &= m_QuantileCull;
    iStreamer.PopKey();
    iStreamer.PushKey("MaxDist");
    iStreamer &= m_MaxDist_;
    iStreamer.PopKey();
    iStreamer.EndStruct();

    return Err::Success;
  }

  Err MCMC2D::RunParams::Serialize(Serializer iStreamer)
  {
    iStreamer.BeginStruct();
    iStreamer.PushKey("Shape");
    iStreamer &= m_Shape;
    iStreamer.PopKey();
    iStreamer.PushKey("NumIter");
    iStreamer &= m_NumIter;
    iStreamer.PopKey();
    iStreamer.PushKey("Placed");
    iStreamer &= m_Placed;
    iStreamer.PopKey();
    iStreamer.PushKey("Static");
    iStreamer &= m_Static;
    iStreamer.PopKey();
    iStreamer.EndStruct();

    return Err::Success;
  }

  Err MCMC2D::Element::Serialize(Serializer iStreamer)
  {
    iStreamer.BeginStruct();
    iStreamer.PushKey("RelDensity");
    iStreamer &= m_RelDensity;
    iStreamer.PopKey();
    iStreamer.PushKey("AbsDensity");
    iStreamer &= m_AbsDensity;
    iStreamer.PopKey();
    iStreamer.PushKey("Turn");
    iStreamer &= m_Turn;
    iStreamer.PopKey();
    iStreamer.PushKey("GridX");
    iStreamer &= m_GridX;
    iStreamer.PopKey();
    iStreamer.PushKey("GridY");
    iStreamer &= m_GridY;
    iStreamer.PopKey();
    iStreamer.PushKey("Layer");
    iStreamer &= m_Layer;
    iStreamer.PopKey();
    iStreamer.PushKey("DirMethod");
    unsigned int method = m_DirMethod;
    iStreamer &= method;
    if (iStreamer.IsReading())
    {
      m_DirMethod = (DirectionMethod)method;
    }
    iStreamer.PopKey();
    iStreamer.PushKey("Shapes");
    iStreamer &= m_Shapes;
    iStreamer.PopKey();
    iStreamer.EndStruct();

    return Err::Success;
  }

  struct PlacedElementAndBox : MCMC2D::PlacedElement
  {
    PlacedElementAndBox()
    {
    }

    PlacedElementAndBox(Vector2i const& iPos, float iAngle, int iElement, int iShapeNum, AABB2Di const& iBox)
    {
      m_Pos = iPos;
      m_Angle = iAngle;
      m_Element = iElement;
      m_ShapeNum = iShapeNum;
      curBox = iBox;
    }

    PlacedElementAndBox(MCMC2D::PlacedElement const& iOther, AABB2Di const& iBox)
    {
      m_Pos = iOther.m_Pos;
      m_Angle = iOther.m_Angle;
      m_Element = iOther.m_Element;
      m_ShapeNum = iOther.m_ShapeNum;
      curBox = iBox;
    }

    AABB2Di curBox;
  };


  typedef std::list<PlacedElementAndBox/*, allocator*/> PlacedElemList;

  template <typename T>
  using SpIdxValue = std::pair<eXl::AABB2Di, T>;

  typedef SpIdxValue<PlacedElemList::iterator> SpIdxValList;

  template <typename T>
  using PointIndex = boost::geometry::index::rtree< SpIdxValue<T>, /*boost::geometry::index::rstar<16, 4>*/boost::geometry::index::linear<16, 4> >;

  bool GetNearestPlaneDir(Polygoni const& iRefPoly, Polygoni const& iOtherPoly, Vector2d& oDir, double& oDist)
  {
    bool found = false;

    Vector2d centroid = MathTools::ToDVec(iOtherPoly.GetAABB().GetCenter());
    oDist = Mathd::MAX_REAL;
    Vector2d chosenSeg[2];
    Vector2d prevPt = MathTools::ToDVec(iRefPoly.Border().back());
    for(unsigned int i = 0; i < iRefPoly.Border().size(); ++i)
    {
      Vector2d curPt = MathTools::ToDVec(iRefPoly.Border()[i]);
      if(curPt != prevPt)
      {
        Vector2d oDir;
        double curDist = Segmentd::NearestPointSeg(prevPt, curPt, centroid, oDir);
        if(curDist < oDist)
        {
          chosenSeg[0] = prevPt;
          chosenSeg[1] = curPt;
          oDist = curDist;
          //dir = oDir;
          found = true;
        }
      }
      prevPt = curPt;
    }
    auto perpDir = MathTools::Perp(chosenSeg[0] - chosenSeg[1]);
    perpDir.Normalize();
    double dotPerp = (chosenSeg[0] - centroid).Dot(perpDir);
    dotPerp = dotPerp >= 0.0 ? 1.0 : -1.0;
    oDir = perpDir * dotPerp;
    return found;
  }

  bool GetSeparatingPlaneDir(Polygoni const& iRefPoly, Polygoni const& iOtherPoly, Vector2d& oDir)
  {
    unsigned int numSep = 0;
    Vector2d curVec;

    Vector2d prevPt = MathTools::ToDVec(iRefPoly.Border().back());
    for(unsigned int i = 0; i < iRefPoly.Border().size(); ++i)
    {
      Vector2d curPt = MathTools::ToDVec(iRefPoly.Border()[i]);
      if(curPt != prevPt)
      {
        bool sepPlane = true;
        for(unsigned int j = 0; j < iOtherPoly.Border().size(); ++j)
        {
          if(!(Segmentd::IsLeft(prevPt, curPt, MathTools::ToDVec(iOtherPoly.Border()[i])) >= 0))
          {
            sepPlane = false;
            break;
          }
        }
        if(sepPlane)
        {
          Vector2d dir = MathTools::Perp(curPt - prevPt);
          dir.Normalize();
          curVec = curVec + dir;
          ++numSep;
        }
      }
      prevPt = curPt;
    }

    if(numSep > 0)
    {
      oDir = curVec / numSep;
      return true;
    }
    return false;
  }

  template <typename Handler, typename Index, typename ObjIdentity>
  bool MCMC2D::Query(Vector<Element> const& iElements, QueryCache<Index>& iCache, Handler& iHandler, ObjIdentity const& iIdent, AABB2Di const& iQueryBox, PlacedElement const& iCurElem, bool iToroidal, AABB2Di const& iSceneBox)
  {
    if(iToroidal && iQueryBox.Intersect(iSceneBox, 1))
    {
      AABB2Di boxes[4] = {iQueryBox};
      Vector2i offset[4];
      int numBoxes = 1;
      for(int dim = 0; dim < 2; ++dim)
      {
        int diffMin = iSceneBox.m_Data[0].m_Data[dim] - iQueryBox.m_Data[0].m_Data[dim];
        if(diffMin > 0)
        {
          for(int box = 0; box<numBoxes; ++box)
          {
            boxes[box + numBoxes] = boxes[box];
            offset[box + numBoxes] = offset[box];
            boxes[box].m_Data[0].m_Data[dim] = iSceneBox.m_Data[0].m_Data[dim];
          }

          for(int loopedBox = numBoxes; loopedBox<numBoxes * 2; ++loopedBox)
          {
            offset[loopedBox].m_Data[dim] = iSceneBox.GetSize().m_Data[dim];
            boxes[loopedBox].m_Data[0].m_Data[dim] = iSceneBox.m_Data[1].m_Data[dim] - diffMin;
            boxes[loopedBox].m_Data[1].m_Data[dim] = iSceneBox.m_Data[1].m_Data[dim];
          }

          numBoxes *= 2;
        }
        else
        {
          //Let's assume the sample box is not bigger in both directions...
          int diffMax = iQueryBox.m_Data[1].m_Data[dim] - iSceneBox.m_Data[1].m_Data[dim];
          if(diffMax > 0)
          {
            for(int box = 0; box<numBoxes; ++box)
            {
              boxes[box + numBoxes] = boxes[box];
              offset[box + numBoxes] = offset[box];
              boxes[box].m_Data[1].m_Data[dim] = iSceneBox.m_Data[1].m_Data[dim];
            }

            for(int loopedBox = numBoxes; loopedBox<numBoxes * 2; ++loopedBox)
            {
              offset[loopedBox].m_Data[dim] = -iSceneBox.GetSize().m_Data[dim];
              boxes[loopedBox].m_Data[0].m_Data[dim] = iSceneBox.m_Data[0].m_Data[dim];
              boxes[loopedBox].m_Data[1].m_Data[dim] = iSceneBox.m_Data[0].m_Data[dim] + diffMax;
            }
            numBoxes *= 2;
          }
        }
      }
      for(int queryIdx = 0; queryIdx < numBoxes; ++queryIdx)
      {

        PlacedElement offsetElement = iCurElem;
        offsetElement.m_Pos += offset[queryIdx];

        if(!Query(iElements, iCache, iHandler, iIdent, boxes[queryIdx], offsetElement))
          return false;
      }
      return true;
    }
    else
    {
      return Query(iElements, iCache, iHandler, iIdent, iQueryBox, iCurElem);
    }
  }

  template <typename Handler, typename Index, typename ObjIdentity>
  bool MCMC2D::Query(Vector<Element> const& iElements, QueryCache<Index>& iCache, Handler& iHandler, ObjIdentity const& iIdent, AABB2Di const& iQueryBox, PlacedElement const& iCurElem)
  {
    iCache.m_RetVal.clear();
    iCache.m_Index.query(boost::geometry::index::intersects(iQueryBox), std::back_inserter(iCache.m_RetVal));

    DirectionMethod dirM1;

    if(iCurElem.m_Element > 0)
    {
      int element = iCurElem.m_Element - 1;
      iCache.m_Poly1 = iElements[element].m_Shapes[iCurElem.m_ShapeNum];
      iCache.m_Poly1.Rotate(iCurElem.m_Angle);
      iCache.m_Poly1.Translate(iCurElem.m_Pos);
      dirM1 = iElements[element].m_DirMethod;
    }
    else
    {
      iCache.m_Poly1 = Polygoni(AABB2Di(iCurElem.m_Pos, Vector2i::ONE * 2));
      dirM1 = eCentroidal;
    }
    for(auto value : iCache.m_RetVal)
    {
      //if(!birth && value.second->m_Element > 0 && value.second == elemIter)
      //  continue;

      if(!iHandler.Accept(value, iIdent))
        continue;

      int otherElem = iHandler.GetElement(value);

      if(otherElem <= 0)
      {
        Vector2d dir;
        double dist = 0.0;
        Polygoni const& curWall = iCache.m_Walls[otherElem * -1];

        GetNearestPlaneDir(curWall, iCache.m_Poly1, dir, dist);

        if(!iHandler.Handle(iCache.m_Poly1, curWall, iCache.m_Poly3, dist, dir, dir * -1.0, iCurElem.m_Element, iCurElem.m_Angle, 0, 0.0, true))
          return false;
      }
      else
      {
        float otherAngle = iHandler.GetAngle(value);

        Vector2i otherPos = iHandler.GetPos(value);
        int subElem = iHandler.GetSubElement(value);
        iCache.m_Poly2 = iElements[otherElem - 1].m_Shapes[subElem];
        iCache.m_Poly2.Rotate(otherAngle);
        iCache.m_Poly2.Translate(otherPos);

        Vector2d centroidalDir = MathTools::ToDVec(iCache.m_Poly2.GetAABB().GetCenter() - iCache.m_Poly1.GetAABB().GetCenter());
        double dist = centroidalDir.Normalize();
        Vector2d dir1;
        Vector2d dir2;

        switch(dirM1)
        {
        case eCentroidal:
        case eMirrorOther:
          dir1 = centroidalDir;
          break;
        case eSeparatingPlanes:
        {
          if(!GetSeparatingPlaneDir(iCache.m_Poly1, iCache.m_Poly2, dir1))
          {
            dir1 = centroidalDir;
          }
        }
        break;
        case eNearestPlane:
        {
          double dummy;
          GetNearestPlaneDir(iCache.m_Poly1, iCache.m_Poly2, dir1, dummy);
        }
        break;

        }

        switch(iElements[otherElem - 1].m_DirMethod)
        {
        case eCentroidal:
          dir2 = centroidalDir * -1;
          break;
        case eSeparatingPlanes:
        {
          if(!GetSeparatingPlaneDir(iCache.m_Poly2, iCache.m_Poly1, dir2))
          {
            dir2 = centroidalDir * -1;
          }
        }
        break;
        case eNearestPlane:
        {
          double dummy;
          GetNearestPlaneDir(iCache.m_Poly2, iCache.m_Poly1, dir2, dummy);
        }
        break;
        case eMirrorOther:
          dir2 = dir1 * -1;
          break;
        }

        if(dirM1 == eMirrorOther)
          dir1 = dir2 * -1;

        bool sameLayer = iCurElem.m_Element > 0 ? (iElements[iCurElem.m_Element - 1].m_Layer & iElements[otherElem - 1].m_Layer) != 0 : true;
        if(!iHandler.Handle(iCache.m_Poly1, iCache.m_Poly2, iCache.m_Poly3, dist, dir1, dir2, iCurElem.m_Element, iCurElem.m_Angle, otherElem, otherAngle, sameLayer))
          return false;
      }
    }

    return true;
  }

  static const unsigned int k_RelDensitySampling = 1000;

  unsigned int MCMC2D::InputBuilder::ComputeNumElems(Vector<MCMC2D::Element> const& iElems, bool iToroidal)
  {
    // Borders
    unsigned int numInter = iToroidal ? 0 : iElems.size();

    for(unsigned int i = 0; i<iElems.size(); ++i)
    {
      auto const& elem = iElems[i];
      for(unsigned int j = 0; j<iElems.size() - i; ++j)
      {
        numInter += Mathi::Max(1, iElems[i +j].m_Turn);
        //numInter += 1;
      }
    }

    return numInter;
  }

  MCMC2D::InputBuilder::InputBuilder(Vector<MCMC2D::Element> const& iElems, bool iToroidal)
    : oneHotSize(ComputeNumElems(iElems, iToroidal))
    , toroidal(iToroidal)
  {
    interMap.resize(iElems.size());
    unsigned int curInter = iToroidal ? 0 : iElems.size();
    for(unsigned int i = 0; i< iElems.size(); ++i)
    {
      auto& otherElem = interMap[i];
      otherElem.resize(iElems.size() - i);
      for(unsigned int j = 0; j<iElems.size() - i; ++j)
      {
        unsigned int const numTurns = Mathi::Max(1, iElems[i + j].m_Turn);
        otherElem[j].first = curInter;
        otherElem[j].second = numTurns;
        curInter += numTurns;
        //curInter += 1;
      }
    }

    eXl_ASSERT(curInter == oneHotSize);
  }

  unsigned int MCMC2D::InputBuilder::BuildOneHot(unsigned int i, int j, float otherAngle) const
  {
    if(j < 0)
    {
      //eXl_ASSERT(!toroidal);
      return i;
    }
    else
    {
      unsigned int const elemOffset = interMap[i][j].first;
      //return elemOffset;

      unsigned int const numTurns = interMap[i][j].second;
          
      otherAngle += (Mathf::PI / numTurns);
      int div = otherAngle / (Mathf::PI * 2);
      if(otherAngle < 0)
      {
        --div;
      }
      otherAngle -= div * Mathf::PI * 2;

      return elemOffset + int((otherAngle / (Mathf::PI * 2)) * numTurns);
    }
  }

  struct RunHandler
  {
    RunHandler(
      MCMC2D::LearnedModel& iModel,
      MCMC2D::InputBuilder& iInputBuilder,
      MCMC2D::Debug* iDebug) 
      : m_Model(iModel)
      , m_InputBuilder(iInputBuilder)
      , m_Debug(iDebug){}

    inline int GetElement(SpIdxValList const& iVal)
    {
      return iVal.second->m_Element;
    }
    
    inline int GetSubElement(SpIdxValList const& iVal)
    {
      return iVal.second->m_ShapeNum;
    }
    
    inline float GetAngle(SpIdxValList const& iVal)
    {
      return iVal.second->m_Angle;
    }
    
    inline Vector2i const& GetPos(SpIdxValList const& iVal)
    {
      return iVal.second->m_Pos;
    }

    inline bool Accept(SpIdxValList const& iVal, PlacedElemList::iterator const& iIdent)
    {
      if(!m_Birth && iVal.second->m_Element > 0 && iVal.second == iIdent)
        return false;
      return true;
    }

    inline bool Handle(Polygoni const& iPoly1, Polygoni const& iPoly2, Vector<Polygoni>& iPoly3, float dist, Vector2d const& iDir1, Vector2d const& iDir2, unsigned int iElem, float iAngle, unsigned int iOtherElem, float iOtherAngle, bool iSameLayer)
    {
      MCMC2D::Debug::ElemInteraction interaction;

      interaction.m_Inter.dir1 = iDir1;
      interaction.m_Inter.dir2 = iDir2;
      interaction.m_Inter.elem2 = iOtherElem;

      float prevValue = m_AcceptanceRate;

      Vector2f baseMine(Mathf::Cos(iAngle), Mathf::Sin(iAngle));

      Vector2f dir1 = Vector2f(iDir1.X(), iDir1.Y());
      dir1.Normalize();

      dir1 = MathTools::GetLocal(baseMine, dir1);

      float centroidalLen = dist;

      bool overlaps = false;
      const bool swapId = iElem > iOtherElem;

      if(iSameLayer)
      {
        overlaps |= iPoly1.GetAABB().IsInside(iPoly2.GetAABB());
        overlaps |= iPoly2.GetAABB().IsInside(iPoly1.GetAABB());
        overlaps |= iPoly1.GetAABB().Intersect(iPoly2.GetAABB());
        overlaps &= boost::geometry::intersects(iPoly1, iPoly2) || boost::geometry::overlaps(iPoly1, iPoly2);
      }

      if(!overlaps)
      {
        float len = iSameLayer ? boost::geometry::distance(iPoly1, iPoly2) : centroidalLen;

        interaction.m_Inter.distance = len;
        if(iOtherElem > 0)
        {
          unsigned int const idx0 = swapId ? iOtherElem - 1 : iElem - 1;
          unsigned int const idx1 = swapId ? iElem - iOtherElem : iOtherElem - iElem;

          Vector2f baseOther(Mathf::Cos(iOtherAngle), Mathf::Sin(iOtherAngle));

          Vector2f dir2 = MathTools::GetLocal(baseMine, baseOther);

          dir2.Normalize();

          if(swapId)
          {
            dir2 = MathTools::GetLocal(dir2, Vector2f::UNIT_X);
            dir1 = MathTools::GetLocal(dir2, dir1 * -1);
          }

          MCMC2D::FullInteraction inter;
          inter.oneHotIdx = m_InputBuilder.BuildOneHot(idx0, idx1, MathTools::GetAngleFromVec(dir2));

          //if(len < m_Model.GetMaxDist(inter.oneHotIdx))
          {
            inter.dir1 = MathTools::ToDVec(dir1);
            inter.dir2 = MathTools::ToDVec(dir2);
            inter.dist = len;

            MCMC2D::FullInteraction grad;
            //m_AcceptanceRate *= m_Net.Eval(inter, m_outGrad.size() >= 3 ? &grad : nullptr, m_InputBuilder.BuildOneHot(idx0, idx1, MathTools::GetAngleFromVec(dir2)));
            interaction.m_Result = m_Model.Sample(inter);
            m_AcceptanceRate *= interaction.m_Result;

            //if(idx0 == 0 && idx1 == 1 && len < 10.0 && Mathf::Abs(dir1.m_Y - 1.0) < 0.1)
            //{
            //  float diff = prevValue / m_AcceptanceRate;
            //  if(Mathf::Abs(iOtherAngle - (Mathf::PI * 0.5)) < Mathf::ZERO_TOLERANCE
            //  || Mathf::Abs(iOtherAngle - (Mathf::PI * 1.5)) < Mathf::ZERO_TOLERANCE)
            //  {
            //    printf("Vertical : %f -> %f\n", len, diff);
            //  }
            //  else
            //  {
            //    printf("Horizontal : %f -> %f\n", len, diff);
            //  }
            //}

            if(m_outGrad.size() >= 3)
            {
              Vector2f gradient(dir1.X() + grad.dir1.X(), dir1.Y() + grad.dir2.X());
              gradient *= (len + grad.dist);
              gradient -= dir1 * len;
              if(swapId)
              {
                gradient = baseOther * gradient.X() + MathTools::GetPerp(baseOther) * gradient.Y();
                m_MoveGrad += MathTools::ToDVec(gradient);
              }
              else
              {
                gradient = baseMine * gradient.X() + MathTools::GetPerp(baseMine) * gradient.Y();
                m_MoveGrad -= MathTools::ToDVec(gradient);
              }
            }
          }
          //else if(diskAcc < Mathf::EPSILON)
          //{
          //  m_AcceptanceRate = -INFINITY;
          //}
        }
        else
        {
          //if(diskProb < -Mathf::EPSILON)
          MCMC2D::FullInteraction inter;
          inter.dir1 = MathTools::ToDVec(dir1);
          inter.dist = len;
          inter.oneHotIdx = m_InputBuilder.BuildOneHot(iElem - 1, -1, 0.0);

          if(len < m_Model.GetMaxDist(inter.oneHotIdx))
          {
            MCMC2D::FullInteraction grad;
            //m_AcceptanceRate *= m_Net.Eval(inter, m_outGrad.size() >= 3 ? &grad : nullptr, m_InputBuilder.BuildOneHot(iElem - 1, -1, 0.0));

            m_AcceptanceRate *= m_Model.Sample(inter);

            if(m_outGrad.size() >= 3)
            {
              Vector2f gradient(dir1.X() + grad.dir1.X(), dir1.Y() + grad.dir2.X());
              gradient *= (len + grad.dist);
              gradient -= dir1 * len;

              gradient = baseMine * gradient.X() + MathTools::GetPerp(baseMine) * gradient.Y();
              m_MoveGrad -= MathTools::ToDVec(gradient);
            }
          }
          //else if(diskAcc < Mathf::EPSILON)
          //{
          //  m_AcceptanceRate = -INFINITY;
          //}
        }
      }
      else
      {
        interaction.m_Inter.distance = 0.0;
        interaction.m_Result = 0.0;
        //int totArea = iPoly1.Area();
        //int interArea = 0;
        //for(auto const& piece : inter)
        //{
        //  interArea += piece.Area();
        //}
        //float overlapRej = 1.0 / float(interArea);
        //m_AcceptanceRate *= overlapRej;
        m_AcceptanceRate = 0.0;
      }

      if(m_Debug)
      {
        m_Debug->m_Ops.back().m_Interaction.push_back(interaction);
      }

      return m_AcceptanceRate > 0.0;//!std::isinf(-m_AcceptanceRate);
    }

    Vector<float> cacheInput;
    Vector2d       m_MoveGrad; 
    Vector<float>  m_outGrad;
    MCMC2D::LearnedModel& m_Model;
    MCMC2D::Debug* m_Debug;
    
    bool m_Birth;
    bool m_ComputeGrad;
    float m_AcceptanceRate;
    MCMC2D::InputBuilder& m_InputBuilder;
  };

//#define INDIVIDUAL_SPACE

  void MCMC2D::Run(Random& iRand, RunParams& iParams, LearnedModel* iModel, Debug* iDebug)
  {
    if(!iModel)
      return;

    //unsigned int maxTotElem = 0;
    //
    //if(iMaxElements != nullptr)
    //{
    //  if(iMaxElements->size() < m_Elements.size())
    //  {
    //    iMaxElements->resize(m_Elements.size(), 0);
    //  }
    //  for(auto curMax : *iMaxElements)
    //  {
    //    maxTotElem += curMax;
    //  }
    //}

    //std::vector<float> relDensity(m_Elements.size());

    auto const& m_Elements = iModel->GetElements();

    InputBuilder builder(m_Elements, iModel->IsToroidal());

    float iMaxDist = 0.0;
    for(unsigned int idx = 0; idx < builder.oneHotSize; ++idx)
    {
      iMaxDist = Mathf::Max(iMaxDist, iModel->GetMaxDist(idx));
    }

    std::map<unsigned int, unsigned int> relDensity;
    std::vector<AABB2Di> querySize(m_Elements.size());
    std::vector<float> elementCoeff(m_Elements.size(), 0.0);
    std::vector<unsigned int> elementsInst(m_Elements.size(), 0);
    double globArea = iParams.m_Shape.Area();
    float totalDensity = 0;
    int firstElem = -1;
    int lastElem  = -1;
    for(unsigned int i = 0; i<m_Elements.size(); ++i)
    {
      totalDensity += m_Elements[i].m_RelDensity;
      for(auto const& poly : m_Elements[i].m_Shapes)
        querySize[i].Absorb(poly.GetAABB());
      
      float maxQuery = iMaxDist;

      querySize[i].m_Data[0].X() -= maxQuery;
      querySize[i].m_Data[0].Y() -= maxQuery;
      querySize[i].m_Data[1].X() += maxQuery;
      querySize[i].m_Data[1].Y() += maxQuery;
    }

    unsigned int cumulatedDensity = 0;
    for(unsigned int i = 0; i<m_Elements.size(); ++i)
    {
      if(m_Elements[i].m_RelDensity > 0.0 && m_Elements[i].m_AbsDensity > 0.0 && !m_Elements[i].m_Shapes.empty())
      {
        double accumArea = 0;
        for(auto const& poly : m_Elements[i].m_Shapes)
          accumArea += poly.Area();
        accumArea /= m_Elements[i].m_Shapes.size();

        elementCoeff[i] = (globArea/accumArea) * m_Elements[i].m_AbsDensity;
        relDensity.insert(std::make_pair(cumulatedDensity, i));
        cumulatedDensity += k_RelDensitySampling * (m_Elements[i].m_RelDensity / totalDensity);
      }
    }

    if(relDensity.empty())
      return;

    firstElem = relDensity.begin()->second;
    lastElem = relDensity.rbegin()->second;

    unsigned int area = iParams.m_Shape.Area() /*/ (iQuerySize * iQuerySize * 0.25 )*/;

    Vector2i samplingAreaSize = iParams.m_Shape.GetAABB().GetSize();

    boost::random::uniform_real_distribution<double> distribX(0, samplingAreaSize.X());
    boost::random::uniform_real_distribution<double> distribY(0, samplingAreaSize.Y());

    if(samplingAreaSize.X() <= 0 || samplingAreaSize.Y() <= 0)
    {
      return;
    }

    RandomWrapper rand(&iRand);
    //PlacedElemList origElements;
    PlacedElemList elements;
    PlacedElemList wallList;
    std::vector<PlacedElemList::iterator> orderedPlaced;
    unsigned int numPlaced = 0;

    //PointIndex<PlacedElemList::iterator> idx;

    QueryCache<PointIndex<PlacedElemList::iterator> > queryCache;
    //RunHandler queryHandler(m_Distribs, m_BorderDistribs);

    RunHandler queryHandler(*iModel, builder, iDebug);
    //queryHandler.m_MaxDist = ;

    Polygoni outerBorder(AABB2Di(iParams.m_Shape.GetAABB().m_Data[0] - Vector2i::ONE * 50, iParams.m_Shape.GetAABB().GetSize() + Vector2i::ONE * 100));
    Vector<Polygoni> negShape;
    outerBorder.Difference(iParams.m_Shape, negShape);

    PlacedElemList::iterator iterOrig;
    {
      Vector<SpIdxValList> values;
      if(!iModel->IsToroidal())
      {
        Vector<Polygoni> trapezoids;
        for(auto poly : negShape)
        {
          trapezoids = poly.GetTrapezoids();
        }
        for(auto trap : trapezoids)
        {
          trap.RemoveUselessPoints();
          PlacedElementAndBox element(Vector2i::ZERO, 0.0, -int(queryCache.m_Walls.size()), 0, trap.GetAABB());
          wallList.push_back(element);
          SpIdxValList value = std::make_pair(trap.GetAABB(), std::prev(wallList.end()));
          values.push_back(value);
          queryCache.m_Walls.emplace_back(std::move(trap));
        }
      }
      for(int i = 0; i < iParams.m_Static.size(); ++i)
      {
        PlacedElement currentElement = iParams.m_Static[i];
        unsigned int element = currentElement.m_Element - 1;
        if(element < m_Elements.size() && currentElement.m_ShapeNum < m_Elements[element].m_Shapes.size())
        {
          queryCache.m_Poly1 = m_Elements[element].m_Shapes[currentElement.m_ShapeNum];
          queryCache.m_Poly1.Rotate(currentElement.m_Angle);
          queryCache.m_Poly1.Translate(currentElement.m_Pos);

          elements.push_back({currentElement, queryCache.m_Poly1.GetAABB()});
          orderedPlaced.push_back(std::prev(elements.end()));
          SpIdxValList spIdxVal = std::make_pair(queryCache.m_Poly1.GetAABB(), std::prev(elements.end()));
          values.push_back(spIdxVal);
          ++elementsInst[element];
          ++numPlaced;
        }
      }
      
      for(int i = 0; i < iParams.m_Placed.size(); ++i)
      {
        PlacedElement const& currentElement = iParams.m_Placed[i];
        unsigned int element = currentElement.m_Element - 1;
        if(element < m_Elements.size() && currentElement.m_ShapeNum < m_Elements[element].m_Shapes.size())
        {
          queryCache.m_Poly1 = m_Elements[element].m_Shapes[currentElement.m_ShapeNum];
          queryCache.m_Poly1.Rotate(currentElement.m_Angle);
          queryCache.m_Poly1.Translate(currentElement.m_Pos);

          elements.push_back({currentElement, queryCache.m_Poly1.GetAABB()});
          SpIdxValList spIdxVal = std::make_pair(queryCache.m_Poly1.GetAABB(), std::prev(elements.end()));
          values.push_back(spIdxVal);
          orderedPlaced.push_back(std::prev(elements.end()));
          ++elementsInst[element];
          ++numPlaced;
        }
      }
      iParams.m_Placed.clear();
      //Packing;
      queryCache.m_Index = PointIndex<PlacedElemList::iterator>(std::make_pair(values.begin(), values.end()));
    }
    
    Vector<Vector<float>> accRates(m_Elements.size());

    for(unsigned int step = 0; step<iParams.m_NumIter; ++step)
    {
      //if(step % 100 == 0)
      //{
      //  char buffer[128];
      //  //int val = iRand.Generate();
      //  sprintf(buffer,"Step : %i\n", step);
      //  OutputDebugStringA(buffer);
      //}
      unsigned int elemIdx;
      PlacedElemList::iterator elemIter;
      PlacedElementAndBox currentElement;
      unsigned int operation = 0;       
      if(/*iMaxElements != nullptr ||*/ orderedPlaced.empty())
      {
        operation = 0;
      }
      else
      {
        operation = (iRand.Generate() % 2);
      }

      bool death = (operation == 1) && orderedPlaced.size() > iParams.m_Static.size();
      bool birth = (operation == 0) || !death;
      //bool move =  (operation == 2) && orderedPlaced.size() > iParams.m_Static.size();
      bool move = false;

      bool manipulatedElem = false;

      if(birth)
      {
        do
        {
          auto densIter = relDensity.lower_bound((iRand.Generate() % k_RelDensitySampling));
          if(densIter == relDensity.end())
          {
            currentElement.m_Element = lastElem;
          }
          else if(densIter == relDensity.begin())
          {
            currentElement.m_Element = firstElem;
          }
          else
          {
            --densIter;
            currentElement.m_Element = densIter->second;
          }
        }while(0);

        currentElement.m_ShapeNum = (iRand.Generate() % m_Elements[currentElement.m_Element].m_Shapes.size());
        currentElement.m_Element++;

        bool inShape = false;
        unsigned int sampleCount = 0;

        do
        {
          ++sampleCount;

          //if(!orderedPlaced.empty() 
          //  && iModel->CanSuggestPosition() 
          //  //&& iParams.m_SuggestionFrequency > 0
          //  && (iRand() % 1000 < iParams.m_SuggestionFrequency * 1000)
          //  && iModel->SuggestPosition(iRand, builder, *orderedPlaced[(iRand.Generate() % orderedPlaced.size())], currentElement)
          //  )
          //{
          //}
          //else
          {
            currentElement.m_Pos = Vector2i(distribX(rand),distribY(rand));
            unsigned int turn = m_Elements[currentElement.m_Element - 1].m_Turn;
            currentElement.m_Angle = turn > 1 ? (iRand.Generate() % turn) * (Mathf::PI * 2 / float(turn)) : 0.0;
            currentElement.m_Pos += iParams.m_Shape.GetAABB().m_Data[0];
          }
          Element const& elemDef = m_Elements[currentElement.m_Element - 1];
          if(elemDef.m_GridX > 1)
          {
            currentElement.m_Pos.X() = currentElement.m_Pos.X() - Mathi::Mod(currentElement.m_Pos.X(), elemDef.m_GridX);
            currentElement.m_Pos.X() += elemDef.m_GridX / 2;
          }
          if(elemDef.m_GridY > 1)
          {
            currentElement.m_Pos.Y() = currentElement.m_Pos.Y() - Mathi::Mod(currentElement.m_Pos.Y(), elemDef.m_GridY);
            currentElement.m_Pos.Y() += elemDef.m_GridY / 2;
          }

          inShape = iParams.m_Shape.ContainsPoint(currentElement.m_Pos);
        }
        while(!inShape && sampleCount < 10000);

        if(!inShape)
        {
          continue;
        }
      }
      else
      {
        elemIdx = iParams.m_Static.size() + (iRand.Generate() % (orderedPlaced.size() - iParams.m_Static.size()));
        elemIter = orderedPlaced[elemIdx];
        currentElement = *elemIter;
      }

      unsigned int element = currentElement.m_Element - 1;
      //AABB2Di sampleBox(MathTools::ToDVec(origPt), Vector2d::ONE * 2 * iQuerySize + MathTools::ToDVec(m_Elements[element - 1].m_Shape.GetAABB().GetSize()));
      //Polygoni samplePoly(querySize[element]);
      //samplePoly.Rotate(currentElement.m_Angle);

      AABB2Df sampleBoxf;
      sampleBoxf.m_Data[0] = MathTools::ToFVec(querySize[element].m_Data[0]);
      sampleBoxf.m_Data[1] = MathTools::ToFVec(querySize[element].m_Data[1]);
      sampleBoxf.Rotate(currentElement.m_Angle);

      AABB2Di sampleBox;
      sampleBox.m_Data[0] = MathTools::ToIVec(sampleBoxf.m_Data[0]) + currentElement.m_Pos;
      sampleBox.m_Data[1] = MathTools::ToIVec(sampleBoxf.m_Data[1]) + currentElement.m_Pos;

      AABB2Di newBox = birth ? AABB2Di() : elemIter->curBox;
      AABB2Di prevBox = newBox;

      if(iDebug)
      {
        Debug::OpDesc newOp;
        newOp.m_Op = death ? Debug::Death : Debug::Birth;
        newOp.m_Element = currentElement;
        iDebug->m_Ops.push_back(newOp);
      }

      float prevAcceptRate = 0.0;

      queryHandler.m_Birth = birth;
      queryHandler.m_ComputeGrad = false;//birth;
      Vector2i prevPos = currentElement.m_Pos;
      bool superiorGrad = true;
      unsigned int const numGradMove = queryHandler.m_ComputeGrad ? 10 : 1;

      for(unsigned int gradMove = 0; gradMove < numGradMove && superiorGrad; ++gradMove)
      {
        prevBox = newBox;
        prevPos = currentElement.m_Pos;

        if(gradMove > 1)
        {
          double moveNorm = queryHandler.m_MoveGrad.Normalize();
          if(moveNorm > Mathf::ZERO_TOLERANCE)
          {
            //queryHandler.m_MoveGrad *= m_MaxDist * (0.1 - 0.05 * float(iRand.Generate() % 1000) / 1000.0 );

            currentElement.m_Pos += Vector2i(Mathf::Round(queryHandler.m_MoveGrad.X()), Mathf::Round(queryHandler.m_MoveGrad.Y())); 
          }
          else
          {
            break;
          }
        }

        prevAcceptRate = queryHandler.m_AcceptanceRate;
#ifdef INDIVIDUAL_SPACE
        queryHandler.m_AcceptanceRate = m_Elements[element].m_AbsDensity * globArea / ((elementsInst[element] + (birth ? 1 : 0)));
#else
        queryHandler.m_AcceptanceRate = m_Elements[element].m_AbsDensity * globArea / ((numPlaced + (birth ? 1 : 0)));
#endif

        queryHandler.m_MoveGrad = Vector2d::ZERO;
        Query(m_Elements, queryCache, queryHandler, elemIter, sampleBox, currentElement, iModel->IsToroidal(), iParams.m_Shape.GetAABB());

        if(iDebug)
        {
          iDebug->m_Ops.back().m_FinalResult = queryHandler.m_AcceptanceRate;
        }

        accRates[element].push_back((death ? -1.0 : 1.0) * queryHandler.m_AcceptanceRate);

        if(gradMove > 1 && queryHandler.m_AcceptanceRate < prevAcceptRate + Mathf::ZERO_TOLERANCE)
        {
          superiorGrad = false;
          newBox = prevBox;
          currentElement.m_Pos = prevPos;
          break;
        }
        
        newBox = queryCache.m_Poly1.GetAABB();
      }

      //printf("%f\n",queryHandler.m_AcceptanceRate);
      if(birth)
      {
        //if(((iRand.Generate() % 1000) * queryHandler.m_AcceptanceRate) / 1000.0 > 0.5)
        if((iRand.Generate() % 1000) < queryHandler.m_AcceptanceRate * 1000 * iParams.m_Temperature)
        {
          if(iDebug)
          {
            iDebug->m_Ops.back().m_Passed = true;
          }
          currentElement.curBox = newBox;
          elements.push_back(currentElement);
          
          SpIdxValList spIdxVal = std::make_pair(newBox, std::prev(elements.end()));
          orderedPlaced.push_back(spIdxVal.second);

          queryCache.m_Index.insert(spIdxVal);

          ++numPlaced;
          ++elementsInst[element];
        }
      }
      else if(death)
      {
        float prob = 1.0 / (queryHandler.m_AcceptanceRate + Mathf::EPSILON);
        //if(((iRand.Generate() % 1000) * prob) / 1000.0 > 0.5)
        if((iRand.Generate() % 1000) < prob * 1000 * iParams.m_Temperature)
        {
          if(iDebug)
          {
            iDebug->m_Ops.back().m_Passed = true;
          }
          AABB2Di shapeBox = elemIter->curBox;

          size_t res = queryCache.m_Index.remove(SpIdxValList(std::make_pair(shapeBox, orderedPlaced[elemIdx])));
          elements.erase(elemIter);
          orderedPlaced.erase(orderedPlaced.begin() + elemIdx);
          --numPlaced;
          --elementsInst[element];
        }
      }
      else if(move)
      {
        AABB2Di shapeBox = elemIter->curBox;
      
        size_t res = queryCache.m_Index.remove(SpIdxValList(std::make_pair(shapeBox, orderedPlaced[elemIdx])));
        elemIter->curBox = newBox;
        queryCache.m_Index.insert(SpIdxValList(std::make_pair(newBox, orderedPlaced[elemIdx])));
      }

      if(iDebug)
      {

      }
    }
    
    iParams.m_Placed.reserve(orderedPlaced.size() - iParams.m_Static.size());
    for(unsigned int i = iParams.m_Static.size(); i < orderedPlaced.size(); ++i)
    {
      iParams.m_Placed.push_back(*orderedPlaced[i]);
    }

    //for(unsigned int i = 0; i<m_Elements.size(); ++i)
    //{
    //  char filename[256];
    //
    //  sprintf(filename, "D:\\Element_%i_History.txt", i);
    //
    //  if(FILE* history = fopen(filename, "w"))
    //  {
    //    fprintf(history, "Element %i with density %f", i, m_Elements[i].m_AbsDensity);
    //    for(auto f : accRates[i])
    //    {
    //      fprintf(history, "\n%f", f);
    //    }
    //    fclose(history);
    //  }
    //}
  }

  struct LearnHandler
  {
    LearnHandler(Vector<MCMC2D::PlacedElement > const& iElements) : m_Elements(iElements){}

    inline int GetElement(SpIdxValue<int> const& iVal)
    {
      if(iVal.second <= 0)
        return iVal.second;
      else
        return m_Elements[iVal.second - 1].m_Element;
    }
    inline float GetAngle(SpIdxValue<int> const& iVal)
    {
      return m_Elements[iVal.second - 1].m_Angle;
    }
    
    inline Vector2i const& GetPos(SpIdxValue<int> const& iVal)
    {
      return m_Elements[iVal.second - 1].m_Pos;
    }
    
    inline int GetSubElement(SpIdxValue<int> const& iVal)
    {
      return m_Elements[iVal.second - 1].m_ShapeNum;
    }

    inline bool Accept(SpIdxValue<int> const& iVal, int iIdent)
    {
      if(iVal.second > 0 && iVal.second == iIdent)
        return false;
      return true;
    }
    Vector<MCMC2D::PlacedElement > const& m_Elements;
  };

  struct MidPoint
  {
    inline bool operator ==(MidPoint const& iOther) const
    {
      eXl_ASSERT("Unreachable");
      return curSum == iOther.curSum && numInter == iOther.numInter && weight == iOther.weight;
    }

    Vector3d curSum;
    unsigned int numInter;
    double weight;
  };

  typedef std::pair<Vector3d, MidPoint> InterIdxValue;

  using InteractionIndex = boost::geometry::index::rtree<InterIdxValue, /*boost::geometry::index::rstar<16, 4>*/boost::geometry::index::linear<16, 4> >;

  struct LearnCountHandler : public LearnHandler
  {
    LearnCountHandler(MCMC2D::InputBuilder& iBuilder, Vector<MCMC2D::PlacedElement > const& iElements, MCMC2D::SamplingCell& iCell, float iMaxDist, bool iAllowOverlap = false) : LearnHandler(iElements), m_Builder(iBuilder), m_Cell(iCell), m_AllowOverlap(iAllowOverlap)
    {
      MCMC2D::LearnedModel::DistParams dummyParams;
      dummyParams.m_MaxDist = iMaxDist;
      m_DistParams.push_back(dummyParams);
    }

    LearnCountHandler(MCMC2D::InputBuilder& iBuilder, Vector<MCMC2D::PlacedElement > const& iElements, MCMC2D::SamplingCell& iCell, Vector<MCMC2D::LearnedModel::DistParams> const& iParams, bool iAllowOverlap = false) : LearnHandler(iElements), m_Builder(iBuilder), m_Cell(iCell), m_DistParams(iParams), m_AllowOverlap(iAllowOverlap){}

    inline bool Handle(Polygoni& iPoly1, Polygoni const& iPoly2, Vector<Polygoni>& iPoly3, float dist, Vector2d const& iDir1, Vector2d const& iDir2, unsigned int iElem, float iAngle, unsigned int iOtherElem, float iOtherAngle, bool iSameLayer)
    {
      Vector2d dir1 = iDir1;
      dir1.Normalize();

      Vector2d baseMine = Vector2d(Mathd::Cos(iAngle), Mathd::Sin(iAngle));
      dir1 = MathTools::GetLocal(baseMine, dir1);

      //float angle1 = GetAngleFromVec(dir) - iAngle;

      float centroidalLen = dist;

      bool overlaps = false;

      if(iSameLayer)
      {
        overlaps |= iPoly1.GetAABB().IsInside(iPoly2.GetAABB());
        overlaps |= iPoly2.GetAABB().IsInside(iPoly1.GetAABB());
        overlaps |= iPoly1.GetAABB().Intersect(iPoly2.GetAABB());
        overlaps &= boost::geometry::intersects(iPoly1, iPoly2) || boost::geometry::overlaps(iPoly1, iPoly2);
      }

      if(m_AllowOverlap || !overlaps)
      {
        float maxOverlappingLen = 0.5 * (iPoly1.GetAABB().GetSize().Length() + iPoly2.GetAABB().GetSize().Length());
        float len = overlaps ? (centroidalLen - maxOverlappingLen) : (iSameLayer ? boost::geometry::distance(iPoly1, iPoly2) : centroidalLen);
        MCMC2D::ElementInter newInter;
        if(iOtherElem > 0)
        {
          Vector2d baseOther = Vector2d(Mathd::Cos(iOtherAngle), Mathd::Sin(iOtherAngle));
          Vector2d dir2 = MathTools::GetLocal(baseMine, baseOther);
          newInter.distance = len;
          newInter.dir1 = dir1;
          newInter.dir2 = dir2;
          newInter.elem2 = iOtherElem - 1;
        }
        else
        {
          newInter.distance = len;
          newInter.dir1 = dir1;
          newInter.dir2 = Vector2d::ZERO;
          newInter.elem2 = - 1;
        }
        auto fullInter = MCMC2D::FullInteraction::Make(m_Builder, m_Cell, newInter);
        if(len < MCMC2D::GetMaxDist_Common(fullInter.oneHotIdx, m_DistParams))
        {
          m_Cell.interactions.push_back(newInter);
        }
        
      }
      return true;
    }
    MCMC2D::InputBuilder& m_Builder;
    MCMC2D::SamplingCell& m_Cell;
    Vector<MCMC2D::LearnedModel::DistParams> m_DistParams;
    bool m_AllowOverlap;
  };

  struct CompPlaced
  {
    bool operator() (MCMC2D::PlacedElement const& elem1, MCMC2D::PlacedElement const& elem2) const
    {
      if(elem1.m_Element == elem2.m_Element)
      {
        if(elem1.m_Angle == elem2.m_Angle)
        {
          if(elem1.m_ShapeNum == elem2.m_ShapeNum)
          {
            return elem1.m_Pos < elem2.m_Pos;
          }
          return elem1.m_ShapeNum < elem2.m_ShapeNum;
        }
        return elem1.m_Angle < elem2.m_Angle;
      }
      return elem1.m_Element < elem2.m_Element;
    }
  };

  struct LearnCountAbsenceHandler : public LearnHandler
  {

    LearnCountAbsenceHandler(Vector<MCMC2D::PlacedElement > const& iElements, 
      Vector<MCMC2D::SamplingCell>& iCells, 
      Vector<MCMC2D::Element> const& iElemDesc,
      float iMaxDist) 
      : LearnHandler(iElements)
      , m_Cells(iCells)
      , m_ElementsDesc(iElemDesc)
      , m_MaxDist(iMaxDist)
    {}

    inline bool Accept(SpIdxValue<int> const& iVal, int iIdent)
    {
      //if(iVal.second <= 0 /*|| m_Elements[iVal.second - 1].m_Element != m_ElementFilter*/)
      //  return false;
      return true;
    }

    inline bool Handle(Polygoni& iPoly1, Polygoni const& iPoly2, Vector<Polygoni>& iPoly3, float dist, Vector2d const& iDir1, Vector2d const& iDir2, unsigned int iElem, float iAngle, unsigned int iOtherElem, float iOtherAngle, bool iSameLayer)
    {
      MCMC2D::PlacedElement curHit;

      auto origPoly = iPoly1;
      Vector2i curPos = iPoly1.GetAABB().GetCenter();
      for(unsigned int i = 0; i<m_ElementsDesc.size(); ++i)
      {
        //Skip if it's an exemplar
        if(m_Cells[i].isPresence)
          continue;

        unsigned int numElem = i + 1;
        curHit.m_Element = numElem;

        float centroidalLen = dist;

        Vector2d dirOrig = iDir1;
        dirOrig.Normalize();

        Vector2d baseOther = Vector2d(Mathd::Cos(iOtherAngle), Mathd::Sin(iOtherAngle));

        MCMC2D::Element const& curElem = m_ElementsDesc[i];
        iSameLayer = iOtherElem > 0 ? (curElem.m_Layer & m_ElementsDesc[iOtherElem - 1].m_Layer) != 0 : true;
        unsigned int numAngleSampling = Mathi::Max(1, curElem.m_Turn);

        for(unsigned int shapeNum = 0; shapeNum < curElem.m_Shapes.size(); ++shapeNum)
        {
          curHit.m_ShapeNum = shapeNum;
          for(unsigned int turn = 0; turn < numAngleSampling; ++turn)
          {
            iAngle = (Mathf::PI * 2.0f / float (numAngleSampling)) * turn;

            Vector2d baseMine = Vector2d(Mathd::Cos(iAngle), Mathd::Sin(iAngle));
            Vector2d dir1 = MathTools::GetLocal(baseMine, dirOrig);
            Vector2d dir2 = MathTools::GetLocal(baseMine, baseOther);

            curHit.m_Angle = iAngle;

            iPoly1 = curElem.m_Shapes[shapeNum];
            if(turn > 0)
              iPoly1.Rotate(iAngle);
            iPoly1.Translate(curPos);

            bool overlaps = false;

            if(iSameLayer)
            {
              //Vector<Polygoni> inter;
              //iPoly1.Intersection(iPoly2, inter);
              //overlaps = !inter.empty();
              overlaps |= iPoly1.GetAABB().IsInside(iPoly2.GetAABB());
              overlaps |= iPoly2.GetAABB().IsInside(iPoly1.GetAABB());
              overlaps |= iPoly1.GetAABB().Intersect(iPoly2.GetAABB());
              overlaps &= boost::geometry::intersects(iPoly1, iPoly2) || boost::geometry::overlaps(iPoly1, iPoly2);
            }

            float len = overlaps ? 0.0 : (iSameLayer ? boost::geometry::distance(iPoly1, iPoly2) : centroidalLen);

            iPoly1 = origPoly;

            if(len < m_MaxDist)
            {
              if(iOtherElem > 0)
              {
              
                //if(numElem > iOtherElem)
                //{
                //  auto temp = dir2;
                //  dir2 = dir1;
                //  dir1 = temp;
                //}

                MCMC2D::ElementInter newAbs;
                newAbs.distance = len;
                newAbs.dir1 = dir1;
                newAbs.dir2 = dir2;
                newAbs.elem2 = iOtherElem - 1;
                m_Cells[i].interactions.push_back(newAbs);
              }
              else
              {
                MCMC2D::ElementInter newAbs;
                newAbs.distance = len;
                newAbs.dir1 = dir1;
                newAbs.dir2 = Vector2d::ZERO;
                newAbs.elem2 = -1;
                m_Cells[i].interactions.push_back(newAbs);
              }
            }
          }
        }
      }
      return true;
    }

    Vector<MCMC2D::Element> const& m_ElementsDesc;
    Vector<MCMC2D::SamplingCell>& m_Cells;
    float m_MaxDist;
  };


  //struct Cell
  //{
  //  Vector<MCMC2D::FullInteraction> inter;
  //
  //  void Perturbate(eXl::Random& iRand, float iRadius, float iAngRange)
  //  {
  //    for(auto& curInter : inter)
  //    {
  //      curInter.Perturbate(iRand, iRadius, iAngRange);
  //    }
  //  }
  //
  //  lbfgsfloatval_t cellWh;
  //  lbfgsfloatval_t presWh;
  //  unsigned int element;
  //};

  struct CompDist
  {
    bool operator() (std::pair<float, unsigned int> const& iElem1, std::pair<float, unsigned int> const& iElem2)
    {
      return iElem1.first < iElem2.first;
    }
  };

  float smoothstep(float edge0, float edge1, float x) 
  {
    // Scale, bias and saturate x to 0..1 range
    x = Mathf::Clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0); 
    // Evaluate polynomial
    return x * x * (3 - 2 * x);
  }

  MCMC2D::LearnedModel::LearnedModel(Vector<DistParams> const& iDist, Vector<Element> const& iElements, bool iToroidal)
    : m_DistParams(iDist)
    , m_Elements(iElements)
    , m_Toroidal(iToroidal)
  {}

  MCMC2D::LearnedModel::~LearnedModel()
  {}

  MCMC2D::QuantileCullParams::QuantileCullParams(Vector<MCMC2D::Element> const& iElements, bool iToroidal, float iMaxDist, double iSigmaDist, double iSigmaAngle)
    : MCMC2D::LearnedModel(Vector<MCMC2D::LearnedModel::DistParams>(), iElements, iToroidal)
    , m_Vectors(InputBuilder::ComputeNumElems(iElements, iToroidal))
    , m_SigmaDist(iSigmaDist)
    , m_SigmaAngle(iSigmaAngle)
  {
    DistParams dist;
    dist.m_MaxDist = iMaxDist;
    m_DistParams.push_back(dist);
  }

  float MCMC2D::QuantileCullParams::Sample(MCMC2D::FullInteraction const& iInter)
  {
    double constant;
    double alpha;
    ComputeDistribution(iInter.oneHotIdx, iInter.dist, iInter.dir1, constant, alpha);

    return 0.5*alpha + (1.0 - alpha) * constant;
  }

  void MCMC2D::QuantileCullParams::ComputeDistribution(unsigned int iIdx, double iDist, Vector2d const& iDir, double& oConstant, double& oAlpha)
  {
    auto const& freqMap = m_Vectors[iIdx];
    double accumValue = 0;
    double totValue = 0.0;
    double locValue = 0.0;
    for(unsigned int k = 0; k<freqMap.size(); ++k)
    {
      double otherDist = freqMap[k].X();
      Vector2d otherDir(freqMap[k].Y(), freqMap[k].Z());

      // == 0.5 * (2 * (1 - cos(theta))) == 0.5 * ||a - b||²
      double dirCoeff = 0.5 * (iDir - otherDir).SquaredLength();

      if(m_DebugDisplayPos)
      {
        dirCoeff *= 1.0 / (m_SigmaAngle * m_SigmaAngle);
        double distCoeff = (iDist - otherDist) / m_SigmaDist;
        locValue += exp(-dirCoeff) * exp(-distCoeff * distCoeff);
      }
      else
      {
        // Do not bother if it's more than 3 std dev away.
        //if(dirCoeff < 3.0 * m_SigmaAngle)
        {

          dirCoeff *= 1.0 / (m_SigmaAngle * m_SigmaAngle);
          dirCoeff = exp(-dirCoeff);
          
          totValue += dirCoeff /** m_SigmaDist*/ * (1.0 + erf(otherDist / m_SigmaDist));
          accumValue += dirCoeff /** m_SigmaDist*/ * (erf((iDist - otherDist) / m_SigmaDist) + erf(otherDist / m_SigmaDist));
        }
      }
    }

    if(m_DebugDisplayPos)
    {
      oConstant = locValue;
      oAlpha = 0;
      return ;
    }

    double ratio = (accumValue / totValue);

    //if(m_DebugOutput)
    {
      unsigned int const quantile = 10;
      unsigned int const numQuantiles = 2;
      float factor = (float(numQuantiles) + 0.5) / quantile;
      if (accumValue < 0.5)
      {
        oAlpha = smoothstep(0, 0.5, accumValue);
        oConstant = smoothstep(0.9 * GetMaxDist(0), 1.1*GetMaxDist(0), iDist);
        //return 0.5 * smoothstep(0, 0.5, accumValue);
      }
      else if(ratio > factor)
      {
        oConstant = 1.0;
        oAlpha = 1.0 - smoothstep(factor, 0.5, ratio);
        //return (1.0 + ) * 0.5;
      }
      else
      {
        oConstant = 0;
        oAlpha = 1.0;
      }
    }
  };

  MCMC2D::QuantileCullParams* MCMC2D::QuantileCullParams::Clone() const
  {
    QuantileCullParams* ret = new QuantileCullParams(m_Elements, m_Toroidal, GetMaxDist(0), m_SigmaDist, m_SigmaAngle);
    ret->m_Vectors = m_Vectors;

    return ret;
  }

  String const& MCMC2D::QuantileCullParams::GetModelName() const
  {
    static String modelName = "QuantileCull";
    return modelName;
  }

  Err MCMC2D::QuantileCullParams::Stream(Streamer& streamer) const
  {
    streamer.BeginStruct();
    WriteCommon(streamer);
    streamer.PushKey("SigmaDist");
    streamer.Write(&m_SigmaDist);
    streamer.PopKey();
    streamer.PushKey("SigmaAngle");
    streamer.Write(&m_SigmaAngle);
    streamer.PopKey();
    streamer.PushKey("SigmaVector");
    streamer.Write(&m_Vectors);
    streamer.PopKey();
    streamer.EndStruct();

    return Err::Success;
  }

  Err MCMC2D::QuantileCullParams::Unstream(Unstreamer& streamer)
  {
    streamer.BeginStruct();
    ReadCommon(streamer);
    streamer.PushKey("SigmaDist");
    streamer.Read(&m_SigmaDist);
    streamer.PopKey();
    streamer.PushKey("SigmaAngle");
    streamer.Read(&m_SigmaAngle);
    streamer.PopKey();
    streamer.PushKey("SigmaVector");
    streamer.Read(&m_Vectors);
    streamer.PopKey();
    streamer.EndStruct();

    return Err::Success;
  }

  void MCMC2D::Learner::BuildCells(LearnExample const& iExamples)
  {
    auto const& elements = GetElements();
    float const iMaxDist = m_Params.m_MaxDist_;

    InputBuilder& builder = GetBuilder();

    if(m_Params.m_QuantileCull)
    {
      m_Mask.reset(new QuantileCullParams(elements, m_Params.m_Toroidal, iMaxDist, m_Params.m_SigmaDist, m_Params.m_SigmaAngle));
    }

    if(m_Params.m_QuantileCull)
    {
      LearnedModel::DistParams defaultParams;
      defaultParams.m_MaxDist = iMaxDist;
      m_DistParams.resize(builder.oneHotSize, defaultParams);
    }
    else
    {
      LearnedModel::DistParams defaultParams;
      defaultParams.m_MaxDist = iMaxDist;
      defaultParams.m_EaseOutOffset = iMaxDist;
      defaultParams.m_EaseOutCoeff = log(9)/(0.3*iMaxDist);

      m_DistParams.push_back(defaultParams);
    }

    std::vector<unsigned int> relDensity;
    std::vector<AABB2Di> querySize(elements.size());
    //float totalDensity = 0;
    Vector2i boxSize = iExamples.m_Shape.GetAABB().GetSize();
    Vector2f iGridStep(float(boxSize.X()) / GetParams().m_GridStep, float(boxSize.Y()) / GetParams().m_GridStep);
    float globArea = iExamples.m_Shape.Area();
    Vector<float> elementArea(elements.size(), 0.0);

    AABB2Di maxQuery(-iMaxDist, -iMaxDist, iMaxDist, iMaxDist);
    
    float weightCoeff = 0.0;
    for(unsigned int i = 0; i<elements.size(); ++i)
    {
      float accumArea = 0.0;
      for(unsigned int j = 0; j<elements[i].m_Shapes.size(); ++j)
      {
        //if(querySize[i].Empty())
        //  querySize[i] = elements[i].m_Shapes[j].GetAABB();
        //else
        //  querySize[i].Absorb(elements[i].m_Shapes[j].GetAABB());
      
        accumArea += elements[i].m_Shapes[j].Area();
      }
      //querySize[i].m_Data[0].X() -= iMaxDist + 1;
      //querySize[i].m_Data[0].Y() -= iMaxDist + 1;
      //querySize[i].m_Data[1].X() += iMaxDist + 1;
      //querySize[i].m_Data[1].Y() += iMaxDist + 1;
      //
      //if(maxQuery.Empty())
      //  maxQuery = querySize[i];
      //else
      //  maxQuery.Absorb(querySize[i]);
      //
      accumArea /= elements[i].m_Shapes.size();
      elementArea[i] = accumArea;
      weightCoeff += elements[i].m_Shapes.size() * Mathi::Max(1, elements[i].m_Turn);
    }
#ifdef INDIVIDUAL_SPACE
    weightCoeff *= elements.size();
#endif
    //weightCoeff = iExamples.m_Elements.size();

    Vector<unsigned int> numElements(elements.size(), 0);

    //for(auto ex : iExamples)
    LearnExample const& ex = iExamples;
    //{
    Vector2i areaSize = ex.m_Shape.GetAABB().GetSize();
    Vector2i gridSize(areaSize.X() / iGridStep.X(), areaSize.Y() / iGridStep.Y());
    if(areaSize.X() % int(iGridStep.X()) != 0)
      ++gridSize.X();
    if(areaSize.Y() % int(iGridStep.Y()) != 0)
      ++gridSize.Y();

    QueryCache<PointIndex<int> > queryCache;

    {
      Vector<SpIdxValue<int> > values;

      if(!m_Params.m_Toroidal)
      {
        Polygoni outerBorder(AABB2Di(ex.m_Shape.GetAABB().m_Data[0] - Vector2i::ONE * 50, ex.m_Shape.GetAABB().GetSize() + Vector2i::ONE * 100));
        Vector<Polygoni> negShape;
        outerBorder.Difference(ex.m_Shape, negShape);

        Vector<Polygoni> trapezoids;
        for(auto poly : negShape)
        {
          trapezoids = poly.GetTrapezoids();
        }
        for(auto trap : trapezoids)
        {
          trap.RemoveUselessPoints();
          //PlacedElement element = {Vector2i::ZERO, 0.0, -walls.size()};
          //wallList.push_back(element);
          SpIdxValue<int> value = std::make_pair(trap.GetAABB(), -int(queryCache.m_Walls.size()));
          //queryCache.m_Index.insert(value);
          values.push_back(value);
          queryCache.m_Walls.emplace_back(std::move(trap));
        }
      }

      for(int i = 0; i < ex.m_Elements.size(); ++i)
      {
        PlacedElement const& currentElement = ex.m_Elements[i];
        if(currentElement.m_Element > 0)
        {
          unsigned int element = currentElement.m_Element - 1;
          ++numElements[element];
          queryCache.m_Poly1 = elements[element].m_Shapes[currentElement.m_ShapeNum];
          queryCache.m_Poly1.Rotate(currentElement.m_Angle);
          queryCache.m_Poly1.Translate(currentElement.m_Pos);

          SpIdxValue<int> spIdxVal = std::make_pair(queryCache.m_Poly1.GetAABB(), i + 1);
          values.push_back(spIdxVal);
        }
      }
      //Packing;
      queryCache.m_Index = PointIndex<int>(std::make_pair(values.begin(), values.end()));
    }
    
    for(unsigned int i = 0; i<elements.size(); ++i)
    {
      //elements[i].m_RelDensity = 1.0;//float(numElements[i]) / ex.elements.size();
    }

    unsigned int gridTotSize = gridSize.X() * gridSize.Y();

    std::map<Vector2i, std::pair<float, int> > weightMap;
    
    auto makePointPosForElem = [this, &elements](PlacedElement const& elem)
    {
      return Vector2i(
        elem.m_Pos.X() + elements[elem.m_Element - 1].m_Shapes[elem.m_ShapeNum].GetAABB().GetCenter().X(), 
        elem.m_Pos.Y() + elements[elem.m_Element - 1].m_Shapes[elem.m_ShapeNum].GetAABB().GetCenter().Y());
    };

    VoronoiGraph gr;
    unsigned int numCells = 0;
    for(auto elem : ex.m_Elements)
    {
      //Vector2i gridPos((elem.m_Pos.X() - ex.m_Shape.GetAABB().m_Data[0].X()) / iGridStep.X(), 
      //  (elem.m_Pos.Y() - ex.m_Shape.GetAABB().m_Data[0].Y()) / iGridStep.Y());
      Vector2i exPoint = makePointPosForElem(elem);
      weightMap.insert(std::make_pair(exPoint, std::make_pair(0.0f, numCells++)));
      //weightMap[gridPos.Y() * gridSize.X() + gridPos.X()];
      gr.AddCircle(1.0, Vector2f(exPoint.X(), exPoint.Y()));
    }
    unsigned int egCells = numCells;
    for(int i = 0; i<gridSize.Y(); ++i)
    {
      for(int j = 0; j<gridSize.X(); ++j)
      {
        Vector2i posGrid(j * iGridStep.X(), i * iGridStep.Y());
        posGrid += ex.m_Shape.GetAABB().m_Data[0];

        if(weightMap.find(posGrid) == weightMap.end())
        {
          gr.AddCircle(1.0, Vector2f(posGrid.X(), posGrid.Y()));
          weightMap.insert(std::make_pair(posGrid, std::make_pair(0.0f, numCells++)));
        }
      }
    }

    Vector<Polygond> samplingCells;
    gr.GetCells(samplingCells);

    float totWeight = 0.0;
    {
      unsigned int idx = 0;
      for(auto& wh : weightMap)
      {
        // Neg Wh for dummyPoints
        wh.second.first = (wh.second.second >= egCells ? -1.0 : 1.0) * samplingCells[wh.second.second].Area();
        wh.second.first /= (weightCoeff /** Mathf::PI * iMaxDist * iMaxDist*/);
        totWeight += Mathf::Abs(wh.second.first);
      }
    }

    printf("Total area : %f\n", globArea);
    printf("Total weight : %f\n", totWeight);

    Vector<SamplingCell>& cells = GetCells();
    cells.resize(egCells);
    Vector<Vector<unsigned int>> cellToSamples(egCells);

    Vector<Vector<std::pair<unsigned int, unsigned int>>> freqMapToCellInter(builder.oneHotSize);

    for(unsigned int i = 0; i < ex.m_Elements.size(); ++i)
    {
      cellToSamples[i].push_back(i);
      PlacedElement const& currentElement = ex.m_Elements[i];
      unsigned int element = currentElement.m_Element - 1;
      //Polygoni samplePoly(querySize[element]);
      //samplePoly.Rotate(currentElement.m_Angle);

      AABB2Di sampleBox = maxQuery;//samplePoly.GetAABB();
      sampleBox.m_Data[0] += currentElement.m_Pos;
      sampleBox.m_Data[1] += currentElement.m_Pos;

      Vector2i pos = makePointPosForElem(currentElement);

      auto iter = weightMap.find(pos);
      if(iter != weightMap.end())
      {
        SamplingCell& newCell = cells[iter->second.second];
        newCell.position = pos;
        newCell.elem1 = element;
        newCell.weight = iter->second.first;
        newCell.isPresence = true;
        newCell.angle = currentElement.m_Angle;
        
        LearnCountHandler learnCountHandler(builder, ex.m_Elements, newCell, iMaxDist);

        AABB2Di const& exampleBox = ex.m_Shape.GetAABB();

        Query(elements, queryCache, learnCountHandler, i + 1, sampleBox, currentElement, m_Params.m_Toroidal, iExamples.m_Shape.GetAABB());
        
        if(GetParams().m_QuantileCull)
        {
          for(unsigned int j = 0; j< newCell.interactions.size(); ++j)
          {
            auto& inter = newCell.interactions[j];
            auto fullInter = FullInteraction::Make(builder, newCell, inter);

            auto& curMap = m_Mask->m_Vectors[fullInter.oneHotIdx];

            curMap.push_back(Vector3d(fullInter.dist, fullInter.dir1.X(), fullInter.dir1.Y()));
            freqMapToCellInter[fullInter.oneHotIdx].push_back(std::make_pair(i, j));
          }
        }
      }
    }

    if(GetParams().m_QuantileCull)
    {
      //m_Mask->m_DebugOutput = true;
      //DrawDbgImg("D:\\NetLearn\\DbgQuantCull\\Dbg_Accum", m_Mask.get());
      //
      //m_Mask->m_DebugDisplayPos = true;
      //
      //DrawDbgImg("D:\\NetLearn\\DbgQuantCull\\Dbg_Pos", m_Mask.get());
      //m_Mask->m_DebugDisplayPos = false;
      //m_Mask->m_DebugOutput = false;

      for(unsigned int idx = 0; idx < builder.oneHotSize; ++idx)
      {
        // Map of the accumulated PDF for each direction.
        // Map<Vector2f, float> dirDistribution;
        Vector<float> individualValue;
        auto const& freqMap = m_Mask->m_Vectors[idx];
        for(unsigned int j = 0; j<freqMap.size(); ++j)
        {
          double pointDist = freqMap[j].X();
          Vector2d pointDir(freqMap[j].Y(), freqMap[j].Z());
      
          double alpha;
          double constant;
          m_Mask->ComputeDistribution(idx, pointDist, pointDir, constant, alpha);
          individualValue.push_back(alpha);
        }
      
        float maxDist = 0;
      
        for(unsigned int k = 0; k<freqMap.size(); ++k)
        {
          float alpha = individualValue[k];
      
          auto& cell = cells[freqMapToCellInter[idx][k].first];
          auto& inter = cell.interactions[freqMapToCellInter[idx][k].second];
          if(alpha < 0.01)
          {
            inter.coeff = 0.0;
          }
          //else
          //{
          //  maxDist = Mathi::Max(maxDist, inter.distance);
          //}
        }
        //if(maxDist > 0)
        //{
        //  m_DistParams[idx].m_MaxDist = maxDist;
        //  m_DistParams[idx].m_EaseOutOffset = maxDist;
        //  m_DistParams[idx].m_EaseOutCoeff = log(9)/(0.3*maxDist);
        //}
      }
    }

    Vector<float> totWh(elements.size(), 0.0);
    Vector<float> redistWh(elements.size(), 0.0);
    unsigned int offsetWh = 0;
    for(auto const& wh : weightMap)
    {
      cellToSamples.push_back(Vector<unsigned int>());

      Vector2i const& gridPos = wh.first;
      float cellWh = wh.second.first;
      
      int presIdx = -1;
      float presAngle;
      if(cellWh > 0.0)
      {
        presIdx = cells[wh.second.second].elem1;
        presAngle =  cells[wh.second.second].angle;
      }

      SamplingCell candidateCell;
      float curAngleDiff = FLT_MAX;

      AABB2Di sampleBox = maxQuery;
      sampleBox.m_Data[0] += gridPos;
      sampleBox.m_Data[1] += gridPos;

      for(int i = 0; i<elements.size(); ++i)
      {
        auto& elemDesc = elements[i];
        unsigned int const angleSampling = Mathi::Max(elemDesc.m_Turn, 1);
        float const angleIncrement = Mathf::PI * 2.0 / angleSampling;
      
        for(unsigned int shapeNum = 0; shapeNum < elemDesc.m_Shapes.size(); ++shapeNum)
        {
          for(unsigned int turn = 0; turn < angleSampling; ++turn)
          {
            SamplingCell newCell;
            newCell.position = gridPos;
            newCell.elem1 = i;
            newCell.weight = Mathf::Abs(cellWh);
            newCell.angle = angleIncrement * turn;
      
            LearnCountHandler handler(builder, ex.m_Elements, newCell, m_DistParams, false);
      
            PlacedElement dummyElem;
            dummyElem.m_Angle = angleIncrement * turn;
            dummyElem.m_Element = i + 1;
            dummyElem.m_Pos = gridPos;
            dummyElem.m_ShapeNum = shapeNum;
      
            Query(elements, queryCache, handler, 0, sampleBox, dummyElem, m_Params.m_Toroidal, iExamples.m_Shape.GetAABB());

            for(auto& inter : newCell.interactions)
            {
              auto fullInter = FullInteraction::Make(builder, newCell, inter);
              double constant;
              double alpha = 1.0;
              if(m_Mask)
              {
                m_Mask->ComputeDistribution(fullInter.oneHotIdx, fullInter.dist, fullInter.dir1, constant, alpha);
              }

              if(alpha < 0.1)
              {
                alpha = 0.0;
              }

              inter.coeff *= alpha;
            }

            if(presIdx == i)
            {
              if(curAngleDiff == FLT_MAX)
              {
                candidateCell = newCell;
                curAngleDiff = MathTools::AngleDist(dummyElem.m_Angle, presAngle);
              }
              else
              {
                float angleDiff = MathTools::AngleDist(dummyElem.m_Angle, presAngle);
                if(Mathf::Abs(angleDiff) < Mathf::Abs(curAngleDiff))
                {
                  cellToSamples.back().push_back(cells.size());
                  cells.push_back(candidateCell);
                  candidateCell = newCell;
                  curAngleDiff = angleDiff;
                }
                else
                {
                  cellToSamples.back().push_back(cells.size());
                  cells.push_back(newCell);
                }
              }
            }
            else
            {
              cellToSamples.back().push_back(cells.size());
              cells.emplace_back(std::move(newCell));
            }
          }
        }
      }
    }

    for(auto& cell : cells)
    {
      for(int i = 0; i<(int)cell.interactions.size(); ++i)
      { 
        if(cell.interactions[i].coeff == 0.0)
        {
          cell.interactions.erase(cell.interactions.begin() + i);
          --i;
        }
      }
    }

    if(GetParams().m_Resample)
    {
      Random& rand = GetParams().m_RandGen;
      Vector<SamplingCell> origCells = std::move(cells);

      for(auto& origCell : origCells)
      {
        for(unsigned int newSample = 0; newSample < GetParams().m_Oversampling; ++newSample)
        {
          SamplingCell newCell = origCell;
          newCell.weight *= 1.0 / GetParams().m_Oversampling;
          
          for(auto& inter : newCell.interactions)
          {
            Vector2d point = inter.dir1 * inter.distance;
            double angle = MathTools::RandFloatIn(rand, -Mathd::PI, Mathd::PI);
            double dist = MathTools::RandNormFloatIn(rand, 0.0, double(GetParams().m_Dispersion * iMaxDist));

            point += Vector2d(Mathd::Cos(angle), Mathd::Sin(angle)) * dist;

            inter.distance = point.Normalize();
            inter.dir1 = point;
          }

          cells.emplace_back(std::move(newCell));
        }
      }
    }
  }

  Err MCMC2D::LearnedModel::DistParams::Stream(Streamer& streamer) const
  {
    streamer.BeginStruct();
    streamer.PushKey("MaxDist");
    streamer.Write(&m_MaxDist);
    streamer.PopKey();
    streamer.PushKey("EaseOutOffset");
    streamer.Write(&m_EaseOutOffset);
    streamer.PopKey();
    streamer.PushKey("EaseOutCoeff");
    streamer.Write(&m_EaseOutCoeff);
    streamer.PopKey();
    streamer.EndStruct();

    return Err::Success;
  }

  Err MCMC2D::LearnedModel::DistParams::Unstream(Unstreamer& streamer)
  {
    streamer.BeginStruct();
    streamer.PushKey("MaxDist");
    streamer.Read(&m_MaxDist);
    streamer.PopKey();
    streamer.PushKey("EaseOutOffset");
    streamer.Read(&m_EaseOutOffset);
    streamer.PopKey();
    streamer.PushKey("EaseOutCoeff");
    streamer.Read(&m_EaseOutCoeff);
    streamer.PopKey();
    streamer.EndStruct();

    return Err::Success;
  }

  void MCMC2D::LearnedModel::WriteCommon(Streamer& streamer) const
  {
    streamer.PushKey("Toroidal");
    streamer.Write(&m_Toroidal);
    streamer.PopKey();
    streamer.PushKey("Elements");
    streamer.Write(&m_Elements);
    streamer.PopKey();
    streamer.PushKey("DistanceParameters");
    streamer.Write(&m_DistParams);
    streamer.PopKey();
    if(m_Mask)
    {
      streamer.PushKey("Mask");
      m_Mask->Stream(streamer);
      streamer.PopKey();
    }
  }

  void MCMC2D::LearnedModel::ReadCommon(Unstreamer& streamer)
  {
    streamer.PushKey("Toroidal");
    streamer.Read(&m_Toroidal);
    streamer.PopKey();
    streamer.PushKey("Elements");
    streamer.Read(&m_Elements);
    streamer.PopKey();
    streamer.PushKey("DistanceParameters");
    streamer.Read(&m_DistParams);
    streamer.PopKey();
    if(streamer.PushKey("Mask"))
    {
      m_Mask.reset(new QuantileCullParams(m_Elements, m_Toroidal, 0, 0, 0));
      m_Mask->Unstream(streamer);
      streamer.PopKey();
    }
  }

  float MCMC2D::GetMaxDist_Common(unsigned int iOneHotIdx, Vector<LearnedModel::DistParams> const& iDists)
  {
    if(iOneHotIdx >= iDists.size())
    {
      //eXl_ASSERT(iDists.size() == 1);
      return iDists[0].m_MaxDist;
    }
    else
    {
      return iDists[iOneHotIdx].m_MaxDist;
    }
  }

  float MCMC2D::LearnedModel::GetMaxDist(unsigned int iOneHotIdx) const
  {
    return GetMaxDist_Common(iOneHotIdx, m_DistParams);
  }

  float MCMC2D::Learner::GetMaxDist(unsigned int iOneHotIdx) const
  {
    return GetMaxDist_Common(iOneHotIdx, m_DistParams);
  }

  Vector<float> MCMC2D::LearnedModel::Sample(unsigned int oneHotIdx, Vector<InteractionGeom> const& interactions)
  {
    Vector<float> result;
    result.reserve(interactions.size());
    for(auto const& inter : interactions)
    {
      FullInteraction fullInter;
      static_cast<InteractionGeom&>(fullInter) = inter;
      fullInter.oneHotIdx = oneHotIdx;

      result.push_back(Sample(fullInter));
    }
    return std::move(result);
  }

  Image MCMC2D::DrawDbgImg(LearnedModel* iModel, uint32_t iOneHotIdx)
  {
    const int imgSize = 256;

    Vector2f range;
    range.X() = FLT_MAX;
    range.Y() = -FLT_MAX;

    Vector<InteractionGeom> interactions;
    for (int j = 0; j < imgSize; ++j)
    {
      for (int k = 0; k < imgSize; ++k)
      {
        Vector2f dir(k - imgSize / 2, j - imgSize / 2);
        dir *= (2.0 / imgSize);
        float len = dir.Normalize();

        InteractionGeom inter;
        inter.dir1 = Vector2d(dir.X(), dir.Y());
        inter.dist = len * iModel->GetMaxDist(iOneHotIdx);

        interactions.push_back(inter);
      }
    }

    Vector<float> out = iModel->Sample(iOneHotIdx, interactions);

    for (auto score : out)
    {
      range.Y() = Mathd::Max(score, range.Y());
      range.X() = Mathd::Min(score, range.X());
    }

    Vector<int> image(imgSize * imgSize);
    double amplitude = range.X() != range.Y() ? range.Y() - range.X() : range.Y();
    double minVal = range.X() != range.Y() ? range.X() : 0.0;
    for (unsigned int pix = 0; pix < image.size(); ++pix)
    {
      double val = (out[pix] - minVal) / amplitude;
      image[pix] = 255 << 24 | (unsigned char)(val * 255) << 8 | (unsigned char)((1.0 - val) * 255);
    }

    return Image(image.data(), Image::Size(imgSize, imgSize), Image::RGBA, Image::Char, 1, Image::Copy);
  }

#ifdef EXL_IMAGESTREAMER_ENABLED
  void MCMC2D::DrawDbgImg(String const& iPath, LearnedModel* iModel)
  {
    //if(!m_NetImpl->m_Net)
    //  return;

    if(!iModel)
      return;

    auto const& elements = iModel->GetElements();

    InputBuilder builder(elements, iModel->IsToroidal());

    auto writeImage = [](String const& iName, Image const& iImg)
    {
      ImageStreamer::Save(&iImg, StringUtil::ToASCII(iName));
    };

    auto makeBorderName = [&iPath](unsigned int i, String const& iSuffix)->String
    {
      String path(iPath);
      path+= "BorderInt";
      path.append(iSuffix);
      path += StringUtil::FromInt(i);
      path += ".png";

      return path;
    };

    auto makeName = [&iPath](unsigned int i, unsigned int j, unsigned int turn, String const& iSuffix)->String
    {
      String path(iPath);
      path += "Int";
      path.append(iSuffix);
      path += StringUtil::FromInt(i);
      path += "_";
      path += StringUtil::FromInt(j + i);
      path += "_";
      path += StringUtil::FromInt(turn);
      path += ".png";

      return path;
    };

    Vector<Vector<float> > imageD;
    unsigned int numImage = 0;

    for (int i = 0; i < elements.size(); ++i)
    {
      int startElem = iModel->IsToroidal() ? 0 : -1;
      for (int j = startElem; j < static_cast<int>(elements.size() - i); ++j)
      {
        if (j < 0)
        {
          unsigned int oneHotIdx = builder.BuildOneHot(i, -1, 0.0);

          Image out = DrawDbgImg(iModel, oneHotIdx);
          writeImage(makeBorderName(i, "Density"), out);
        }
        else
        {
          unsigned int const numTurns = Mathi::Max(elements[i + j].m_Turn, 1);
          for (unsigned int turn = 0; turn < numTurns; ++turn)
          {
            float angle2 = turn * 2.0 * Mathf::PI / numTurns;

            unsigned int oneHotIdx = builder.BuildOneHot(i, j, angle2);

            Image out = DrawDbgImg(iModel, oneHotIdx);
            writeImage(makeName(i, j, turn, "Density"), out);
          }
        }
      }
    }
  }
#endif
}
