/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

namespace eXl
{

  template <typename Graph, typename T>
  class TGraphMap
  {
  public:
    typedef Map<typename Graph::vertex_descriptor const, T> GnrMap;
    typedef typename GnrMap::key_type key_type; 
    typedef typename GnrMap::value_type::second_type value_type;
    typedef value_type& reference;
    typedef boost::read_write_property_map_tag category;

    GnrMap m_Map;
  };

  template <typename Graph>
  using TIndexMap = TGraphMap<Graph, uint32_t>;

  template <typename Graph>
  TIndexMap<Graph> MakeIndexMap(Graph const& iGraph)
  {
    TGraphMap<Graph, uint32_t> indexMap;
    FillIndexMap(iGraph, indexMap);

    return indexMap;
  }
  
  template <typename Graph>
  inline void FillIndexMap(Graph const& iGraph, TIndexMap<Graph>& oMap)
  {
    unsigned int numInputVtx = 0;
    {
      for(auto vertices = boost::vertices(iGraph); vertices.first != vertices.second; ++vertices.first, ++numInputVtx)
      {
        auto idx = *vertices.first;
        oMap.m_Map.insert(std::make_pair(idx, numInputVtx));
      }
    }
  }

  template <typename Graph>
  class IndexMapRef
  {
  public:
    using IndexMap = TGraphMap<Graph, uint32_t>;
    typedef typename IndexMap::key_type key_type; 
    typedef typename IndexMap::value_type value_type;
    typedef typename IndexMap::reference reference;
    typedef typename IndexMap::category category;

    inline IndexMapRef(IndexMap& iMap) : m_Map(iMap){}
    inline IndexMapRef(IndexMapRef const& iMap) : m_Map(iMap.m_Map){}
    inline IndexMap& GetMap() {return m_Map;}
    inline IndexMap const& GetMap() const {return m_Map;}
  protected:
    IndexMap& m_Map;
  };

  //template<typename Graph>
  //std::vector<typename boost::graph_traits<Graph>::vertex_descriptor> 
  //  My_Vertex_Order_By_Mult(const Graph& graph, IndexMap<Graph> const& iMap) 
  //{
  //
  //  std::vector<typename boost::graph_traits<Graph>::vertex_descriptor> vertex_order;
  //  std::copy(vertices(graph).first, vertices(graph).second, std::back_inserter(vertex_order));
  //
  //  boost::detail::sort_vertices(graph, iMap, vertex_order);
  //  return vertex_order;
  //}

  template<typename Graph1, typename Graph2>
  struct MyPrintCallback 
  {
    
    MyPrintCallback(Graph1 const& iGraph1, const TGraphMap<Graph1, uint32_t>& iIndex1, Graph2 const& iGraph2, const TGraphMap<Graph2, uint32_t>& iIndex2)
      : m_Graph1(iGraph1), m_Index1(iIndex1), m_Graph2(iGraph2), m_Index2(iIndex2){}
    
    template <typename CorrespondenceMap1To2,
              typename CorrespondenceMap2To1>
    bool operator()(CorrespondenceMap1To2 f, CorrespondenceMap2To1) const 
    {  
      //m_Match.push_back(std::vector<unsigned int>(m_Graph1.m_Nodes.size()));
      for(auto vertices = boost::vertices(m_Graph1); vertices.first != vertices.second; ++vertices.first)
      {
        auto v = *vertices.first;
        //std::cout << '(' << boost::get(m_Index1, v) << ", " 
        //          << get(m_Index2, get(f, v)) << ") ";
		    eXl_ASSERT_MSG(false, "Unreachable");
      }
      //std::cout << std::endl;
      
      return true;
    }
    
  private:
    const Graph1& m_Graph1;
    const Graph2& m_Graph2;
    const TGraphMap<Graph1, uint32_t>& m_Index1;
    const TGraphMap<Graph2, uint32_t>& m_Index2;
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
    EdgesIter_Adaptor(Graph const& iGraph)
      : m_Range(boost::edges(iGraph))
    {}

    typename Graph::edge_iterator begin() { return m_Range.first; }
    typename Graph::edge_iterator end() { return m_Range.second; }

    std::pair<typename Graph::edge_iterator, typename Graph::edge_iterator> m_Range;
  };

  template <typename Graph>
  struct OutEdgesIter_Adaptor
  {
    OutEdgesIter_Adaptor(Graph const& iGraph, typename Graph::vertex_descriptor iNode)
      : m_Range(boost::out_edges(iNode, iGraph))
    {}

    typename Graph::out_edge_iterator begin() {return m_Range.first;}
    typename Graph::out_edge_iterator end() {return m_Range.second;}

    std::pair<typename Graph::out_edge_iterator, typename Graph::out_edge_iterator> m_Range;
  };

  template <typename Graph>
  struct InEdgesIter_Adaptor
  {
    InEdgesIter_Adaptor(Graph const& iGraph, typename Graph::vertex_descriptor iNode)
      : m_Range(boost::in_edges(iNode, iGraph))
    {}

    typename Graph::in_edge_iterator begin() { return m_Range.first; }
    typename Graph::in_edge_iterator end() { return m_Range.second; }

    std::pair<typename Graph::in_edge_iterator, typename Graph::in_edge_iterator> m_Range;
  };

  template <typename Graph>
  EdgesIter_Adaptor<Graph> EdgesIter(Graph const& iGraph) { return EdgesIter_Adaptor<Graph>(iGraph); }
  template <typename Graph>
  OutEdgesIter_Adaptor<Graph> OutEdgesIter(Graph const& iGraph, typename Graph::vertex_descriptor iNode) { return OutEdgesIter_Adaptor<Graph>(iGraph, iNode); }
  template <typename Graph>
  InEdgesIter_Adaptor<Graph> InEdgesIter(Graph const& iGraph, typename Graph::vertex_descriptor iNode) { return InEdgesIter_Adaptor<Graph>(iGraph, iNode); }

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


  template <typename Graph, typename T>
  inline typename eXl::TGraphMap<Graph, T>::value_type get(eXl::TGraphMap<Graph, T> const& i,
                                                       typename Graph::vertex_descriptor key)
  {
    return i.m_Map.find(key)->second;
  }

  template <typename Graph, typename T>
  inline void put(eXl::TGraphMap<Graph, T>& i,
                  typename eXl::TGraphMap<Graph, T>::key_type key,
                  typename eXl::TGraphMap<Graph, T>::value_type const& value)
  {
    auto iter = i.m_Map.find(key);
    if (iter != i.m_Map.end())
    {
      iter->second = value;
    }
    else
    {
      i.m_Map.insert(std::make_pair(key, value));
    }
  }

  template <typename Graph>
  inline typename eXl::IndexMapRef<Graph>::value_type get(eXl::IndexMapRef<Graph> const& i,
    typename Graph::vertex_descriptor key)
  {
    return i.GetMap().m_Map.find(key)->second;
  }

  template <typename Graph>
  inline void put(eXl::IndexMapRef<Graph>& i,
    typename eXl::IndexMapRef<Graph>::key_type key,
    typename eXl::IndexMapRef<Graph>::value_type const& value)
  {
    auto iter = i.GetMap().m_Map.find(key);
    if (iter != i.GetMap().m_Map.end())
    {
      iter->second = value;
    }
    else
    {
      i.GetMap().m_Map.insert(std::make_pair(key, value));
    }
  }
}
