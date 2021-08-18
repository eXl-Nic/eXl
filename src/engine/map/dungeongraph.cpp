/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/map/dungeongraph.hpp>


namespace eXl
{
  IMPLEMENT_RTTI(DungeonGraph::NodeProperties);
  IMPLEMENT_RTTI(DungeonGraph::EdgeProperties);

  //const DungeonGraph::ContentName DungeonGraph::s_Default("DefaultRoom");

  void DungeonGraph::NodeAdded(Graph& iGraph, GraphVtx iVtx)
  {
    uint32_t newId;
    if (!m_AvailableVtxIdx.empty())
    {
      newId = *m_AvailableVtxIdx.begin();
      m_AvailableVtxIdx.erase(m_AvailableVtxIdx.begin());
    }
    else
    {
      newId = m_MaxVtxId++;
    }
    boost::put(boost::vertex_index, m_Graph, iVtx, newId);
  }

  void DungeonGraph::EdgeAdded(Graph& iGraph, GraphEdge iEdge)
  {
    uint32_t newId;
    if (!m_AvailableEdgeIdx.empty())
    {
      newId = *m_AvailableEdgeIdx.begin();
      m_AvailableEdgeIdx.erase(m_AvailableEdgeIdx.begin());
    }
    else
    {
      newId = m_MaxEdgeId++;
    }
    boost::put(boost::edge_index, m_Graph, iEdge, newId);
  }


  void DungeonGraph::NodeRemoved(Graph const& iGraph, GraphVtx iVtx)
  {
    uint32_t freedId = boost::get(boost::vertex_index, iGraph, iVtx);
    m_AvailableVtxIdx.insert(freedId);

    while (!m_AvailableVtxIdx.empty() && *(std::prev(m_AvailableVtxIdx.end())) == m_MaxVtxId - 1)
    {
      --m_MaxVtxId;
      m_AvailableVtxIdx.erase(std::prev(m_AvailableVtxIdx.end()));
    }
  }

  void DungeonGraph::EdgeRemoved(Graph const& iGraph, GraphEdge iEdge)
  {
    uint32_t freedId = boost::get(boost::edge_index, iGraph, iEdge);
    m_AvailableEdgeIdx.insert(freedId);

    while (!m_AvailableEdgeIdx.empty() && *(std::prev(m_AvailableEdgeIdx.end())) == m_MaxEdgeId - 1)
    {
      --m_MaxEdgeId;
      m_AvailableEdgeIdx.erase(std::prev(m_AvailableEdgeIdx.end()));
    }
  }

  void DungeonGraph::DefragIds(Graph& iGraph)
  {
    for (auto freeIdx : m_AvailableVtxIdx)
    {
      for (auto vtx : VerticesIter(iGraph))
      {
        if(boost::get(boost::vertex_index, iGraph, vtx) == m_MaxVtxId - 1)
        {
          NodeIdxChanged(iGraph, vtx, m_MaxVtxId - 1, freeIdx);
          boost::put(boost::vertex_index, iGraph, vtx, freeIdx);
          --m_MaxVtxId;
        }
      }
    }
    m_AvailableVtxIdx.clear();

    for (auto freeIdx : m_AvailableEdgeIdx)
    {
      for (auto edge : EdgesIter(iGraph))
      {
        if (boost::get(boost::edge_index, iGraph, edge) == m_MaxEdgeId - 1)
        {
          EdgeIdxChanged(iGraph, edge, m_MaxEdgeId - 1, freeIdx);
          boost::put(boost::edge_index, iGraph, edge, freeIdx);
          --m_MaxEdgeId;
        }
      }
    }
    m_AvailableEdgeIdx.clear();
  }

  void DungeonGraph::NodeProperties::AddContent(ContentName iName, uint32_t iSize)
  {
    eXl_ASSERT_REPAIR_RET(!HasContent(iName), )
    {
      m_Contents.push_back({iName, iSize});
      m_Size += iSize;
    }
  }

  bool DungeonGraph::NodeProperties::HasContent(ContentName iName) const
  {
    return std::find_if(m_Contents.begin(), m_Contents.end(), [&iName](NodeContent const& iContent)
    {
      return iContent.m_Type == iName;
    }) != m_Contents.end();
  }

  DungeonGraph::NodeContent* DungeonGraph::NodeProperties::GetContent(ContentName iName)
  {
    auto foundContent = std::find_if(m_Contents.begin(), m_Contents.end(), [&iName](NodeContent const& iContent)
    {
      return iContent.m_Type == iName;
    });
    return foundContent != m_Contents.end() ? &(*foundContent) : nullptr;
  }

  DungeonGraph::NodeContent const* DungeonGraph::NodeProperties::GetContent(ContentName iType) const
  {
    return const_cast<NodeProperties*>(this)->GetContent(iType);
  }

  void DungeonGraph::NodeProperties::SetContentSize(ContentName iType, uint32_t iSize)
  {
    if(auto content = GetContent(iType))
    {
      m_Size -= content->m_Size;
      content->m_Size = iSize;
      m_Size += content->m_Size;
    }
  }

  void DungeonGraph::NodeProperties::RemoveContent(ContentName iType)
  {
    for(auto iter = m_Contents.begin(); iter != m_Contents.end(); ++iter)
    {
      if(iter->m_Type == iType)
      {
        m_Size -= iter->m_Size;
        m_Contents.erase(iter);
      }
    }
  }

  //void DungeonGraph::MakeSingleRoom(unsigned int iRoomSize)
  //{
  //  auto& throneRoom = AddNode();
  //  throneRoom.second.AddContent(s_Default, iRoomSize);
  //}

  void DungeonGraph::PrintGraph()
  {
    for (auto vtx : VerticesIter(m_Graph))
    {
      printf("Node %i : {\n", GetIndexMap()[vtx]);
      if (NodeProperties const* props = GetProperties(vtx))
      {
        for (auto content : *props)
        {
          printf("   %s", content.m_Type.get().c_str());
          printf(" : %i\n", content.m_Size);
        }
      }
      printf("}");

      for(auto edge : OutEdgesIter(m_Graph, vtx))
      {
        auto target = GetTarget(vtx, edge);

        printf("  -> %i\n", GetIndexMap()[target]);
      }

      printf("\n");
    }
  }
}