/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "vector2.hpp"
#include "aabb2d.hpp"
#include <core/containers.hpp>

namespace boost
{
  namespace polygon
  {
    template <class T,typename enable>
    struct polygon_90_mutable_traits;
    template <class T,typename enable>
    struct polygon_with_holes_mutable_traits;
  }
}
namespace eXl
{
  template <typename Real>
  class AABB2DPolygon
  {
    template <class T,typename enable>
    friend struct boost::polygon::polygon_90_mutable_traits;
    template <class T,typename enable>
    friend struct boost::polygon::polygon_with_holes_mutable_traits;
  public:

    typedef Vector<Vector2<Real> > PtList;
    typedef Vector<PtList> PtLists;

    AABB2DPolygon();

    AABB2DPolygon(AABB2D<Real> const& iBox);

    AABB2DPolygon(Vector<Vector2<Real> > const& iPoints);

    Err Stream(Streamer& iStreamer) const;
    Err Unstream(Unstreamer& iUnstreamer);

    void Clear();

    bool Empty() const;

    void Swap(AABB2DPolygon& iOther);

    static void Merge(Vector<AABB2DPolygon>& oPoly);

    void Translate(Vector2<Real> const& iTrans);

    void Scale(Real iNum, Real iDenom = 1);
    void ScaleComponents(Real iNumX, Real iNumY, Real iDenomX = 1, Real iDenomY = 1);

    //If Intersection is empty, Union is empty too.
    void Union(AABB2DPolygon const& iOther, AABB2DPolygon& oPoly)const;

    void Difference(AABB2DPolygon const& iOther, Vector<AABB2DPolygon>& oPoly)const;

    void Intersection(AABB2DPolygon const& iOther, Vector<AABB2DPolygon>& oPoly)const;

    void GetBoxes(Vector<AABB2D<Real> >& oBoxes)const;

    void RemoveTiny(Real iFactor, Vector<AABB2DPolygon>& oOut, Vector<AABB2DPolygon>& oRemoved)const;

    void Shrink(Real iFactor, Vector<AABB2DPolygon>& oOut)const;

    void Bloat(Real iFactor,AABB2DPolygon& oOut)const;

    Real Perimeter()const;

    Real Area() const;

    inline AABB2D<Real> const& GetAABB()const{return m_AABB;}

    inline PtList const& Border() const{return m_Ext;}

    inline PtLists const& Holes() const{return m_Holes;}

    inline PtList& Border() {return m_Ext;}

    inline PtLists& Holes() {return m_Holes;}

    int AddBox(AABB2D<Real> const& iBox);

    void RemoveUselessPoints();
    
  protected:

    void _RemoveUselessPoints(Vector<Vector2<Real> >& ioPoints);
    
    inline PtLists& HolesRW() {return m_Holes;}

    void InternalSwap(AABB2DPolygon &iOther)const;

    void ForceClockwise();
    void ForceCClockwiseHoles();

    mutable PtList  m_Ext;
    mutable PtLists m_Holes;
    mutable AABB2D<Real> m_AABB;
    void*           m_Cache;
  };

  namespace TypeTraits
  {
    template <typename Real>
    struct IsComparable<AABB2DPolygon<Real>> { static constexpr bool s_Value = false; };
  }

  DEFINE_MATH_TYPE_EX(AABB2DPolygon<int>, AABB2DPolygoni)
}

#if defined(EXL_SHARED_LIBRARY)
#if defined(WIN32) || 1
namespace eXl
{
  extern template class EXL_MATH_API AABB2DPolygon<int>;
  //template class EXL_MATH_API AABB2DPolygon<float>;
  //template class EXL_MATH_API AABB2DPolygon<double>;
}

#else
#include <math/aabb2dpolygon.inl>
#endif
#endif 

namespace eXl
{
  typedef AABB2DPolygon<int>    AABB2DPolygoni;
  //typedef AABB2DPolygon<float>  AABB2DPolygonf;
  //typedef AABB2DPolygon<double> AABB2DPolygond;
}

