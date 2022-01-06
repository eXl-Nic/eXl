/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/map/dungeongraph_res.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/identity.hpp>

#include <math/math.hpp>

#include <gen/graphutils.hpp>

namespace eXl
{
  DungeonGraph::ContentName DungeonGraph_Res::Resource() { return ContentName("Resource"); }
  DungeonGraph::ContentName DungeonGraph_Res::ResourceRelay() { return ContentName("ResourceRelay"); }
  DungeonGraph::ContentName DungeonGraph_Res::ThroneRoom() { return ContentName("ThroneRoom"); }
  DungeonGraph::ContentName DungeonGraph_Res::TreasureRoom() { return ContentName("TreasureRoom"); }
  DungeonGraph::ContentName DungeonGraph_Res::Library() { return ContentName("Library"); }
  DungeonGraph::ContentName DungeonGraph_Res::RestRoom() { return ContentName("RestRoom"); }

  IMPLEMENT_RTTI(DungeonGraph_Res::EdgeProperties_Res);

  struct NodeIdentity
  {
    typedef DungeonGraph::Graph::vertex_descriptor result_type;
    result_type operator()(std::pair<DungeonGraph::GraphVtx, DungeonGraph::NodeProperties*> const& iEntry) const
    {
      return iEntry.first;
    }
  };

  struct NodeSize
  {
    typedef uint32_t result_type;
    result_type operator()(std::pair<DungeonGraph::GraphVtx, DungeonGraph::NodeProperties*> const& iEntry) const
    {
      return iEntry.second->GetSize();
    }
  };

  typedef boost::multi_index::multi_index_container<std::pair<DungeonGraph::GraphVtx, DungeonGraph::NodeProperties*>,
    boost::multi_index::indexed_by<
    boost::multi_index::hashed_unique<NodeIdentity>
    ,boost::multi_index::ordered_non_unique<NodeSize>
    >  
  > NodeSizeMap;


  void DungeonGraph_Res::NodeAdded(Graph& iGraph, GraphVtx iVtx)
  {
    DungeonGraph::NodeAdded(iGraph, iVtx);
    uint32_t nodeId = GetIndex(iVtx);
    m_Nodes.resize(Mathi::Max(nodeId + 1, m_MaxVtxId));
    m_Nodes[nodeId] = std::make_unique<NodeProperties>();
    m_Nodes[nodeId]->SetPhysical(true);
    boost::put(boost::vertex_name, iGraph, iVtx, m_Nodes[nodeId].get());
  }

  void DungeonGraph_Res::EdgeAdded(Graph& iGraph, GraphEdge iEdge)
  {
    DungeonGraph::EdgeAdded(iGraph, iEdge);
    uint32_t edgeId = GetIndex(iEdge);
    m_Edges.resize(Mathi::Max(edgeId + 1, m_MaxEdgeId));
    m_Edges[edgeId] = std::make_unique<EdgeProperties_Res>();
    boost::put(boost::edge_name, iGraph, iEdge, m_Edges[edgeId].get());
  }

  void DungeonGraph_Res::NodeRemoved(Graph const& iGraph, GraphVtx iVtx)
  {
    m_Nodes[GetIndex(iVtx)] = nullptr;
    DungeonGraph::NodeRemoved(iGraph, iVtx);
  }

  void DungeonGraph_Res::EdgeRemoved(Graph const& iGraph, GraphEdge iEdge)
  {
    m_Edges[GetIndex(iEdge)] = nullptr;
    DungeonGraph::EdgeRemoved(iGraph, iEdge);
  }

  void DungeonGraph_Res::NodeIdxChanged(Graph& iGraph, GraphVtx iVtx, uint32_t iFrom, uint32_t iTo)
  {
    std::swap(m_Nodes[iFrom], m_Nodes[iTo]);
  }

  void DungeonGraph_Res::EdgeIdxChanged(Graph& iGraph, GraphEdge iEdge, uint32_t iFrom, uint32_t iTo)
  {
    std::swap(m_Edges[iFrom], m_Edges[iTo]);
  }

  std::pair<DungeonGraph::GraphVtx, DungeonGraph::NodeProperties*> DungeonGraph_Res::AddNode()
  {
    auto newVtx = boost::add_vertex(m_Graph);
    NodeAdded(m_Graph, newVtx);

    return std::make_pair(newVtx, GetProperties(newVtx));
  }

  std::pair<DungeonGraph::GraphEdge, DungeonGraph_Res::EdgeProperties_Res*> DungeonGraph_Res::AddEdge(GraphVtx iNode1, GraphVtx iNode2)
  {
    auto newEdge = boost::add_edge(iNode1, iNode2, m_Graph).first;

    EdgeAdded(m_Graph, newEdge);
    
    return std::make_pair(newEdge, EdgeProperties_Res::DynamicCast(GetProperties(newEdge)));
  }

  void DungeonGraph_Res::MakeSingleRoom(unsigned int iRoomSize)
  {
    auto throneRoom = AddNode();
    throneRoom.second->AddContent(ThroneRoom(), iRoomSize);
  }

  void DungeonGraph_Res::InitRes()
  {
    eXl_ASSERT_REPAIR_BEGIN(m_Desc.m_MaxRelayDegree * m_Desc.m_MaxRelaySize >= m_Desc.m_Resources)
    {
      m_Desc.m_Resources = m_Desc.m_MaxRelayDegree * m_Desc.m_MaxRelaySize;
    }

    auto throneRoom = AddNode();
    throneRoom.second->AddContent(ThroneRoom(), m_Desc.m_Resources);

    auto resources = AddNode();
    resources.second->AddContent(Resource(), m_Desc.m_Resources);

    auto relay = AddNode();
    relay.second->AddContent(ResourceRelay(), m_Desc.m_Resources);

    //At least one relay between resources and throne.

    auto relayEdge = AddEdge(throneRoom, relay);
    relayEdge.second->m_ResourceFlow = m_Desc.m_Resources;
    relayEdge.second->m_ResourceSrc = relay.first;

    auto resourceEdge = AddEdge(relay, resources);
    resourceEdge.second->m_ResourceFlow = m_Desc.m_Resources;
    resourceEdge.second->m_ResourceSrc = resources.first;
  }

  UnorderedSet<DungeonGraph::Graph::vertex_descriptor> FitNodesTo(uint32_t iValue, NodeSizeMap const& iNodes, uint32_t iNodesTot)
  {
    typedef UnorderedSet<DungeonGraph::Graph::vertex_descriptor> ResultType;

    if(iValue == 0 || iNodes.empty() || iNodesTot < iValue)
    {
      return ResultType();
    }

    auto iter = iNodes.get<1>().lower_bound(iValue);
    if(iter == iNodes.get<1>().end())
    {
      if(!iNodes.empty())
      {
        iter = std::prev(iter);
      }
      else
      {
        return ResultType();
      }
    }
    else
    {
      if(iter->second->GetSize() == iValue)
      {
        ResultType res;
        res.insert(iter->first);
        return res;
      }

      if(iter != iNodes.get<1>().begin())
      {
        --iter;
      }
    }

    uint32_t candidateSize = iter->second->GetSize();
    uint32_t nodesTotAfterRem = iNodesTot - candidateSize;

    if((candidateSize < iValue)
    && (iNodes.size() > 1)
    && (nodesTotAfterRem >= (iValue - candidateSize)))
    {
      NodeSizeMap smallerMap = iNodes;
      smallerMap.get<0>().erase(iter->first);

      auto subRes1 = FitNodesTo(iValue, smallerMap, nodesTotAfterRem);
      auto subRes2 = FitNodesTo(iValue - candidateSize, smallerMap, nodesTotAfterRem);

      if(!subRes1.empty())
      {
        if(subRes2.empty() || subRes1.size() <= subRes2.size() )
        {
          return subRes1;
        }
      }
      else if(!subRes2.empty())
      {
        subRes2.insert(iter->first);
        return subRes2;
      }
    }

    return ResultType();
  }

  void DungeonGraph_Res::ExpandResources(Random& iRand)
  {
    Map<uint32_t, GraphVtx> vertexToInspect;

    auto addVertexToInspect = [this, &vertexToInspect](GraphVtx iVtx)
    {
      vertexToInspect.insert(std::make_pair(GetIndexMap()[iVtx], iVtx));
    };
    
    for(auto vtx : VerticesIter(m_Graph))
    {
      addVertexToInspect(vtx);
    }

    while(!vertexToInspect.empty())
    {
      auto vtx = vertexToInspect.begin()->second;
      vertexToInspect.erase(vertexToInspect.begin());

      std::function<bool(GraphVtx, bool)> splitFunction;
      splitFunction = [&](GraphVtx vtx, bool forceSplit)
      {
        NodeProperties* descPtr = GetProperties(vtx);
        eXl_ASSERT(descPtr != nullptr);
        NodeProperties& desc = *descPtr;

        auto rscNode = desc.GetContent(ResourceRelay());
        rscNode = rscNode ? rscNode : desc.GetContent(Resource());

        if(rscNode)
        {
          bool nodeTooBig = forceSplit ? (forceSplit && rscNode->m_Size > 1) : rscNode->m_Type == ResourceRelay() ? rscNode->m_Size > m_Desc.m_MaxRelaySize : rscNode->m_Size > m_Desc.m_MaxResourceSize;
          bool randomSplit = nodeTooBig ? 
            false : 
            rscNode->m_Size > 1 && float(iRand() % m_Desc.m_MaxRelaySize) < rscNode->m_Size * m_Desc.m_RandomRelaySplitChance;

          if(randomSplit)
          {
            // Check that someone down the road does not have a too big degree to no randomly undo the work.
            for(auto edge : OutEdgesIter(m_Graph, vtx))
            {
              auto& edgeDesc = m_Edges[GetIndex(edge)];
              auto target = GetTarget(vtx, edge);
              if(edgeDesc->m_ResourceSrc == vtx)
              {
                if(boost::degree(target, m_Graph) < m_Desc.m_MaxRelayDegree)
                {
                  randomSplit = false;
                }
              }
            }
          }

          if(nodeTooBig || randomSplit)
          {
            uint32_t split = (iRand() % (rscNode->m_Size - 1)) + 1;
            uint32_t splitRem = rscNode->m_Size - split;

            auto newRsc = AddNode();

            addVertexToInspect(newRsc.first);
            addVertexToInspect(vtx);

            newRsc.second->AddContent(rscNode->m_Type, splitRem);

            desc.SetContentSize(rscNode->m_Type, split);

            while(true)
            {
              NodeSizeMap sources;

              //Follow the chain to split the source.
              for(auto edge : OutEdgesIter(m_Graph, vtx))
              {
                auto& edgeDesc = m_Edges[GetIndex(edge)];
                if(edgeDesc->m_ResourceSrc == GetTarget(vtx, edge))
                {
                  uint32_t nodeIdx = GetIndex(edgeDesc->m_ResourceSrc);
                  sources.insert(std::make_pair(edgeDesc->m_ResourceSrc, m_Nodes[nodeIdx].get()));
                }
              }

              if(sources.empty())
              {
                //No need to fit, just split.

                // Adjust outgoing edges.
                List<Graph::vertex_descriptor> edgesToAdd;
                for(auto edge : OutEdgesIter(m_Graph, vtx))
                {
                  auto& edgeDesc = m_Edges[GetIndex(edge)];
                  auto target = GetTarget(edgeDesc->m_ResourceSrc, edge);
                  edgeDesc->m_ResourceFlow = split;

                  auto newEdge = AddEdge(target, newRsc.first);
                  newEdge.second->m_ResourceSrc = newRsc.first;
                  newEdge.second->m_ResourceFlow = splitRem;

                  addVertexToInspect(target);
                }

                return true;
              }
              else
              {
                auto resSplit = FitNodesTo(split, sources, rscNode->m_Size);
                if(resSplit.empty())
                {
                  resSplit = FitNodesTo(splitRem, sources, rscNode->m_Size);
                  if(!resSplit.empty())
                  {
                    std::swap(split, splitRem);
                  }
                }

                if(resSplit.empty())
                {
                  //Split bigger node.
                  auto vtxToSplit = sources.get<1>().rbegin()->first;
                  //for(auto iter = sources.begin(); iter != sources.end(); ++iter)
                  {
                    //if(iter->second.GetSize() > 1)
                    {
                      //auto vtxToSplit = iter->first;
                      splitFunction(vtxToSplit, true);
                    }
                  }
                }
                else
                {
                  List<Graph::edge_descriptor> edgesToRemove;

                  // Adjust incoming and outgoing edges.
                  for(auto edge : OutEdgesIter(m_Graph, vtx))
                  {
                    auto& edgeDesc = m_Edges[GetIndex(edge)];
                    auto target = GetTarget(vtx, edge);

                    // vertex is a source fot the target
                    if(edgeDesc->m_ResourceSrc == vtx)
                    {
                      edgeDesc->m_ResourceFlow = split;

                      auto newEdge = AddEdge(target, newRsc.first);
                      newEdge.second->m_ResourceSrc = newRsc.first;
                      newEdge.second->m_ResourceFlow = splitRem;
                    }
                    else
                    {
                      // target is a source.
                      if(resSplit.count(target) == 0)
                      {
                        edgesToRemove.push_back(edge);
                        //Source assigned to the other node
                        auto newEdge = AddEdge(target, newRsc.first);
                        newEdge.second->m_ResourceSrc = target;
                        newEdge.second->m_ResourceFlow = edgeDesc->m_ResourceFlow;
                      }
                    }

                    addVertexToInspect(target);
                  }

                  for(auto edgeRemoved : edgesToRemove)
                  {
                    EdgeRemoved(m_Graph, edgeRemoved);
                    boost::remove_edge(edgeRemoved, m_Graph);
                  }
                  DefragIds(m_Graph);

                  return true;
                }
              }
            }
          }
        }
        return false;
      };

      //Check for degree.
      if(boost::degree(vtx, m_Graph) > m_Desc.m_MaxRelayDegree)
      {
        Graph::edge_descriptor srcEdgeToRemove;
        Graph::vertex_descriptor toMove;
        uint32_t nodeToMoveSize = m_Desc.m_Resources;

        // Move one relay/resource to another source.
        for(auto edge : OutEdgesIter(m_Graph, vtx))
        {
          //Move smaller nodes in priority
          auto target = GetTarget(vtx, edge);
          auto const& edgeDesc = m_Edges[GetIndex(edge)];
          auto targetDesc = GetProperties(target);

          if(edgeDesc->m_ResourceSrc == target)
          {
            if(targetDesc->GetSize() < nodeToMoveSize)
            {
              srcEdgeToRemove = edge;
              toMove = target;
              nodeToMoveSize = targetDesc->GetSize();
            }
          }
        }

        Vector<Graph::vertex_descriptor> moveDests;

        auto getDests = [&](bool allowNonRelay)
        {
          for(auto edge : OutEdgesIter(m_Graph, vtx))
          {
            auto target = GetTarget(vtx, edge);
            if(target != toMove)
            {
              auto const& edgeDesc = m_Edges[GetIndex(edge)];
              auto targetDesc = GetProperties(target);

              if(edgeDesc->m_ResourceSrc == target)
              {
                if((allowNonRelay || targetDesc->HasContent(ResourceRelay()))
                && targetDesc->GetSize() + nodeToMoveSize <= m_Desc.m_MaxRelaySize)
                {
                  moveDests.push_back(target);
                }
              }
            }
          }
        };

        auto sortBySizeAndDegree = [&](GraphVtx const& iNode1, GraphVtx const& iNode2)
        {
          auto desc1 = GetProperties(iNode1);
          auto desc2 = GetProperties(iNode2);

          return desc1->GetSize() + boost::degree(iNode1, m_Graph) < desc2->GetSize() + boost::degree(iNode2, m_Graph);
        };

        boost::optional<Graph::vertex_descriptor> dest = boost::none;

        getDests(false);
        std::sort(moveDests.begin(), moveDests.end(), sortBySizeAndDegree);
        if(moveDests.empty())
        {
          getDests(true);
          std::sort(moveDests.begin(), moveDests.end(), sortBySizeAndDegree);
        }
        else
        {
          dest = [&]() -> boost::optional<Graph::vertex_descriptor>
          {
            for(auto dest : moveDests)
            {
              auto targetDesc = GetProperties(dest);
              if(targetDesc->HasContent(ResourceRelay())
                && (boost::degree(dest, m_Graph) + 1) < m_Desc.m_MaxRelayDegree)
              {
                return dest;
              }
            }
            return boost::none;
          }();
        }

        eXl_ASSERT(!moveDests.empty());

        if (!dest)
        {
          auto newNode = AddNode();
          addVertexToInspect(newNode.first);

          auto nodeToMoveAway = moveDests[0];
          auto nodeToMoveAwayDesc = GetProperties(nodeToMoveAway);
          auto rscNode = nodeToMoveAwayDesc->GetContent(ResourceRelay());
          rscNode = rscNode ? rscNode : nodeToMoveAwayDesc->GetContent(Resource());

          eXl_ASSERT(rscNode);

          newNode.second->AddContent(ResourceRelay(), rscNode->m_Size);

          auto destEdgeToRemove = boost::edge(vtx, moveDests[0], m_Graph).first;
          {
            auto const& oldEdgeDesc = m_Edges[GetIndex(destEdgeToRemove)];
              
            auto newEdge1 = AddEdge(vtx, newNode.first);
            newEdge1.second->m_ResourceFlow = oldEdgeDesc->m_ResourceFlow;
            newEdge1.second->m_ResourceSrc = newNode.first;

            auto newEdge2 = AddEdge(newNode.first, nodeToMoveAway);
            newEdge2.second->m_ResourceFlow = oldEdgeDesc->m_ResourceFlow;
            newEdge2.second->m_ResourceSrc = nodeToMoveAway;
          }
          EdgeRemoved(m_Graph, destEdgeToRemove);
          boost::remove_edge(destEdgeToRemove, m_Graph);

          dest = newNode.first;

          DefragIds(m_Graph);
        }


        eXl_ASSERT_REPAIR_RET(!(!dest),)
        {
          {
            auto const& oldEdgeDesc = m_Edges[GetIndex(srcEdgeToRemove)];

            auto newEdge = AddEdge(toMove, *dest);
            newEdge.second->m_ResourceFlow = oldEdgeDesc->m_ResourceFlow;
            newEdge.second->m_ResourceSrc = toMove;
          }
          EdgeRemoved(m_Graph, srcEdgeToRemove);
          boost::remove_edge(srcEdgeToRemove, m_Graph);

          DefragIds(m_Graph);

          auto destNodeDesc = GetProperties(*dest);
          auto relayPtr = destNodeDesc->GetContent(ResourceRelay());
          eXl_ASSERT(relayPtr);
          destNodeDesc->SetContentSize(ResourceRelay(), nodeToMoveSize + relayPtr->m_Size);

          addVertexToInspect(vtx);
          addVertexToInspect(*dest);


        }

      }

      //Check for resource/relay size
      splitFunction(vtx, false);

    }
  }

  void DungeonGraph_Res::ExpandRooms(Random& iRand)
  {

  }
}