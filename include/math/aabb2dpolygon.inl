/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <math/aabb2dpolygon.hpp>
#include <math/polygon_def.hpp>
#include <boost/polygon/detail/iterator_points_to_compact.hpp>
#include <boost/polygon/detail/iterator_compact_to_points.hpp>
#include <core/containers.hpp>
namespace boost 
{ 
  namespace polygon 
  {

    template <typename Real>
    struct geometry_concept<eXl::AABB2DPolygon<Real> >{ typedef polygon_90_with_holes_concept /*polygon_90_concept*/ type; };

    template <typename Real>
    struct geometry_concept<eXl::Vector<glm::vec<2, Real> > >{ typedef polygon_90_concept type; };

    template <typename Real>
    struct geometry_concept<eXl::Vector<eXl::AABB2DPolygon<Real> > >{ typedef polygon_90_set_concept type; };

    template <typename Real>
    struct polygon_90_traits<eXl::AABB2DPolygon<Real> > {
      typedef Real coordinate_type;
      typedef iterator_points_to_compact<typename eXl::Vector<glm::vec<2,Real> >::const_iterator,glm::vec<2,Real> > compact_iterator_type;
      typedef glm::vec<2,Real> point_type;

      static inline compact_iterator_type begin_compact(eXl::AABB2DPolygon<Real> const& t) {
          return compact_iterator_type (t.Border().begin(),t.Border().end());
      }
      static inline compact_iterator_type end_compact(eXl::AABB2DPolygon<Real> const& t) {
          return compact_iterator_type (t.Border().end(),t.Border().end());
      }

      // Get the number of sides of the polygon
      static inline std::size_t size(eXl::AABB2DPolygon<Real> const& t) {
        return t.Border().size();
      }

      // Get the winding direction of the polygon
      static inline winding_direction winding(eXl::AABB2DPolygon<Real> const& t) {
        return unknown_winding;
      }
    };

    template <typename Real>
    struct polygon_90_mutable_traits<eXl::AABB2DPolygon<Real> > {

      template <typename iT>
      static inline eXl::AABB2DPolygon<Real>& set_compact(eXl::AABB2DPolygon<Real> & t, 
                                         iT input_begin, iT input_end) {

        eXl::Vector<glm::vec<2,Real>> tempStor;
        iterator_compact_to_points<iT,glm::vec<2,Real>> iterBegin(input_begin,input_end);
        iterator_compact_to_points<iT,glm::vec<2,Real>> iterEnd(input_end,input_end);

        tempStor.assign(iterBegin,iterEnd);

        t = eXl::AABB2DPolygon<Real>(tempStor);
        t.ForceClockwise();
        return t;
      }

    };

    template <typename Real>
    struct polygon_90_traits<eXl::Vector<glm::vec<2,Real> > > {
      typedef Real coordinate_type;
      typedef iterator_points_to_compact<typename eXl::Vector<glm::vec<2,Real> >::const_iterator,glm::vec<2,Real> > compact_iterator_type;
      typedef glm::vec<2,Real> point_type;

      static inline compact_iterator_type begin_compact(typename eXl::AABB2DPolygon<Real>::PtList const& t) {
          return compact_iterator_type (t.begin(),t.end());
      }
      static inline compact_iterator_type end_compact(typename eXl::AABB2DPolygon<Real>::PtList const& t) {
          return compact_iterator_type (t.end(),t.end());
      }

      // Get the number of sides of the polygon
      static inline std::size_t size(typename eXl::AABB2DPolygon<Real>::PtList const& t) {
        return t.size();
      }

      // Get the winding direction of the polygon
      static inline winding_direction winding(typename eXl::AABB2DPolygon<Real>::PtList const& t) {
        return unknown_winding;
      }
    };

    template <typename Real>
    struct polygon_90_mutable_traits<eXl::Vector<glm::vec<2,Real> > > {

      template <typename iT>
      static inline typename eXl::AABB2DPolygon<Real>::PtList& set_compact(typename eXl::AABB2DPolygon<Real>::PtList & t, 
                                         iT input_begin, iT input_end) {

        iterator_compact_to_points<iT,glm::vec<2,Real> > iterBegin(input_begin,input_end);
        iterator_compact_to_points<iT,glm::vec<2,Real> > iterEnd(input_end,input_end);
        t.assign(iterBegin,iterEnd);

        return t;
      }

    };

    template <typename Real,typename enable>
    struct polygon_with_holes_traits<eXl::AABB2DPolygon<Real>,enable> {
         typedef typename eXl::AABB2DPolygon<Real>::PtLists::const_iterator iterator_holes_type;
         typedef typename eXl::AABB2DPolygon<Real>::PtList hole_type;
         static inline iterator_holes_type begin_holes(const eXl::AABB2DPolygon<Real>& t) {
              return t.Holes().begin();
         }
         static inline iterator_holes_type end_holes(const eXl::AABB2DPolygon<Real>& t) {
              return t.Holes().end();
         }
         static inline Real size_holes(const eXl::AABB2DPolygon<Real>& t) {
              return t.Holes().size();
         }
    };

    template <typename Real, typename enable>
    struct polygon_with_holes_mutable_traits<eXl::AABB2DPolygon<Real>,enable> {
         template <typename iT>
         static inline eXl::AABB2DPolygon<Real>& set_holes(eXl::AABB2DPolygon<Real>& t, iT inputBegin, iT inputEnd) {
              //t.Holes().assign(inputBegin, inputEnd);
              for(;inputBegin != inputEnd;++inputBegin)
              {
                typename eXl::AABB2DPolygon<Real>::PtList hole;

                polygon_90_mutable_traits<typename eXl::AABB2DPolygon<Real>::PtList>::set_compact(hole, polygon_90_traits<decltype(*inputBegin)>::begin_compact(*inputBegin),
                  polygon_90_traits<decltype(*inputBegin)>::end_compact(*inputBegin));

                t.HolesRW().push_back(typename eXl::AABB2DPolygon<Real>::PtList());
                t.HolesRW().back().swap(hole);
              }
              t.ForceCClockwiseHoles();
              return t;
         }
    };

    //template <typename T>
    //struct is_polygon_90_set_type<eXl::Vector<T> > {
    //  typedef gtl_yes type;
    //};
    //
    //template <typename T>
    //struct is_mutable_polygon_90_set_type<eXl::Vector<T> > {
    //  typedef gtl_yes type;
    //};

    template <typename Real>
    struct polygon_90_set_traits<eXl::Vector<eXl::AABB2DPolygon<Real>> >
    {
      typedef Real coordinate_type;
      typedef typename eXl::Vector<eXl::AABB2DPolygon<Real>>::const_iterator iterator_type;
      typedef eXl::Vector<eXl::AABB2DPolygon<Real>> operator_arg_type;
    
      static inline iterator_type begin(const eXl::Vector<eXl::AABB2DPolygon<Real>>& polygon_set) 
      {
        return polygon_set.begin();
      }
    
      static inline iterator_type end(const eXl::Vector<eXl::AABB2DPolygon<Real>>& polygon_set) 
      {
        return polygon_set.end();
      }
    
      static inline orientation_2d orient(const eXl::Vector<eXl::AABB2DPolygon<Real>>&) { return HORIZONTAL; }
    
      static inline bool clean(const eXl::Vector<eXl::AABB2DPolygon<Real>>&) { return false; }
    
      static inline bool sorted(const eXl::Vector<eXl::AABB2DPolygon<Real>>&) { return false; }
    };

    template <typename Real>
    struct polygon_90_set_mutable_traits<eXl::Vector<eXl::AABB2DPolygon<Real>> > 
    {
      typedef polygon_90_with_holes_concept concept_type;
      template <typename input_iterator_type>
      static inline void set(eXl::Vector<eXl::AABB2DPolygon<Real>>& polygon_set, input_iterator_type input_begin, input_iterator_type input_end, orientation_2d orient) 
      {
        polygon_set.clear();
        size_t num_ele = std::distance(input_begin, input_end);
        polygon_set.reserve(num_ele);
        polygon_90_set_data<Real> ps(orient);
        ps.reserve(num_ele);
        ps.insert(input_begin, input_end, orient);
        ps.clean();
        get_90_dispatch(polygon_set, ps, orient, concept_type());
      }
    };

} }

namespace eXl
{
  class Serializer;
  template <typename PolygonType>
  Err Stream_T(PolygonType& iPoly, Serializer iStreamer);

  template <typename Real> 
  struct PreciseVector
  {
    typedef glm::vec<2, typename eXl::PreciseType<Real>::type > type; 
  };

  template <typename Real> 
  inline typename PreciseVector<Real>::type ToPrecise(glm::vec<2,Real> const& iVec)
  {
    return typename PreciseVector<Real>::type(Math<Real>::ToPrecise(iVec.x), Math<Real>::ToPrecise(iVec.y));
  }

  template <typename Real> 
  inline bool VectorNotnullptr(glm::vec<2,Real> const& iVec)
  {
    return iVec.Length() > Math<Real>::ZeroTolerance();
  }

  template <> 
  inline bool VectorNotnullptr<int>(Vec2i const& iVec)
  {
    return iVec != Zero<Vec2i>();
  }

  template <typename Real>
  AABB2DPolygon<Real>::AABB2DPolygon(){}  

  template <typename Real>
  void AABB2DPolygon<Real>::Translate(glm::vec<2,Real> const& iTrans)
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
    m_AABB.m_Data[0] += iTrans;
    m_AABB.m_Data[1] += iTrans;
  }

  template <typename Real>
  void AABB2DPolygon<Real>::Scale(Real iNum, Real iDenom)
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

    boost::polygon::extents(m_AABB, *this);
  }

  template <typename Real>
  void AABB2DPolygon<Real>::ScaleComponents(Real iNumX, Real iNumY, Real iDenomX, Real iDenomY)
  {
    for (unsigned int i = 0; i < m_Ext.size(); ++i)
    {
      m_Ext[i].x = (m_Ext[i].x * iNumX) / iDenomX;
      m_Ext[i].y = (m_Ext[i].y * iNumY) / iDenomY;
    }

    for (unsigned int i = 0; i < m_Holes.size(); ++i)
    {
      for (unsigned int j = 0; j < m_Holes[i].size(); ++j)
      {
        m_Holes[i][j].x = (m_Holes[i][j].x * iNumX) / iDenomX;
        m_Holes[i][j].y = (m_Holes[i][j].y * iNumY) / iDenomY;
      }
    }

    boost::polygon::extents(m_AABB, *this);
  }

  template <typename Real>
  AABB2DPolygon<Real>::AABB2DPolygon(Vector<glm::vec<2,Real>> const& iPoints)
  {
    m_Ext.clear();
    
    if(iPoints.size()>=4)
    {
      m_Ext = iPoints;
      if((m_Ext[0]-m_Ext[1]).y != 0)
      {
        glm::vec<2,Real> tempPt = m_Ext[0];
        //memmove(&m_Points[0],&m_Points[1],m_Points.size() * sizeof(glm::vec<2,Real>));
        m_Ext.erase(m_Ext.begin());
        m_Ext.push_back(tempPt);
      }
    }
    if(m_Ext.front() != m_Ext.back())
      m_Ext.push_back(m_Ext.front());

    boost::polygon::extents(m_AABB,*this);
  }

  template <typename Real>
  AABB2DPolygon<Real>::AABB2DPolygon(AABB2D<Real> const& iBox)
  {
    AddBox(iBox);
    if(m_Ext.front() != m_Ext.back())
      m_Ext.push_back(m_Ext.front());
  }

  template <typename Real>
  void AABB2DPolygon<Real>::RemoveUselessPoints()
  {
    _RemoveUselessPoints(m_Ext);
    for(unsigned int i = 0; i<m_Holes.size(); ++i)
    {
      _RemoveUselessPoints(m_Holes[i]);
    }
  }

  template <class Real>
  void AABB2DPolygon<Real>::_RemoveUselessPoints(Vector<glm::vec<2,Real> >& ioPoints)
  {
    if(ioPoints.size() > 2)
    {
      if(ioPoints.back() == ioPoints.front())
      {
        ioPoints.pop_back();
      }

      for (uint32_t i = 0; i < ioPoints.size() - 1; ++i)
      {
        if (ioPoints[i] == ioPoints[i + 1])
        {
          ioPoints.erase(ioPoints.begin() + i + 1);
        }
      }

      glm::vec<2,Real> prevPt1 = ioPoints[ioPoints.size() - 1];
      glm::vec<2,Real> prevPt2 = ioPoints[ioPoints.size() - 2];
      for (int i = 0; i<(int)ioPoints.size(); ++i)
      {
        unsigned int prevIdx = i > 0 ? i - 1 : (i == -1 ? ioPoints.size() - 2 : ioPoints.size() - 1);
        unsigned int curIdx = i >= 0 ? i : ioPoints.size() - 1;
        glm::vec<2,Real> curPt = ioPoints[curIdx];
        if (curPt != prevPt1 && prevPt1 != prevPt2)
        {
          typename PreciseVector<Real>::type dir1 = ToPrecise<int>(prevPt1 - prevPt2);
          typename PreciseVector<Real>::type dir2 = ToPrecise<int>(curPt - prevPt2);
          typename PreciseType<Real>::type len1 = NormalizeAndGetLength(dir1);
          typename PreciseType<Real>::type len2 = NormalizeAndGetLength(dir2);

          if (dot(dir1, dir2) > (1 - eXl::Math<typename PreciseType<Real>::type>::Epsilon()) && len1 < len2)
          {
            ioPoints[prevIdx] = curPt;
            ioPoints.erase(ioPoints.begin() + curIdx);
            prevPt1 = curPt;
            --i;
            continue;
          }
        }
        prevPt2 = prevPt1;
        prevPt1 = curPt;
      }

      if(ioPoints.front() != ioPoints.back())
        ioPoints.push_back(ioPoints.front());
    }
    else
    {
      ioPoints.clear();
    }
  }

  template <typename Real>
  void AABB2DPolygon<Real>::Swap(AABB2DPolygon& iOther)
  {
    AABB2D<Real> temp = iOther.m_AABB;
    iOther.m_Ext.swap(m_Ext);
    iOther.m_Holes.swap(m_Holes);
    iOther.m_AABB = m_AABB;
    m_AABB = temp;
  }

  template <typename Real>
  Real AABB2DPolygon<Real>::Perimeter()const
  {
    return boost::polygon::perimeter(*this);
  }

  template <typename Real>
  Real AABB2DPolygon<Real>::Area() const
  {
    return boost::polygon::area(*this);
  }

  template <typename Real>
  void AABB2DPolygon<Real>::ForceClockwise()
  {
    if(boost::polygon::winding(*this) != boost::polygon::CLOCKWISE)
    {
      std::reverse(m_Ext.begin(),m_Ext.end());
    }
  }

  template <typename Real>
  void AABB2DPolygon<Real>::ForceCClockwiseHoles()
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
  void AABB2DPolygon<Real>::InternalSwap(AABB2DPolygon &iOther)const
  {
    m_Ext.swap(iOther.m_Ext);
    m_Holes.swap(iOther.m_Holes);
    std::swap(iOther.m_AABB, m_AABB);
  }

  template <typename Real>
  void AABB2DPolygon<Real>::Clear()
  {
    m_Ext.clear();
    m_Holes.clear();
    m_AABB = AABB2D<Real>();
  }

  template <typename Real>
  bool AABB2DPolygon<Real>::Empty() const
  {
    return m_Ext.empty();
  }

  template <typename Real>
  void AABB2DPolygon<Real>::GetBoxes(Vector<AABB2D<Real> >& oBoxes)const
  {
    Vector<AABB2DPolygon> meSet;
    meSet.push_back(AABB2DPolygon());
    InternalSwap(meSet[0]);

    Vector<AABB2D<Real>> vBoxes;
    boost::polygon::get_rectangles(vBoxes,meSet);

    Vector<AABB2DPolygon> swappedPoly(1, meSet[0]);
    for(auto& pt : swappedPoly[0].Border())
    {
      std::swap(pt.x, pt.y);
    }
    for(auto& hole : swappedPoly[0].Holes())
    {
      for(auto& pt : hole)
      {
        std::swap(pt.x, pt.y);
      }
    }

    Vector<AABB2D<Real>> hBoxes;
    boost::polygon::get_rectangles(hBoxes,swappedPoly);

    for(auto const& hBoxSwapped : hBoxes)
    {
      AABB2D<Real> hBox = hBoxSwapped;
      std::swap(hBox.m_Data[0].x, hBox.m_Data[0].y);
      std::swap(hBox.m_Data[1].x, hBox.m_Data[1].y);
      for(auto const& vBox : vBoxes)
      {
        AABB2D<Real> commonBox;
        commonBox.SetCommonBox(vBox, hBox);
        if(!commonBox.Empty())
        {
          oBoxes.push_back(commonBox);
        }
      }
    }

    InternalSwap(meSet[0]);
  }

  template <typename Real>
  void AABB2DPolygon<Real>::Merge(Vector<AABB2DPolygon>& ioPoly)
  {
    Vector<AABB2DPolygon> res;
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
  void AABB2DPolygon<Real>::Union(AABB2DPolygon const& iOther, AABB2DPolygon& oPoly)const
  {   
    oPoly.Clear();

    Vector<AABB2DPolygon> meSet;
    Vector<AABB2DPolygon> otherSet;

    meSet.push_back(AABB2DPolygon());
    InternalSwap(meSet[0]);

    otherSet.push_back(AABB2DPolygon());
    iOther.InternalSwap(otherSet[0]);

    Vector<AABB2DPolygon> result;
    boost::polygon::assign(result,boost::polygon::operators::operator+(meSet,otherSet));

    InternalSwap(meSet[0]);
    iOther.InternalSwap(otherSet[0]);
    if(result.size() == 1)
      oPoly.Swap(result[0]);
  }

  template <typename Real>
  void AABB2DPolygon<Real>::Difference(AABB2DPolygon const& iOther, Vector<AABB2DPolygon>& oPoly)const
  {
    oPoly.clear();

    Vector<AABB2DPolygon> meSet;
    Vector<AABB2DPolygon> otherSet;
    
    meSet.push_back(AABB2DPolygon());
    InternalSwap(meSet[0]);

    otherSet.push_back(AABB2DPolygon());
    iOther.InternalSwap(otherSet[0]);

    boost::polygon::assign(oPoly,boost::polygon::operators::operator-(meSet,otherSet));

    InternalSwap(meSet[0]);
    iOther.InternalSwap(otherSet[0]);

  }

  template <typename Real>
  void AABB2DPolygon<Real>::Intersection(AABB2DPolygon const& iOther, Vector<AABB2DPolygon>& oPoly)const
  {
    Vector<AABB2DPolygon> meSet;
    Vector<AABB2DPolygon> otherSet;
    
    meSet.push_back(AABB2DPolygon());
    InternalSwap(meSet[0]);

    otherSet.push_back(AABB2DPolygon());
    iOther.InternalSwap(otherSet[0]);

    boost::polygon::assign(oPoly,boost::polygon::operators::operator&(meSet,otherSet));

    InternalSwap(meSet[0]);
    iOther.InternalSwap(otherSet[0]);
  }

  template <typename Real>
  void AABB2DPolygon<Real>::Shrink(Real iFactor,Vector<AABB2DPolygon>& oOut)const
  {
    Vector<AABB2DPolygon> meSet;
    meSet.push_back(*this);

    boost::polygon::shrink(meSet,iFactor/2,iFactor/2,iFactor/2,iFactor/2);

    if(meSet.size()>0)
      meSet.swap(oOut);
    else
      oOut.clear();
  }

  template <typename Real>
  void AABB2DPolygon<Real>::Bloat(Real iFactor,AABB2DPolygon& oOut)const
  {
    Vector<AABB2DPolygon> meSet;
    meSet.push_back(*this);

    boost::polygon::bloat(meSet,iFactor/2,iFactor/2,iFactor/2,iFactor/2);

    if(meSet.size()>0)
      meSet[0].Swap(oOut);
    else
      oOut.Clear();
  }

  template <typename Real>
  void AABB2DPolygon<Real>::RemoveTiny(Real iFactor,Vector<AABB2DPolygon>& oOut,Vector<AABB2DPolygon>& oRemoved)const
  {
    oOut.clear();
    oRemoved.clear();
    Vector<AABB2DPolygon> meSet;
    meSet.push_back(*this);

    /*meSet = */boost::polygon::shrink(meSet,iFactor/2,iFactor/2,iFactor/2,iFactor/2);

    /*meSet = */boost::polygon::bloat(meSet,iFactor/2,iFactor/2,iFactor/2,iFactor/2);

    //meSet = boost::polygon::keep(meSet,0,ULLONG_MAX,iFactor,ULLONG_MAX,iFactor,ULLONG_MAX);

    if(meSet.empty())
    {
      oRemoved.push_back(*this);
    }
    else
    {

      Vector<AABB2DPolygon> good;
      good.push_back(AABB2DPolygon());
      InternalSwap(good.back());

      boost::polygon::assign(oRemoved,boost::polygon::operators::operator-(good,meSet));
      InternalSwap(good.back());

      //meSet.back().Swap(*this);
      oOut.swap(meSet);
    }
  }

  template <typename Real>
  int AABB2DPolygon<Real>::AddBox(AABB2D<Real> const& iBox)
  {
    if (Empty())
    {
      m_Ext.push_back(glm::vec<2,Real>(iBox.m_Data[1].x, iBox.m_Data[0].y));
      m_Ext.push_back(glm::vec<2,Real>(iBox.m_Data[0].x, iBox.m_Data[0].y));
      m_Ext.push_back(glm::vec<2,Real>(iBox.m_Data[0].x, iBox.m_Data[1].y));
      m_Ext.push_back(glm::vec<2,Real>(iBox.m_Data[1].x, iBox.m_Data[1].y));

      boost::polygon::extents(m_AABB, *this);

      return 1;
    }
    else
    {

      Vector<glm::vec<2,Real>> temp;
      temp.push_back(glm::vec<2,Real>(iBox.m_Data[1].x, iBox.m_Data[0].y));
      temp.push_back(glm::vec<2,Real>(iBox.m_Data[0].x, iBox.m_Data[0].y));
      temp.push_back(glm::vec<2,Real>(iBox.m_Data[0].x, iBox.m_Data[1].y));
      temp.push_back(glm::vec<2,Real>(iBox.m_Data[1].x, iBox.m_Data[1].y));

      Vector<AABB2DPolygon> meSet;
      Vector<AABB2DPolygon> boxSet;

      meSet.push_back(AABB2DPolygon());
      InternalSwap(meSet[0]);


      boxSet.push_back(AABB2DPolygon());
      boxSet[0].m_Ext.swap(temp);

      boost::polygon::operators::operator|=(boxSet, meSet);
      //meSet.clear();
      //temp.get_polygons(meSet);
      if (boxSet.size() == 1)
      {
        InternalSwap(boxSet[0]);
        boost::polygon::extents(m_AABB, *this);
        return 1;
      }
      else
      {
        InternalSwap(meSet[0]);
        return 0;
      }


      return 1;
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
  Err AABB2DPolygon<Real>::Stream(Streamer& iStreamer) const
  {
    Serializer serializer(iStreamer);
    return Stream_T(*this, serializer);
  }

  template <typename Real>
  Err AABB2DPolygon<Real>::Unstream(Unstreamer& iStreamer)
  {
    Serializer serializer(iStreamer);
    Err res = Stream_T(*this, serializer);
    if (res)
    {
      boost::polygon::extents(m_AABB, *this);
    }
    return res;
  }
}