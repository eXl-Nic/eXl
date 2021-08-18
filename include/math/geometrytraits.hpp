/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <math/aabb2dpolygon.hpp>
#include <math/polygon.hpp>
#include <math/segment.hpp>

#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/segment.hpp>

#include <boost/geometry.hpp>

namespace boost
{
  namespace geometry
  {
    namespace traits
    {
      template<typename Real> struct tag<eXl::Vector2<Real> > { typedef point_tag type; };
      template<typename Real> struct dimension<eXl::Vector2<Real> > : boost::mpl::int_<2> {};
      template<typename Real> struct coordinate_type<eXl::Vector2<Real> > { typedef Real type; };
      template<typename Real> struct coordinate_system<eXl::Vector2<Real> > { typedef cs::cartesian type ; };

      template<typename Real, std::size_t Dim> struct access<eXl::Vector2<Real>, Dim>
      {
          static inline Real get(eXl::Vector2<Real> const& p) { return p.m_Data[Dim]; }
          static inline void set(eXl::Vector2<Real>& p, Real const& value) { p.m_Data[Dim] = value; }
      };

      template<typename Real> struct tag<eXl::Vector3<Real> > { typedef point_tag type; };
      template<typename Real> struct dimension<eXl::Vector3<Real> > : boost::mpl::int_<3> {};
      template<typename Real> struct coordinate_type<eXl::Vector3<Real> > { typedef Real type; };
      template<typename Real> struct coordinate_system<eXl::Vector3<Real> > { typedef cs::cartesian type ; };

      template<typename Real, std::size_t Dim> struct access<eXl::Vector3<Real>, Dim>
      {
        static inline Real get(eXl::Vector3<Real> const& p) { return p.m_Data[Dim]; }
        static inline void set(eXl::Vector3<Real>& p, Real const& value) { p.m_Data[Dim] = value; }
      };


      template<typename Real> struct tag<eXl::Segment<Real> > { typedef segment_tag type; };
      template<typename Real> struct point_type<eXl::Segment<Real> > { typedef eXl::Vector2<Real> type; };
      template <typename Real, std::size_t Dimension>
      struct indexed_access<eXl::Segment<Real>, 0, Dimension>
      {
        typedef eXl::Segment<Real> segment_type;
        typedef typename geometry::coordinate_type<segment_type>::type coordinate_type;

        static inline coordinate_type get(segment_type const& s)
        {
          return geometry::get<Dimension>(s.m_Ext1);
        }

        static inline void set(segment_type& s, coordinate_type const& value)
        {
          geometry::set<Dimension>(s.m_Ext1, value);
        }
      };


      template <typename Real, std::size_t Dimension>
      struct indexed_access<eXl::Segment<Real>, 1, Dimension>
      {
        typedef eXl::Segment<Real> segment_type;
        typedef typename geometry::coordinate_type<segment_type>::type coordinate_type;

        static inline coordinate_type get(segment_type const& s)
        {
          return geometry::get<Dimension>(s.m_Ext2);
        }

        static inline void set(segment_type& s, coordinate_type const& value)
        {
          geometry::set<Dimension>(s.m_Ext2, value);
        }
      };

      template<typename Real> struct tag<eXl::AABB2D<Real> > { typedef box_tag type; }; \
      template<typename Real> struct point_type<eXl::AABB2D<Real> > { typedef eXl::Vector2<Real> type; };

      template <typename Real, std::size_t Corner, std::size_t Dimension>
      struct indexed_access<eXl::AABB2D<Real>, Corner, Dimension>
      {
        typedef Real coordinate_type;

        static inline coordinate_type get(eXl::AABB2D<Real> const& b)
        {
          return b.m_Data[Corner].m_Data[Dimension];
        }

        static inline void set(eXl::AABB2D<Real>& b, coordinate_type const& value)
        {
          b.m_Data[Corner].m_Data[Dimension] = value;
        }
      };

      template<typename Real> struct tag<eXl::Vector<eXl::Vector2<Real> > > { typedef ring_tag type; };

      template <typename Real>
      struct tag<eXl::Polygon<Real> >
      {
          typedef polygon_tag type;
      };

      template <typename Real>
      struct ring_const_type<eXl::Polygon<Real> >
      {
          typedef typename eXl::Polygon<Real>::PtList const& type;
      };

      template <typename Real>
      struct ring_mutable_type<eXl::Polygon<Real> >
      {
          typedef typename eXl::Polygon<Real>::PtList& type;
      };

      template <typename Real>
      struct interior_const_type<eXl::Polygon<Real> >
      {
          typedef typename eXl::Polygon<Real>::PtLists const& type;
      };

      template <typename Real>
      struct interior_mutable_type<eXl::Polygon<Real> >
      {
          typedef typename eXl::Polygon<Real>::PtLists& type;
      };

      template <typename Real>
      struct exterior_ring<eXl::Polygon<Real> >
      {
        typedef eXl::Polygon<Real> polygon_type;
        typedef typename eXl::Polygon<Real>::PtList ring_type;

        static inline ring_type& get(polygon_type& p)
        {
          return p.Border();
        }

        static inline ring_type const& get(polygon_type const& p)
        {
          return p.Border();
        }
      };

      template <typename Real>
      struct interior_rings<eXl::Polygon<Real> >
      {
        typedef eXl::Polygon<Real> polygon_type;
        typedef typename eXl::Polygon<Real>::PtLists inner_container_type;

        static inline inner_container_type& get(polygon_type& p)
        {
          return p.Holes();
        }

        static inline inner_container_type const& get(polygon_type const& p)
        {
          return p.Holes();
        }
      };

      template <typename Real>
      struct tag<eXl::AABB2DPolygon<Real> >
      {
        typedef polygon_tag type;
      };

      template <typename Real>
      struct ring_const_type<eXl::AABB2DPolygon<Real> >
      {
        typedef typename eXl::AABB2DPolygon<Real>::PtList const& type;
      };

      template <typename Real>
      struct ring_mutable_type<eXl::AABB2DPolygon<Real> >
      {
        typedef typename eXl::AABB2DPolygon<Real>::PtList& type;
      };

      template <typename Real>
      struct interior_const_type<eXl::AABB2DPolygon<Real> >
      {
        typedef typename eXl::AABB2DPolygon<Real>::PtLists const& type;
      };

      template <typename Real>
      struct interior_mutable_type<eXl::AABB2DPolygon<Real> >
      {
        typedef typename eXl::AABB2DPolygon<Real>::PtLists& type;
      };

      template <typename Real>
      struct exterior_ring<eXl::AABB2DPolygon<Real> >
      {
        typedef eXl::AABB2DPolygon<Real> polygon_type;
        typedef typename eXl::AABB2DPolygon<Real>::PtList ring_type;

        static inline ring_type& get(polygon_type& p)
        {
          return p.Border();
        }

        static inline ring_type const& get(polygon_type const& p)
        {
          return p.Border();
        }
      };

      template <typename Real>
      struct interior_rings<eXl::AABB2DPolygon<Real> >
      {
        typedef eXl::AABB2DPolygon<Real> polygon_type;
        typedef typename eXl::AABB2DPolygon<Real>::PtLists inner_container_type;

        static inline inner_container_type& get(polygon_type& p)
        {
          return p.Holes();
        }

        static inline inner_container_type const& get(polygon_type const& p)
        {
          return p.Holes();
        }
      };
    }
  }
}
