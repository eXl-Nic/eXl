#pragma once

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>

#include <iostream>

namespace eXl
{

  template <typename Graph, typename T>
  class TGraphMap
  {
  public:
    typedef std::map<typename Graph::vertex_descriptor const, T> GnrMap;
    typedef typename GnrMap::key_type key_type; 
    typedef typename GnrMap::value_type::second_type value_type;
    typedef value_type& reference;
    typedef boost::read_write_property_map_tag category;

    GnrMap m_Map;
  };

  template <typename Graph>
  class IndexMap : public TGraphMap<Graph, unsigned int>
  {
    public:
    
    inline IndexMap(){}

    inline IndexMap(Graph const& iGraph)
    {
      Fill(iGraph);
    }

    inline void Fill(Graph const& iGraph)
    {
      unsigned int numInputVtx = 0;
      {
        for(auto vertices = boost::vertices(iGraph); vertices.first != vertices.second; ++vertices.first, ++numInputVtx)
        {
          auto idx = *vertices.first;
          this->m_Map.insert(std::make_pair(idx, numInputVtx));
        }
      }
    }
  };

  template <typename Graph>
  class IndexMapRef
  {
  public:

    typedef typename IndexMap<Graph>::key_type key_type; 
    typedef typename IndexMap<Graph>::value_type value_type;
    typedef typename IndexMap<Graph>::reference reference;
    typedef typename IndexMap<Graph>::category category;


    inline IndexMapRef(IndexMap<Graph>& iMap) : m_Map(iMap){}
    inline IndexMapRef(IndexMapRef const& iMap) : m_Map(iMap.m_Map){}
    inline IndexMap<Graph>& GetMap() {return m_Map;}
    inline IndexMap<Graph> const& GetMap() const {return m_Map;}
  protected:
    IndexMap<Graph>& m_Map;
  };

  template<typename Graph1, typename Graph2>
  struct MyPrintCallback 
  {
    
    MyPrintCallback(Graph1 const& iGraph1, const IndexMap<Graph1>& iIndex1, Graph2 const& iGraph2, const IndexMap<Graph2>& iIndex2) 
      : m_Graph1(iGraph1), m_Index1(iIndex1), m_Graph2(iGraph2), m_Index2(iIndex2){}
    
    template <typename CorrespondenceMap1To2,
              typename CorrespondenceMap2To1>
    bool operator()(CorrespondenceMap1To2 f, CorrespondenceMap2To1) const 
    {  
      //m_Match.push_back(std::vector<unsigned int>(m_Graph1.m_Nodes.size()));
      for(auto vertices = boost::vertices(m_Graph1); vertices.first != vertices.second; ++vertices.first)
      {
        auto v = *vertices.first;
        std::cout << '(' << boost::get(m_Index1, v) << ", " 
                  << get(m_Index2, get(f, v)) << ") ";
      }
      std::cout << std::endl;
      
      return true;
    }
    
  private:
    const Graph1& m_Graph1;
    const Graph2& m_Graph2;
    const IndexMap<Graph1>& m_Index1;
    const IndexMap<Graph2>& m_Index2;
  };

  template <typename Graph>
  struct VerticesIter_Adaptor
  {
    VerticesIter_Adaptor(Graph const& iGraph)
      : m_Range(boost::vertices(iGraph))
    {}

    typename Graph::vertex_iterator begin() {return m_Range.first;}
    typename Graph::vertex_iterator end() {return m_Range.second;}

    std::pair<typename Graph::vertex_iterator, typename Graph::vertex_iterator> m_Range;
  };

  template <typename Graph>
  VerticesIter_Adaptor<Graph> VerticesIter(Graph const& iGraph) { return VerticesIter_Adaptor<Graph>(iGraph); }

  template <typename Graph>
  struct EdgesIter_Adaptor
  {
    EdgesIter_Adaptor(Graph const& iGraph, typename Graph::vertex_descriptor iNode)
      : m_Range(boost::out_edges(iNode, iGraph))
    {}

    typename Graph::out_edge_iterator begin() {return m_Range.first;}
    typename Graph::out_edge_iterator end() {return m_Range.second;}

    std::pair<typename Graph::out_edge_iterator, typename Graph::out_edge_iterator> m_Range;
  };

  template <typename Graph>
  EdgesIter_Adaptor<Graph> EdgesIter(Graph const& iGraph, typename Graph::vertex_descriptor iNode) { return EdgesIter_Adaptor<Graph>(iGraph, iNode); }

  template <typename Vertex, typename Edge>
  Vertex GetTarget(Vertex iNode, Edge const& iEdge)
  {
    return iEdge.m_source == iNode ? iEdge.m_target : iEdge.m_source;
  }

}

namespace boost
{
  //template<typename Graph>
  //struct property_map<Graph, vertex_index_t > 
  //{
  //  typedef eXl::IndexMap<Graph> const const_type;
  //  typedef eXl::IndexMap<Graph> type ;
  //};


  template <typename Graph>
  inline typename eXl::IndexMap<Graph>::value_type get(eXl::IndexMap<Graph> const& i,
                                                       typename Graph::vertex_descriptor key)
  {
    return i.m_Map.find(key)->second;
  }

  template <typename Graph>
  inline void put(eXl::IndexMap<Graph>& i,
                  typename eXl::IndexMap<Graph>::key_type key,
                  typename eXl::IndexMap<Graph>::value_type const& value)
  {
    auto iter = i.m_Map.find(key);
    if(iter != i.m_Map.end())
      iter->second = value;
    else
      i.m_Map.insert(std::make_pair(key, value));
  }

  template <typename Graph>
  inline typename eXl::IndexMap<Graph>::value_type get(eXl::IndexMapRef<Graph> const& i,
    typename Graph::vertex_descriptor key)
  {
    return i.GetMap().m_Map.find(key)->second;
  }

  template <typename Graph>
  inline void put(eXl::IndexMapRef<Graph>& i,
    typename eXl::IndexMap<Graph>::key_type key,
    typename eXl::IndexMap<Graph>::value_type const& value)
  {
    auto iter = i.GetMap().m_Map.find(key);
    if(iter != i.GetMap().m_Map.end())
      iter->second = value;
    else
      i.GetMap().m_Map.insert(std::make_pair(key, value));
  }
}
