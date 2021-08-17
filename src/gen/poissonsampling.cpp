#include <gen/poissonsampling.hpp>
#include <math/geometrytraits.hpp>
#include <math/vector2.hpp>
#include <math/aabb2d.hpp>

#include <core/random.hpp>
#include <core/heapobject.hpp>

#include <boost/random.hpp>
//#include <boost/pool/pool_alloc.hpp>
#include <boost/geometry/index/rtree.hpp>

typedef std::pair<eXl::Vector2d, unsigned int> value;
typedef boost::geometry::index::rtree< value, boost::geometry::index::rstar<16, 4> > PointIndex;

namespace eXl
{

  static const unsigned int k_BootstrapLimit = 30;
  static const unsigned int k_GeneratedPoints = 30;

  //typedef boost::fast_pool_allocator<Vector2d,
  //  boost::default_user_allocator_new_delete,
  //  boost::details::pool::default_mutex,
  //  64, 128> allocator;

  typedef std::list<Vector2d/*, allocator*/> PointsStack;

  struct PoissonDiskSampling_Impl : public HeapObject
  {
    PoissonDiskSampling_Impl(Polygoni const& iPoly, Random& iGen) : m_PrecisePoly(iPoly), m_Gen(iGen)
    {
      //m_Stack.reserve(1024);
    }

    void AddPoint(Vector2d const& iPoint, float iCovering)
    {

      for(unsigned int i = 0; i < m_PointsIndex.size(); ++i)
      {
        double checkRadius = (m_Covering[i] + iCovering);
        AABB2Dd box = AABB2Dd::FromCenterAndSize(iPoint, Vector2d::ONE * checkRadius * 2);
        PointIndex const& index = m_PointsIndex[i];
        std::vector<value> returned_values;
        index.query(boost::geometry::index::within(box), std::back_inserter(returned_values));
        for(auto val : returned_values)
        {
          if((iPoint - val.first).Length() < checkRadius)
          {
            m_PointsValid[i][val.second] = false;
          }
        }
      }
      
      m_Stack.push_back(iPoint);
    }

    PointsStack m_Stack;

    std::vector<PointIndex>             m_PointsIndex;
    std::vector<std::vector<bool> >     m_PointsValid;
    std::vector<std::vector<Vector2d> > m_Points;
    std::vector<float>                  m_Covering;

    Polygond              m_PrecisePoly;
    Random&               m_Gen;
  };

  PoissonDiskSampling::PoissonDiskSampling(Polygoni const& iPoly, Random& iGen)
    :m_Impl(eXl_NEW PoissonDiskSampling_Impl(iPoly, iGen))
  {
    
  }

  PoissonDiskSampling::~PoissonDiskSampling()
  {
    eXl_DELETE m_Impl;
  }

  void PoissonDiskSampling::Sample(float iRadius, float iCovering, unsigned int iMaxPts)
  {
    Vector<Polygond> shrinkedPoly;
    {
      m_Impl->m_PrecisePoly.Shrink(iCovering, shrinkedPoly);
      if(shrinkedPoly.empty())
      {
        return;
      }
    }

    PointIndex curIndex;
    std::vector<Vector2d> curPoints;

    unsigned int numPts = 0;
    RandomWrapper rand(&m_Impl->m_Gen);

    for(auto poly : shrinkedPoly)
    {
      boost::random::uniform_real_distribution<double> distribX(poly.GetAABB().m_Data[0].X(), poly.GetAABB().m_Data[1].X());
      boost::random::uniform_real_distribution<double> distribY(poly.GetAABB().m_Data[0].Y(), poly.GetAABB().m_Data[1].Y());

      for(unsigned int i = 0; i<k_BootstrapLimit; ++i)
      {
        Vector2d origPt(distribX(rand),distribY(rand));
        if(poly.ContainsPoint(origPt))
        {
          m_Impl->AddPoint(origPt, iCovering);          
          curIndex.insert(std::make_pair(origPt, numPts));
          curPoints.push_back(origPt);
          ++numPts;
          break;
        }
      }
      
      while(!m_Impl->m_Stack.empty()
      && (iMaxPts == 0 || numPts < iMaxPts))
      {
        boost::random::uniform_real_distribution<double> distribTheta(-Mathd::PI, Mathd::PI);
        Vector2d origPt = m_Impl->m_Stack.front();
        m_Impl->m_Stack.pop_front();

        for(unsigned int i = 0; i<k_GeneratedPoints; ++i)
        {
          double angle = distribTheta(rand);
          Vector2d point = origPt + Vector2d(Mathd::Cos(angle), Mathd::Sin(angle)) * iRadius * 2;
          if(poly.ContainsPoint(point))
          {
            AABB2Dd box = AABB2Dd::FromCenterAndSize(point, Vector2d(iRadius, iRadius) * 4);
            bool validPoint = true;
            std::vector<value> returned_values;
            curIndex.query(boost::geometry::index::within(box), std::back_inserter(returned_values));

            for(auto val : returned_values)
            {
              if((point - val.first).Length() < 2*iRadius)
              {
                validPoint = false;
                break;
              }
            }
            if(validPoint)
            {
              m_Impl->AddPoint(point, iCovering);
              curIndex.insert(std::make_pair(point, numPts));
              curPoints.push_back(point);
              ++numPts;
            }
          }
        }
      }
    }

    m_Impl->m_Covering.push_back(iCovering);
    m_Impl->m_Points.push_back(std::vector<Vector2d>());
    m_Impl->m_PointsValid.push_back(std::vector<bool>());
    m_Impl->m_PointsIndex.push_back(PointIndex());

    m_Impl->m_Points.back().swap(curPoints);
    m_Impl->m_PointsIndex.back().swap(curIndex);
    m_Impl->m_PointsValid.back().resize(numPts, true);
    
  }

  unsigned int PoissonDiskSampling::GetNumLayers() const
  {
    return m_Impl->m_PointsIndex.size();
  }

  void PoissonDiskSampling::GetLayer(unsigned int iLayer, Vector<Vector2d>& oPoints) const
  {
    oPoints.clear();
    if(iLayer < m_Impl->m_PointsIndex.size())
    {
      for(unsigned int i = 0; i<m_Impl->m_Points[iLayer].size(); ++i)
      {
        if(m_Impl->m_PointsValid[iLayer][i])
        {
          oPoints.push_back(m_Impl->m_Points[iLayer][i]);
        }
      }
    }
  }

}