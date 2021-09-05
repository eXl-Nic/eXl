/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <math/polygon.hpp>
#include <math/polygon_def.hpp>
#include <math/segment.hpp>
#include <set>

#include <boost/polygon/polygon.hpp>

namespace boost
{
  namespace polygon 
  {
  
    template <typename Real>
    struct geometry_concept<eXl::Polygon<Real> >{ typedef polygon_with_holes_concept type; };
  
    template <typename Real>
    struct geometry_concept<eXl::Vector<eXl::Vector2<Real> > >{ typedef polygon_concept type; };
  
    template <typename Real>
    struct polygon_90_traits < eXl::Polygon<Real>, boost::polygon::gtl_no > {};
    
  
    template <typename Real>
    struct polygon_90_traits < eXl::Vector<eXl::Vector2<Real> >, boost::polygon::gtl_no > {};
    
  
    template <typename Real>
    struct polygon_traits_general<eXl::Polygon<Real> > {
      typedef Real coordinate_type;
      typedef typename eXl::Vector<eXl::Vector2<Real> >::const_iterator iterator_type;
      typedef eXl::Vector2<Real> point_type;
    
      static inline iterator_type begin_points(eXl::Polygon<Real> const& t) {
          return t.Border().begin();
      }
      static inline iterator_type end_points(eXl::Polygon<Real> const& t) {
          return t.Border().end();
      }
    
      // Get the number of sides of the polygon
      static inline std::size_t size(eXl::Polygon<Real> const& t) {
        return t.Border().size();
      }
    
      // Get the winding direction of the polygon
      static inline winding_direction winding(eXl::Polygon<Real> const& t) {
        return unknown_winding;
      }
    };
    
    template <typename Real>
    struct polygon_mutable_traits<eXl::Polygon<Real> > {
    
      template <typename iT>
      static inline eXl::Polygon<Real>& set_points(eXl::Polygon<Real> & t, 
                                         iT input_begin, iT input_end) {
    
        eXl::Vector<eXl::Vector2<Real> > tempStor;
        //iterator_compact_to_points<iT,eXl::Vector2<Real> > iterBegin(input_begin,input_end);
        //iterator_compact_to_points<iT,eXl::Vector2<Real> > iterEnd(input_end,input_end);
        
        //tempStor.assign(input_begin,input_end);
        for (; input_begin != input_end; ++input_begin)
        {
          eXl::Vector2<Real> temp(boost::polygon::point_traits<typename iT::value_type>::get(*input_begin, boost::polygon::HORIZONTAL),
                                  boost::polygon::point_traits<typename iT::value_type>::get(*input_begin, boost::polygon::VERTICAL)) ;
          tempStor.push_back(temp);
        }
    
        t = eXl::Polygon<Real>();
        t.m_Ext.swap(tempStor);
        t.ForceClockwise();
        t.UpdateAABB();
        return t;
      }
    
    };
  
    template <typename Real>
    struct polygon_traits_general<eXl::Vector<eXl::Vector2<Real> > > {
      typedef Real coordinate_type;
      typedef typename eXl::Vector<eXl::Vector2<Real> >::const_iterator iterator_type;
      typedef eXl::Vector2<Real> point_type;
  
      static inline iterator_type begin_points(typename eXl::Polygon<Real>::PtList const& t) {
          return t.begin();
      }
      static inline iterator_type end_points(typename eXl::Polygon<Real>::PtList const& t) {
          return t.end();
      }
  
      // Get the number of sides of the polygon
      static inline std::size_t size(typename eXl::Polygon<Real>::PtList const& t) {
        return t.size();
      }
  
      // Get the winding direction of the polygon
      static inline winding_direction winding(typename eXl::Polygon<Real>::PtList const& t) {
        return unknown_winding;
      }
    };
  
    template <typename Real>
    struct polygon_mutable_traits<eXl::Vector<eXl::Vector2<Real> > > {
  
      template <typename iT>
      static inline typename eXl::Polygon<Real>::PtList& set_points(typename eXl::Polygon<Real>::PtList & t, 
                                         iT input_begin, iT input_end) {
  
        //iterator_compact_to_points<iT,eXl::Vector2<Real> > iterBegin(input_begin,input_end);
        //iterator_compact_to_points<iT,eXl::Vector2<Real> > iterEnd(input_end,input_end);
        //t.assign(input_begin,input_end);
        t.clear();
        for (; input_begin != input_end; ++input_begin)
        {
          eXl::Vector2<Real> temp(boost::polygon::point_traits<typename iT::value_type>::get(*input_begin, boost::polygon::HORIZONTAL),
                                  boost::polygon::point_traits<typename iT::value_type>::get(*input_begin, boost::polygon::VERTICAL)) ;
          t.push_back(temp);
        }
        return t;
      }
  
    };
  
    template <typename Real,typename enable>
    struct polygon_with_holes_traits<eXl::Polygon<Real>,enable> {
         typedef typename eXl::Polygon<Real>::PtLists::const_iterator iterator_holes_type;
         typedef typename eXl::Polygon<Real>::PtList hole_type;
         static inline iterator_holes_type begin_holes(const eXl::Polygon<Real>& t) {
              return t.Holes().begin();
         }
         static inline iterator_holes_type end_holes(const eXl::Polygon<Real>& t) {
              return t.Holes().end();
         }
         static inline Real size_holes(const eXl::Polygon<Real>& t) {
              return t.Holes().size();
         }
    };
    
    template <typename Real, typename enable>
    struct polygon_with_holes_mutable_traits<eXl::Polygon<Real>,enable> {
         template <typename iT>
         static inline eXl::Polygon<Real>& set_holes(eXl::Polygon<Real>& t, iT inputBegin, iT inputEnd) {
  
              for(;inputBegin != inputEnd;++inputBegin)
              {
                typename eXl::Polygon<Real>::PtList hole;
                boost::polygon::assign(hole,*inputBegin);
                t.HolesRW().push_back(typename eXl::Polygon<Real>::PtList());
                t.HolesRW().back().swap(hole);
              }
              t.ForceCClockwiseHoles();
              return t;
         }
    };

    template <typename T>
    struct is_polygon_set_type<eXl::Vector<T> > {
      typedef typename gtl_or<
        typename is_polygonal_concept<typename geometry_concept<eXl::Vector<T> >::type>::type,
        typename is_polygonal_concept<typename geometry_concept<typename eXl::Vector<T>::value_type>::type>::type>::type type;
    };

    template <typename T>
    struct is_mutable_polygon_set_type<eXl::Vector<T> > {
      typedef typename gtl_or<
        typename gtl_same_type<polygon_set_concept, typename geometry_concept<eXl::Vector<T> >::type>::type,
        typename is_polygonal_concept<typename geometry_concept<typename eXl::Vector<T>::value_type>::type>::type>::type type;
    };

    template <typename T>
    struct polygon_set_mutable_traits<eXl::Vector<T> > {
      template <typename input_iterator_type>
      static inline void set(eXl::Vector<T>& polygon_set, input_iterator_type input_begin, input_iterator_type input_end) {
        polygon_set.clear();
        size_t num_ele = std::distance(input_begin, input_end);
        polygon_set.reserve(num_ele);
        polygon_set_data<typename polygon_set_traits<std::list<T> >::coordinate_type> ps;
        ps.reserve(num_ele);
        ps.insert(input_begin, input_end);
        ps.get(polygon_set);
      }
    };
  
  }
}

namespace eXl
{
  class Serializer;

  template <typename PolygonType>
  Err Stream_T(PolygonType& iPoly, Serializer iStreamer);

  template <typename Real> 
  struct PreciseVector
  {
    typedef eXl::Vector2<typename eXl::PreciseType<Real>::type > type; 
  };

  template <typename Real> 
  inline typename PreciseVector<Real>::type ToPrecise(Vector2<Real> const& iVec)
  {
    return typename PreciseVector<Real>::type(Math<Real>::ToPrecise(iVec.X()), Math<Real>::ToPrecise(iVec.Y()));
  }

  template <typename Real> 
  inline bool VectorNotNull(Vector2<Real> const& iVec)
  {
    return iVec.Length() > Math<Real>::ZERO_TOLERANCE;
  }

  template <> 
  inline bool VectorNotNull<int>(Vector2i const& iVec)
  {
    return iVec != Vector2i::ZERO;
  }

  template <typename Real>
  Polygon<Real>::Polygon(){}  

  template <typename Real>
  Polygon<Real>::~Polygon(){}

  template <typename Real>
  void Polygon<Real>::Translate(Vector2<Real> const& iTrans)
  {
    for (unsigned int i = 0; i < m_Ext.size(); ++i)
    {
      m_Ext[i] += iTrans;
    }
    
    for (unsigned int i = 0; i < m_Holes.size(); ++i)
    {
      for (unsigned int j = 0; j < m_Holes[i].size(); ++j)
      {
        m_Holes[i][j] += iTrans;
      }
    }
    //UpdateAABB();
    m_AABB.m_Data[0] += iTrans;
    m_AABB.m_Data[1] += iTrans;
  }

  template <typename Real>
  void Polygon<Real>::Scale(Real iNum, Real iDenom)
  {
    //Polygone valide si edge confondues ??
    for (unsigned int i = 0; i < m_Ext.size(); ++i)
    {
      m_Ext[i] = (m_Ext[i] * iNum) / iDenom;
    }
    
    for (unsigned int i = 0; i < m_Holes.size(); ++i)
    {
      for (unsigned int j = 0; j < m_Holes[i].size(); ++j)
      {
        m_Holes[i][j] = (m_Holes[i][j] * iNum) / iDenom;
      }
    }
    UpdateAABB();
  }

  template <typename Real>
  void Polygon<Real>::Rotate(typename eXl::PreciseType<Real>::type iAngle)
  {
    if(Math<typename eXl::PreciseType<Real>::type>::Abs(iAngle) > Math<typename eXl::PreciseType<Real>::type>::ZERO_TOLERANCE)
    {
      auto xAxis = Vector2<typename eXl::PreciseType<Real>::type>(Math<typename eXl::PreciseType<Real>::type>::Cos(iAngle), Math<typename eXl::PreciseType<Real>::type>::Sin(iAngle));
      auto yAxis = Vector2<typename eXl::PreciseType<Real>::type>(-xAxis.Y(), xAxis.X());

      for (unsigned int i = 0; i < m_Ext.size(); ++i)
      {
        auto vec = xAxis * m_Ext[i].X() + yAxis * m_Ext[i].Y();
        m_Ext[i] = Vector2<Real>(Math<Real>::Round(vec.X()), Math<Real>::Round(vec.Y()));
      }
    
      for (unsigned int i = 0; i < m_Holes.size(); ++i)
      {
        for (unsigned int j = 0; j < m_Holes[i].size(); ++j)
        {
          auto vec = xAxis * m_Holes[i][j].X() + yAxis * m_Holes[i][j].Y();
          m_Holes[i][j] = Vector2<Real>(Math<Real>::Round(vec.X()), Math<Real>::Round(vec.Y()));
        }
      }

      UpdateAABB();
    }
  }

  template <typename Real>
  Polygon<Real>::Polygon(Vector<Vector2<Real> > const& iPoints)
  {
    if(iPoints.size() > 2)
    {
      m_Ext = iPoints;
      if(m_Ext.front() != m_Ext.back())
        m_Ext.push_back(m_Ext.front());
      ForceClockwise();
    }
    UpdateAABB();
  }

  template <typename Real>
  void Polygon<Real>::UpdateAABB()
  {
    boost::polygon::extents(m_AABB, *this);
  }

  template <typename Real>
  Polygon<Real>::Polygon(AABB2D<Real> const& iBox)
  {
    m_Ext.clear();
    
    m_Ext.push_back(iBox.m_Data[0]);
    m_Ext.push_back(Vector2<Real>(iBox.m_Data[0].X(), iBox.m_Data[1].Y()));
    m_Ext.push_back(iBox.m_Data[1]);
    m_Ext.push_back(Vector2<Real>(iBox.m_Data[1].X(), iBox.m_Data[0].Y()));
    m_Ext.push_back(iBox.m_Data[0]);

    ForceClockwise();
    
    m_AABB = iBox;
  }

  template <typename Real>
  void Polygon<Real>::Swap(Polygon& iOther)
  {
    AABB2D<Real> temp = iOther.m_AABB;
    iOther.m_Ext.swap(m_Ext);
    iOther.m_Holes.swap(m_Holes);
    iOther.m_AABB = m_AABB;
    m_AABB = temp;
  }


  template <typename Real>
  Real Polygon<Real>::Perimeter()const
  {
    return boost::polygon::perimeter(*this);
  }

  template <typename Real>
  Real Polygon<Real>::Area() const
  {
    return boost::polygon::area(*this);
  }

  template <typename Real>
  void Polygon<Real>::ForceClockwise()
  {
    if(boost::polygon::winding(*this) != boost::polygon::CLOCKWISE)
    {
      std::reverse(m_Ext.begin(),m_Ext.end());
    }
  }

  template <typename Real>
  void Polygon<Real>::ForceCClockwiseHoles()
  {
    for(unsigned int i = 0;i<m_Holes.size();++i)
    {
      if(boost::polygon::winding(m_Holes[i]) != boost::polygon::COUNTERCLOCKWISE)
      {
        std::reverse(m_Holes[i].begin(),m_Holes[i].end());
      }
    }
  }

  template <typename Real>
  void Polygon<Real>::InternalSwap(Polygon &iOther)const
  {
    m_Ext.swap(iOther.m_Ext);
    m_Holes.swap(iOther.m_Holes);
  }

  template <typename Real>
  void Polygon<Real>::Clear()
  {
    m_Ext.clear();
    m_Holes.clear();
  }

  template <typename Real>
  bool Polygon<Real>::Empty() const
  {
    return m_Ext.empty();
  }

  template <typename Real>
  bool Polygon<Real>::ContainsPoint(Vector2<Real> const& iPoint) const
  {
    return boost::polygon::contains(*this, iPoint);
  }

  //template <typename Real>
  //void Polygon<Real>::GetBoxes(Vector<AABB2D<Real> >& oBoxes)const
  //{
  //  Vector<Polygon> meSet;
  //  meSet.push_back(Polygon());
  //  InternalSwap(meSet[0]);
  //
  //  boost::polygon::get_rectangles(oBoxes,meSet);
  //
  //  InternalSwap(meSet[0]);
  //}

  template <typename Real>
  void Polygon<Real>::Merge(Vector<Polygon>& ioPoly)
  {
    Vector<Polygon> res;
    if(ioPoly.size() > 0)
    {
      res.push_back(ioPoly.front());
      for(unsigned int i = 1 ; i<ioPoly.size();++i)
      {
        boost::polygon::operators::operator|=(res,ioPoly[i]);
      }
    }
    ioPoly.swap(res);
  }

  template <typename Real>
  void Polygon<Real>::Union(Polygon const& iOther, Polygon& oPoly)const
  {   
    oPoly.Clear();

    Vector<Polygon> meSet;
    Vector<Polygon> otherSet;
    
    meSet.push_back(Polygon());
    InternalSwap(meSet[0]);
    
    otherSet.push_back(Polygon());
    iOther.InternalSwap(otherSet[0]);
    
    Vector<Polygon> result;
    boost::polygon::assign(result,boost::polygon::operators::operator+(meSet,otherSet));

    InternalSwap(meSet[0]);
    iOther.InternalSwap(otherSet[0]);
    if (result.size() == 1)
    {
      oPoly.Swap(result[0]);
    }
  }

  template <typename Real>
  void Polygon<Real>::Difference(Polygon const& iOther, Vector<Polygon>& oPoly)const
  {
    oPoly.clear();

    Vector<Polygon> meSet;
    Vector<Polygon> otherSet;
    
    meSet.push_back(Polygon());
    InternalSwap(meSet[0]);
    
    otherSet.push_back(Polygon());
    iOther.InternalSwap(otherSet[0]);

    boost::polygon::assign(oPoly,boost::polygon::operators::operator-(meSet,otherSet));

    InternalSwap(meSet[0]);
    iOther.InternalSwap(otherSet[0]);
  }

  template <typename Real>
  void Polygon<Real>::Intersection(Polygon const& iOther, Vector<Polygon>& oPoly)const
  {
    Vector<Polygon> meSet;
    Vector<Polygon> otherSet;
    
    meSet.push_back(Polygon());
    InternalSwap(meSet[0]);
    
    otherSet.push_back(Polygon());
    iOther.InternalSwap(otherSet[0]);

    boost::polygon::assign(oPoly,boost::polygon::operators::operator&(meSet,otherSet));

    InternalSwap(meSet[0]);
    iOther.InternalSwap(otherSet[0]);

  }

  template <typename Real>
  Vector<Polygon<Real> > Polygon<Real>::GetTrapezoids() const
  {
    Vector< Polygon<Real> > result;
    Vector<Polygon> meSet;
    
    meSet.push_back(Polygon());
    InternalSwap(meSet[0]);

    boost::polygon::get_trapezoids(result, meSet, boost::polygon::VERTICAL);

    InternalSwap(meSet[0]);

    return result;
  }

  template <typename Real>
  void Polygon<Real>::CutConvex(Vector2<Real> const& iOrig, Vector2<Real> const& iDir, Polygon& oLeftPoly, Polygon& oRightPoly) const
  {
    oLeftPoly.Clear();
    oRightPoly.Clear();

    auto lowPt = ToPrecise(m_AABB.m_Data[0]);
    auto highPt = ToPrecise(m_AABB.m_Data[1]);

    typename PreciseVector<Real>::type boxSeg[5] = 
    {lowPt,  typename PreciseVector<Real>::type(lowPt.X(), highPt.Y()),
     highPt, typename PreciseVector<Real>::type(highPt.X(), lowPt.Y()),
     lowPt
    };

    unsigned int curPoint = 0;
    Vector2<Real> cutPoints[2];

    auto dOrig = ToPrecise(iOrig);
    auto dDir  = ToPrecise(iDir);

    for (unsigned int i = 0; i < 4 && curPoint < 2; ++i)
    {
      typename PreciseVector<Real>::type res;
      unsigned int result = Segment<typename PreciseType<Real>::type>::Intersect(dOrig, dOrig + dDir, boxSeg[i], boxSeg[i + 1], res);
      unsigned int mask = Segment<typename PreciseType<Real>::type>::PointFound | Segment<typename PreciseType<Real>::type>::PointOnSegment2;
      if((result & mask) == mask)
      {
        Vector2<Real> potPoint(Math<Real>::Round(res.X()), Math<Real>::Round(res.Y()));
        if (curPoint == 0 || VectorNotNull(cutPoints[0] - potPoint))
        {
          //Snap to bbox seg if integer.
          cutPoints[curPoint] = potPoint;
          ++curPoint;
        }
      }
    }

    if (curPoint == 2)
    {
      Vector2<Real> cutVect = cutPoints[0] - cutPoints[1];
      if (VectorNotNull(cutVect))
      {
        int origPoint = cutVect.Dot(iDir) > 0 ? 0 : 1;

        typename Segment<Real>::SortByAngle sortMeth(cutPoints[origPoint]);
        std::set<Vector2<Real>, typename Segment<Real>::SortByAngle > sortedPoints(sortMeth);
        sortedPoints.insert(cutPoints[1 - origPoint]);
        sortedPoints.insert(m_AABB.m_Data[0]);
        sortedPoints.insert(m_AABB.m_Data[1]);
        sortedPoints.insert(Vector2<Real>(m_AABB.m_Data[0].X(), m_AABB.m_Data[1].Y()));
        sortedPoints.insert(Vector2<Real>(m_AABB.m_Data[1].X(), m_AABB.m_Data[0].Y()));

        Vector<Vector2<Real> > cutPoly;

        cutPoly.push_back(cutPoints[origPoint]);

        auto iter = sortedPoints.begin();
        while (*iter != cutPoints[1 - origPoint] && iter != sortedPoints.end())
        {
          cutPoly.push_back(*iter);
          ++iter;
        }
        //eXl_ASSERT(iter != sortedPoints.end(), "Error");
        //eXl_ASSERT(cutPoly.size() > 3, "Error");
        if (iter != sortedPoints.end() && cutPoly.size() > 0)
        {
          cutPoly.push_back(*iter);
          Vector<Polygon<Real> > oPoly1;
          Polygon<Real> leftPoly(cutPoly);
          Intersection(leftPoly, oPoly1);
          //eXl_ASSERT(oPoly1.size() == 0 || oPoly1.size() == 1, "Error");
          if (oPoly1.size() == 1)
          {
            Vector<Polygon<Real> > oPoly2;
            Difference(oPoly1[0], oPoly2);
            //eXl_ASSERT(oPoly2.size() == 0 || oPoly2.size() == 1, "Error");
            if (oPoly2.size() < 2)
            {
              if (oPoly2.size() == 1)
              {
                oRightPoly.Swap(oPoly2[0]);
              }
              oLeftPoly.Swap(oPoly1[0]);
            }
          }
          else if (oPoly1.size() == 0)
          {
            oRightPoly = *this;
          }
        }
      }
    }
  }

  template <typename Real>
  bool Polygon<Real>::IsConvex() const
  {
    if(!m_Holes.empty())
      return false;

    if(m_Ext.size() <= 2)
      return true;

    Vector2<Real> initPoint = m_Ext[0];
    Vector2<Real> lastPoint1 = initPoint;
    Vector2<Real> lastPoint2 = m_Ext[1];
    Vector2<Real> curPoint = m_Ext[2];

    Real initSign = Segment<Real>::IsLeft(lastPoint1, lastPoint2, curPoint);
    initSign = initSign > 0 ? 1 : -1; 
    lastPoint1 = lastPoint2;
    lastPoint2 = curPoint;
    //+1 to loop on the last segment.
    for (unsigned int i = 3; i < m_Ext.size() + 1; ++i)
    {
      if (m_Ext.size() == i)
      {
        curPoint = m_Ext[0];
        if(Segment<Real>::IsLeft(lastPoint1, lastPoint2, curPoint) * initSign < 0)
          return false;
      }
      else
      {
        curPoint = m_Ext[i];
        if (curPoint == initPoint)
        {
          if(Segment<Real>::IsLeft(lastPoint1, lastPoint2, curPoint) * initSign < 0)
            return false;
          break;
        }
        if(Segment<Real>::IsLeft(lastPoint1, lastPoint2, curPoint) * initSign < 0)
          return false;

      }
      lastPoint1 = lastPoint2;
      lastPoint2 = curPoint;
    }
    return true;
  }

  template <class Real>
  struct SortPoints
  {
    bool operator()(Vector2<Real> const& iPt1, Vector2<Real> const& iPt2)
    {
      if(iPt1.X() == iPt2.X())
        return iPt1.Y() < iPt2.Y();
      return iPt1.X() < iPt2.X();
    }
  };

  template <class Real>
  void Polygon<Real>::ConvexHull(Polygon<Real>& oHull) const
  {
    return ConvexHull(m_Ext, oHull);
  }

  template <class Real>
  void Polygon<Real>::ConvexHull(Vector<Vector2<Real> >const& iPoints, Polygon<Real>& oHull)
  {
    oHull.Clear();
    if(!iPoints.empty())
    {
      Vector<Vector2<Real> > sorted = iPoints;

      std::sort(sorted.begin(), sorted.end(), SortPoints<Real>());

      int n = sorted.size();
      int k = 0;
	    Vector<Vector2<Real> > H(2*n);

	    
	    for (int i = 0; i < n; ++i) 
      {
		    while (k >= 2 && Segment<Real>::IsLeft(H[k-2], H[k-1], sorted[i]) <= 0) 
          k--;

		    H[k++] = sorted[i];
	    }

	    // Build upper hull
	    for (int i = n-2, t = k+1; i >= 0; i--) 
      {
		    while (k >= t && Segment<Real>::IsLeft(H[k-2], H[k-1], sorted[i]) <= 0) 
          k--;

		    H[k++] = sorted[i];
	    }

	    H.resize(k);

      oHull = Polygon<Real>(H);
    }
  }

  template <class Real>
  void Polygon<Real>::RemoveUselessPoints()
  {
    _RemoveUselessPoints(m_Ext);
    for(unsigned int i = 0; i<m_Holes.size(); ++i)
    {
      _RemoveUselessPoints(m_Holes[i]);
    }
  }

  template <class Real>
  void Polygon<Real>::_RemoveUselessPoints(Vector<Vector2<Real> >& ioPoints)
  {
    Vector2<Real> prevPt1 = ioPoints.back();
    Vector2<Real> prevPt2 = ioPoints.back();
    for (unsigned int i = 0; i<ioPoints.size(); ++i)
    {
      Vector2<Real> curPt =  ioPoints[i];
      if (curPt != prevPt1 && prevPt1 != prevPt2)
      {
        typename PreciseVector<Real>::type dir1 = ToPrecise<Real>(prevPt1 - prevPt2);
        typename PreciseVector<Real>::type dir2 = ToPrecise<Real>(curPt - prevPt2);
        typename PreciseType<Real>::type len1 = dir1.Normalize();
        typename PreciseType<Real>::type len2 = dir2.Normalize();
        if (dir1.Dot(dir2) > (1 - eXl::Math<typename PreciseType<Real>::type>::EPSILON) && len1 < len2)
        {
          ioPoints[i - 1] = curPt;
          ioPoints.erase(ioPoints.begin() + i);
          prevPt1 = curPt;
          --i;
          continue;
        }
      }
      prevPt2 = prevPt1;
      prevPt1 = curPt;
    }
  }

  template <typename Real>
  void Polygon<Real>::Shrink(Real iFactor,Vector<Polygon>& oOut)const
  {
    Vector<Polygon> meSet;
    meSet.push_back(*this);

    boost::polygon::shrink(meSet,iFactor);

    if(meSet.size()>0)
      meSet.swap(oOut);
    else
      oOut.clear();
  }

  template <typename Real>
  void Polygon<Real>::Bloat(Real iFactor,Polygon& oOut)const
  {
    Vector<Polygon> meSet;
    meSet.push_back(*this);

    boost::polygon::bloat(meSet,iFactor);

    if(meSet.size()>0)
      meSet[0].Swap(oOut);
    else
      oOut.Clear();
  }

  template <typename Real>
  void Polygon<Real>::RemoveTiny(Real iFactor,Vector<Polygon>& oOut,Vector<Polygon>& oRemoved)const
  {
    oOut.clear();
    oRemoved.clear();
    Vector<Polygon> meSet;
    meSet.push_back(*this);

    /*meSet = */boost::polygon::shrink(meSet,iFactor);

    /*meSet = */boost::polygon::bloat(meSet,iFactor);

    //meSet = boost::polygon::keep(meSet,0,ULLONG_MAX,iFactor,ULLONG_MAX,iFactor,ULLONG_MAX);

    if(meSet.empty())
    {
      oRemoved.push_back(*this);
    }
    else
    {

      Vector<Polygon> good;
      good.push_back(Polygon());
      InternalSwap(good.back());

      boost::polygon::assign(oRemoved,boost::polygon::operators::operator-(good,meSet));
      InternalSwap(good.back());

      //meSet.back().Swap(*this);
      oOut.swap(meSet);
    }
  }
}

#include <core/stream/serializer.hpp>

namespace eXl
{

  template <typename PolygonType>
  Err Stream_T(PolygonType& iPoly, Serializer iStreamer)
  {
    iStreamer.BeginStruct();
    iStreamer.PushKey("Border");
    iStreamer &= iPoly.Border();
    iStreamer.PopKey();
    iStreamer.PushKey("Holes");
    iStreamer &= iPoly.Holes();
    iStreamer.PopKey();
    iStreamer.EndStruct();

    RETURN_SUCCESS;
  }

  template <typename Real>
  Err Polygon<Real>::Stream(Streamer& iStreamer) const
  {
    Serializer serializer(iStreamer);
    return Stream_T(*this, serializer);
  }

  template <typename Real>
  Err Polygon<Real>::Unstream(Unstreamer& iStreamer)
  {
    Serializer serializer(iStreamer);
    return Stream_T(*this, serializer);
  }
}