/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <algorithm>
#include <boost/polygon/detail/polygon_sort_adaptor.hpp> //gtlsort not found on linux otherwise
#include <boost/polygon/polygon.hpp>
#include <math/math.hpp>

namespace boost
{
  namespace polygon
  {

    template <typename Real>
    struct geometry_concept < eXl::Vector2<Real> > { typedef point_concept type; };


    //Then we specialize the gtl point traits for our point type
    template <typename Real>
    struct point_traits < eXl::Vector2<Real> > {
      typedef Real coordinate_type;

      static inline coordinate_type get(const eXl::Vector2<Real>& point,
        orientation_2d orient) {
        if (orient == HORIZONTAL)
          return point.X();
        return point.Y();
      }
    };

    template <typename Real>
    struct point_mutable_traits < eXl::Vector2<Real> > {
      typedef int coordinate_type;


      static inline void set(eXl::Vector2<Real>& point, orientation_2d orient, int value) {
        if (orient == HORIZONTAL)
          point.X() = value;
        else
          point.Y() = value;
      }
      static inline eXl::Vector2<Real> construct(int x_value, int y_value) {
        eXl::Vector2<Real> retval;
        retval.X() = x_value;
        retval.Y() = y_value;
        return retval;
      }
    };

    template <typename Real>
    struct geometry_concept < std::pair<Real, Real> > { typedef interval_concept type; };

    template <typename Real>
    struct interval_traits < std::pair<Real, Real> > {
      typedef Real coordinate_type;

      static inline coordinate_type get(const std::pair<Real, Real>& interval, direction_1d dir) {
        if (dir == LOW)
          return interval.first;
        else
          return interval.second;
      }
    };

    template <typename Real>
    struct interval_mutable_traits < std::pair<Real, Real> > {
      typedef int coordinate_type;

      static inline void set(std::pair<Real, Real>& interval, direction_1d dir,
        typename interval_traits<std::pair<Real, Real> >::coordinate_type value) {
        if (dir == LOW)
          interval.first = value;
        else
          interval.second = value;
      }
      static inline std::pair<Real, Real> construct(typename interval_traits<std::pair<Real, Real> >::coordinate_type low_value,
        typename interval_traits<std::pair<Real, Real> >::coordinate_type high_value) {
        return std::make_pair(low_value, high_value);
      }
    };

    template <typename Real>
    struct geometry_concept < eXl::AABB2D<Real> > { typedef rectangle_concept type; };


    template <typename Real>
    struct rectangle_traits < eXl::AABB2D<Real>, gtl_no > {};

    template <typename Real, typename enable>
    struct rectangle_traits < eXl::AABB2D<Real>, enable > {
      typedef int coordinate_type;
      typedef std::pair<Real, Real> interval_type;
      static inline interval_type get(const eXl::AABB2D<Real>& rectangle, orientation_2d orient) {
        if (orient == HORIZONTAL)
          return std::make_pair(rectangle.m_Data[0].X(), rectangle.m_Data[1].X());
        else
          return std::make_pair(rectangle.m_Data[0].Y(), rectangle.m_Data[1].Y());
      }
    };

    template <typename Real>
    struct rectangle_mutable_traits < eXl::AABB2D<Real> > {
      template <typename T2>
      static inline void set(eXl::AABB2D<Real>& rectangle, orientation_2d orient, const T2& interval) {
        if (orient == HORIZONTAL)
        {
          rectangle.m_Data[0].X() = low(interval);
          rectangle.m_Data[1].X() = high(interval);
        }
        else
        {
          rectangle.m_Data[0].Y() = low(interval);
          rectangle.m_Data[1].Y() = high(interval);
        }
      }
      template <typename T2, typename T3>
      static inline eXl::AABB2D<Real> construct(const T2& interval_horizontal, const T3& interval_vertical) {
        eXl::AABB2D<Real> ret;
        ret.m_Data[0].X() = low(interval_horizontal);
        ret.m_Data[1].X() = high(interval_horizontal);
        ret.m_Data[0].Y() = low(interval_vertical);
        ret.m_Data[1].Y() = high(interval_vertical);
        return ret;
      }
    };

  }
}

#include <boost/polygon/segment_utils.hpp>
#include <math/segment.hpp>

namespace boost
{
  namespace polygon
  {
    template <typename Real>
    struct geometry_concept < eXl::Segment<Real> > { typedef segment_concept type; };

    template <typename Real>
    struct segment_traits<eXl::Segment<Real>> 
    {
      typedef Real coordinate_type;
      typedef eXl::Vector2<Real> point_type;

      static inline point_type get(const eXl::Segment<Real>& segment, direction_1d dir) {
        return (dir == LOW ? segment.m_Ext1 : segment.m_Ext2);
      }
    };

    template <typename Real>
    struct segment_mutable_traits<eXl::Segment<Real>> 
    {
      typedef typename segment_traits<eXl::Segment<Real>>::coordinate_type coordinate_type;
      typedef typename segment_traits<eXl::Segment<Real>>::point_type point_type;

      template <typename Point>
      static inline void set(eXl::Segment<Real>& segment, direction_1d dir, const Point& point) 
      {
        (dir == LOW ? segment.m_Ext1 : segment.m_Ext2) = eXl::Vector2<Real>(point_traits<Point>::get(point, HORIZONTAL), point_traits<Point>::get(point, VERTICAL));
      }

      template <typename Point>
      static inline eXl::Segment<Real> construct(const Point& low, const Point& high) {
        eXl::Segment<Real> res;
        res.m_Ext1 = eXl::Vector2<Real>(point_traits<Point>::get(low, HORIZONTAL), point_traits<Point>::get(low, VERTICAL));
        res.m_Ext2 = eXl::Vector2<Real>(point_traits<Point>::get(high, HORIZONTAL), point_traits<Point>::get(high, VERTICAL));
        return res;
      }
    };

    template <typename Segment, typename SegmentIterator, typename Allocator>
    typename enable_if<
      typename gtl_and<
      typename gtl_if<
      typename is_segment_concept<
      typename geometry_concept<
      typename std::iterator_traits<SegmentIterator>::value_type
      >::type
      >::type
      >::type,
      typename gtl_if<
      typename is_segment_concept<
      typename geometry_concept<Segment>::type
      >::type
      >::type
      >::type,
      void
    >::type
      intersect_segments(
        std::vector<std::pair<std::size_t, Segment>, Allocator>& result,
        SegmentIterator first, SegmentIterator last) {
      typedef typename segment_traits<Segment>::coordinate_type Unit;
      typedef typename scanline_base<Unit>::Point Point;
      typedef typename scanline_base<Unit>::half_edge half_edge;
      typedef int segment_id;
      std::vector<std::pair<half_edge, segment_id> > half_edges;
      std::vector<std::pair<half_edge, segment_id> > half_edges_out;
      segment_id id_in = 0;
      half_edges.reserve(std::distance(first, last));
      for (; first != last; ++first) {
        Point l, h;
        assign(l, low(*first));
        assign(h, high(*first));
        half_edges.push_back(std::make_pair(half_edge(l, h), id_in++));
      }
      half_edges_out.reserve(half_edges.size());
      // Apparently no need to pre-sort data when calling validate_scan.
      if (half_edges.size() != 0) {
        line_intersection<Unit>::validate_scan(
          half_edges_out, half_edges.begin(), half_edges.end());
      }

      result.reserve(result.size() + half_edges_out.size());
      for (std::size_t i = 0; i < half_edges_out.size(); ++i) {
        std::size_t id = (std::size_t)(half_edges_out[i].second);
        Point l = half_edges_out[i].first.first;
        Point h = half_edges_out[i].first.second;
        result.push_back(std::make_pair(id, construct<Segment>(l, h)));
      }
    }
  }
}
