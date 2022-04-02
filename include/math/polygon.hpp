/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#pragma warning(disable : 4661)

#include "aabb2d.hpp"

#include <core/containers.hpp>

#include "mathexp.hpp"

#include "polygon_def.hpp"

namespace boost
{
  namespace polygon
  {
    template <typename T>
    struct polygon_traits_general;
    template <class T,typename enable>
    struct polygon_mutable_traits;
    template <class T,typename enable>
    struct polygon_with_holes_mutable_traits;
    template <typename T, typename enable>
    struct polygon_with_holes_traits;
  }
}

namespace eXl
{
  template <typename Real>
  class Polygon
  {
    template <typename T>
    friend struct boost::polygon::polygon_traits_general;
    template <class T,typename enable>
    friend struct boost::polygon::polygon_mutable_traits;
    template <class T,typename enable>
    friend struct boost::polygon::polygon_with_holes_mutable_traits;
    template <class T,typename enable>
    friend struct boost::polygon::polygon_with_holes_traits;
  public:

    typedef Vector<glm::vec<2,Real> > PtList;
    typedef Vector<PtList> PtLists;

    Polygon();

    ~Polygon();

    Polygon(Polygon const& iOther) = default;

    Err Stream(Streamer& iStreamer) const;
    Err Unstream(Unstreamer& iUnstreamer);

    Polygon& operator = (Polygon const& iOther) = default;

    template<typename U>
    inline Polygon(Polygon<U> const& iOther);

	  template<typename U>
	  inline Polygon& operator = (Polygon<U> const& iOther);

    inline Polygon(Polygon&& iOther){iOther.Swap(*this);}

    explicit Polygon(Vector<glm::vec<2,Real> > const& iPoints);

    explicit Polygon(AABB2D<Real> const& iBox);

    void Clear();

    bool Empty() const;

    void Swap(Polygon& iOther);

    static void Merge(Vector<Polygon>& oPoly);

    void Translate(glm::vec<2,Real> const& iTrans);

    void Scale(Real iNum, Real iDenom = 1);

    void Rotate(typename eXl::PreciseType<Real>::type iAngle);

    //If Intersection is empty, Union is empty too.
    void Union(Polygon const& iOther, Polygon& oPoly)const;

    void Difference(Polygon const& iOther, Vector<Polygon>& oPoly)const;

    void Intersection(Polygon const& iOther, Vector<Polygon>& oPoly)const;

    //Two poly only when the poly is convex.
    void CutConvex(glm::vec<2,Real> const& iOrig, glm::vec<2,Real> const& iDir, Polygon& oLeftPoly, Polygon& oRightPoly) const;

    bool IsConvex() const;

    //void GetBoxes(Vector<AABB2D<Real> >& oBoxes)const;

    void ConvexHull(Polygon<Real>& oHull) const;

    static void ConvexHull(Vector<glm::vec<2,Real> >const& iPoints, Polygon<Real>& oHull);

    void RemoveUselessPoints();

    bool ContainsPoint(glm::vec<2,Real> const& iPoint) const;

    void RemoveTiny(Real iFactor,Vector<Polygon>& oOut,Vector<Polygon>& oRemoved)const;

    void Shrink(Real iFactor,Vector<Polygon>& oOut)const;

    void Bloat(Real iFactor,Polygon& oOut)const;

    Real Perimeter()const;

    Real Area() const;

    Vector<Polygon<Real> > GetTrapezoids() const;

    inline AABB2D<Real> const& GetAABB()const{return m_AABB;}

    inline PtList const& Border() const{return m_Ext;}

    inline PtLists const& Holes() const{return m_Holes;}

    inline PtList& Border() {return m_Ext;}

    inline PtLists& Holes() {return m_Holes;}

  protected:

    static void _RemoveUselessPoints(Vector<glm::vec<2,Real> >& ioPoints);

    void UpdateAABB();

    typedef Real coordinate_type;
    typedef typename Vector<glm::vec<2,Real> >::const_iterator iterator_type;
    typedef glm::vec<2,Real> point_type;
    
    inline iterator_type begin_points() {return m_Ext.begin();}
    inline iterator_type end_points() {return m_Ext.end();}
    inline unsigned int size(){return m_Ext.size();}
    
    typedef typename PtLists::const_iterator iterator_holes_type;
    typedef Vector<glm::vec<2,Real> > hole_type;
    inline iterator_holes_type begin_holes() {return m_Holes.begin();}
    inline iterator_holes_type end_holes() {return m_Holes.end();}
    inline unsigned int size_holes() {return m_Holes.size(); }
    
    void set(iterator_type iBegin, iterator_type iEnd){m_Ext.assign(iBegin, iEnd);}
    void set_holes(iterator_holes_type iBegin, iterator_holes_type iEnd){m_Holes.assign(iBegin, iEnd);}
    
    //int AddBox(AABB2D<Real> const& iBox);
    
    inline PtLists& HolesRW() {return m_Holes;}
    
    void InternalSwap(Polygon &iOther)const;
    
    void ForceClockwise();
    void ForceCClockwiseHoles();

    mutable PtList  m_Ext;
    mutable PtLists m_Holes;
    AABB2D<Real>    m_AABB;
    
  };

  template <typename Real>
  template<typename U>
  Polygon<Real>::Polygon(Polygon<U> const& iOther)
  {
    m_Ext.clear();
    m_Holes.clear();

    for(auto point : iOther.Border())
    {
      m_Ext.push_back(glm::vec<2,Real>(point.x, point.y));
    }

    for(auto hole : iOther.Holes())
    {
      m_Holes.push_back(PtList());
      for(auto point : hole)
      {
        m_Holes.back().push_back(glm::vec<2,Real>(point.x, point.y));
      }
    }
  }

  template <typename Real>
  template<typename U>
  Polygon<Real>& Polygon<Real>::operator=(Polygon<U> const& iOther)
  {
	  m_Ext.clear();
	  m_Holes.clear();

	  for (auto point : iOther.Border())
	  {
		  m_Ext.push_back(glm::vec<2,Real>(point.x, point.y));
	  }

	  for (auto hole : iOther.Holes())
	  {
		  m_Holes.push_back(PtList());
		  for (auto point : hole)
		  {
			  m_Holes.back().push_back(glm::vec<2,Real>(point.x, point.y));
		  }
	  }
	  return *this;
  }
  
  typedef Polygon<int>    Polygoni;
  typedef Polygon<float>  Polygonf;
  typedef Polygon<double> Polygond;
}

#if defined(EXL_SHARED_LIBRARY)
#if defined(WIN32) || 1
namespace eXl
{
  eXl_TEMPLATE_EXTERN template class EXL_MATH_API Polygon<int>;
  eXl_TEMPLATE_EXTERN template class EXL_MATH_API Polygon<float>;
  eXl_TEMPLATE_EXTERN template class EXL_MATH_API Polygon<double>;
}
#else
#include <math/polygon.inl>
#endif
#endif

