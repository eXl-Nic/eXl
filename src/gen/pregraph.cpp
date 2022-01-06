/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <gen/pregraph.hpp>

#include <boost/graph/vf2_sub_graph_iso.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/bipartite.hpp>
#include <boost/graph/johnson_all_pairs_shortest.hpp>
#include <boost/property_map/function_property_map.hpp>

#include <math/mathtools.hpp>
#include <gen/graphutils.hpp>
//#define DEBUG_PRINT_PREGRAPH

namespace eXl
{
  IMPLEMENT_RTTI(ES_RuleSystem::NodeData);
  IMPLEMENT_RTTI(ES_RuleSystem::EdgeData);
  IMPLEMENT_RTTI(ES_RuleSystem::UserMatchContext);
  IMPLEMENT_RTTI(ES_RuleSystem::UserRewriteContext);

  ES_RuleSystem::Rule::Rule(Rule const& iOther)
    : matchNodes(iOther.matchNodes)
    , matchEdges(iOther.matchEdges)
    , newNodes(iOther.newNodes)
    , newEdges(iOther.newEdges)
    , isSymmetric(iOther.isSymmetric)
    , hasPotentialLoop(iOther.hasPotentialLoop)
    , hasDeletes(iOther.hasDeletes)
  {
    UnorderedMap<GraphVtx, GraphVtx> vtxMap;
    for (auto& vtx : VerticesIter(iOther.matchGraph))
    {
      auto desc = boost::add_vertex(matchGraph);
      vtxMap.insert(std::make_pair(vtx, desc));
    }

    for (auto const& pair : vtxMap)
    {
      boost::put(boost::vertex_index, matchGraph, pair.second, boost::get(boost::vertex_index, iOther.matchGraph, pair.first));
    }

    for (auto edge : EdgesIter(iOther.matchGraph))
    {
      auto edgeDesc = boost::add_edge(vtxMap[edge.m_source], vtxMap[edge.m_target], matchGraph).first;
      boost::put(boost::edge_index, matchGraph, edgeDesc, boost::get(boost::edge_index, iOther.matchGraph, edge));
    }
  }

  ES_RuleSystem::Rule& ES_RuleSystem::Rule::operator =(Rule const& iOther)
  {
    //Rule newRule(iOther);
    //*this = std::move(newRule);
    this->~Rule();
    new(this) Rule(iOther);
    return *this;
  }

  ES_RuleSystem::Rule::Rule(Rule&& iRule)
  {
    this->~Rule();
    new(this) Rule(iRule);
  }

  ES_RuleSystem::Rule& ES_RuleSystem::Rule::operator=(Rule&& iRule)
  {
    this->~Rule();
    new(this) Rule(iRule);
    return *this;
  }

  bool ES_RuleSystem::Edge::IsEquivalent(Edge const& iOther) const
  {
    return tag == iOther.tag
      && ((node[0] == iOther.node[0] && node[1] == iOther.node[1])
        || (node[0] == iOther.node[1] && node[1] == iOther.node[0]));
  }

  ES_RuleSystem::RuleBuilder& ES_RuleSystem::RuleBuilder::AddNode(uint32_t iTag, NodeCheckCallback iCb)
  {
    NodeMeta node;
    node.type = NodeType::Env;
    node.checkCb = std::move(iCb);
    node.tag = iTag;
    m_Nodes.emplace_back(std::move(node));
    ++m_NumMatchNodes;
    return *this;
  }

  ES_RuleSystem::RuleBuilder& ES_RuleSystem::RuleBuilder::AddCutNode(uint32_t iTag, NodeCheckCallback iCb, NodeRemoveCallback iRemoveCb)
  {
    NodeMeta node;
    node.type = NodeType::Cut;
    node.checkCb = std::move(iCb);
    node.removeCb = std::move(iRemoveCb);
    node.tag = iTag;
    m_Nodes.emplace_back(std::move(node));
    ++m_NumMatchNodes;
    return *this;
  }

  ES_RuleSystem::RuleBuilder& ES_RuleSystem::RuleBuilder::AddNewNode(uint32_t iTag, NodeCreateCallback iCb)
  {
    NodeMeta node;
    node.type = NodeType::New;
    node.createCb = std::move(iCb);
    node.tag = iTag;
    m_Nodes.emplace_back(std::move(node));
    return *this;
  }

  ES_RuleSystem::RuleBuilder& ES_RuleSystem::RuleBuilder::AddConnection(uint32_t iNode1, uint32_t iNode2, uint32_t iTag, EdgeCheckCallback iCb)
  {
    eXl_ASSERT_REPAIR_RET(iNode1 < m_Nodes.size(), *this);
    eXl_ASSERT_REPAIR_RET(iNode2 < m_Nodes.size(), *this);
    eXl_ASSERT_REPAIR_RET(iNode1 != iNode2, *this);

    auto const& node1 = m_Nodes[iNode1];
    auto const& node2 = m_Nodes[iNode2];

    eXl_ASSERT_REPAIR_RET(node1.type == NodeType::Env && node2.type == NodeType::Env, *this);

    Edge edge;
    edge.type = EdgeType::Env;
    edge.node[0] = iNode1;
    edge.node[1] = iNode2;
    edge.checkCb = std::move(iCb);
    edge.tag = iTag;

    m_Edges.emplace_back(std::move(edge));
    ++m_NumMatchEdges;

    return *this;
  }

  ES_RuleSystem::RuleBuilder& ES_RuleSystem::RuleBuilder::AddCutConnection(uint32_t iNode1, uint32_t iNode2, uint32_t iTag, EdgeCheckCallback iCb, EdgeRemoveCallback iRemoveCb)
  {
    eXl_ASSERT_REPAIR_RET(iNode1 < m_Nodes.size(), *this);
    eXl_ASSERT_REPAIR_RET(iNode2 < m_Nodes.size(), *this);
    eXl_ASSERT_REPAIR_RET(iNode1 != iNode2, *this);

    auto const& node1 = m_Nodes[iNode1];
    auto const& node2 = m_Nodes[iNode2];

    eXl_ASSERT_REPAIR_RET(node1.type != NodeType::New && node2.type != NodeType::New, *this);

    Edge edge;
    edge.type = EdgeType::Cut;
    edge.node[0] = iNode1;
    edge.node[1] = iNode2;
    edge.checkCb = std::move(iCb);
    edge.removeCb = std::move(iRemoveCb);
    edge.tag = iTag;

    m_Edges.emplace_back(std::move(edge));
    ++m_NumMatchEdges;

    return *this;
  }

  ES_RuleSystem::RuleBuilder& ES_RuleSystem::RuleBuilder::AddNewConnection(uint32_t iNode1, uint32_t iNode2
    , uint32_t iPort1, uint32_t iPort2, uint32_t iTag, EdgeCreateCallback iCb)
  {
    eXl_ASSERT_REPAIR_RET(iNode1 < m_Nodes.size(), *this);
    eXl_ASSERT_REPAIR_RET(iNode2 < m_Nodes.size(), *this);
    eXl_ASSERT_REPAIR_RET(iNode1 != iNode2, *this);

    uint32_t ports[] = { iPort1, iPort2 };
    uint32_t nodeId[] = { iNode1, iNode2 };
    NodeMeta nodes[] = { m_Nodes[iNode1], m_Nodes[iNode2] };
    for (uint32_t i = 0; i < 2; ++i)
    {
      eXl_ASSERT_REPAIR_RET(nodes[i].type != NodeType::Cut, *this);

      if (ports[i] != -1)
      {
        eXl_ASSERT_REPAIR_RET(nodes[i].type != NodeType::New, *this);
        eXl_ASSERT_REPAIR_RET(ports[i] < m_Edges.size() && m_Edges[ports[i]].type == EdgeType::Cut, *this);
        eXl_ASSERT_REPAIR_RET(m_Edges[ports[i]].node[0] == nodeId[i] || m_Edges[ports[i]].node[1] == nodeId[i], *this);
      }
    }

    Edge edge;
    edge.type = EdgeType::New;
    edge.node[0] = iNode1;
    edge.node[1] = iNode2;
    edge.port[0] = iPort1;
    edge.port[1] = iPort2;
    edge.createCb = std::move(iCb);
    edge.tag = iTag;

    m_Edges.emplace_back(std::move(edge));
    return *this;
  }

  int ES_RuleSystem::RuleBuilder::End(MatchCheckCallback iCb)
  {
    int numRule = -1;
    eXl_ASSERT_REPAIR_RET(!m_Nodes.empty(), numRule);

    numRule = m_System.m_Rules.size();
    m_System.m_Rules.push_back(static_cast<Rule const&>(Rule()));
    Rule& curRule = m_System.m_Rules.back();
    curRule.finalCb = std::move(iCb);
    UnorderedMap<uint32_t, Vector<uint32_t>> permutationSetsL;
    UnorderedMap<uint32_t, Vector<uint32_t>> permutationSetsR;
    {
      Vector<GraphVtx> matchVtx;
      Vector<uint32_t> vtxMapping;
      for (auto& node : m_Nodes)
      {
        uint32_t vtxId;
        if (node.type != NodeType::New)
        {
          vtxId = curRule.matchNodes.size();

          curRule.matchNodes.emplace_back(std::move(node));
          auto desc = boost::add_vertex(curRule.matchGraph);
          matchVtx.push_back(desc);

          vtxMapping.push_back(vtxId);

          curRule.hasDeletes |= node.type == NodeType::Cut;
        }
        else
        {
          vtxId = curRule.newNodes.size() + m_NumMatchNodes;
          curRule.newNodes.emplace_back(std::move(node));
          vtxMapping.push_back(vtxId);
        }

        if (node.type == NodeType::Env || node.type == NodeType::Cut)
        {
          auto entry = permutationSetsL.insert(std::make_pair(node.tag, Vector<uint32_t>())).first;
          entry->second.push_back(vtxId);
        }

        if (node.type == NodeType::Env)
        {
          auto entry = permutationSetsR.insert(std::make_pair(node.tag, Vector<uint32_t>())).first;
          entry->second.push_back(vtxId);
        }
      }
      for (uint32_t i = 0; i < matchVtx.size(); ++i)
      {
        boost::put(boost::vertex_index, curRule.matchGraph, matchVtx[i], i);
      }

      uint32_t edgeIdx = 0;
      for (auto& edge : m_Edges)
      {
        edge.node[0] = vtxMapping[edge.node[0]];
        edge.node[1] = vtxMapping[edge.node[1]];

        if (edge.type != EdgeType::New)
        {
          auto edgeDesc = boost::add_edge(matchVtx[edge.node[0]], matchVtx[edge.node[1]], curRule.matchGraph).first;
          curRule.matchEdges.emplace_back(std::move(edge));
          boost::put(boost::edge_index, curRule.matchGraph, edgeDesc, edgeIdx);
          ++edgeIdx;
        }
        else
        {
          if (edge.port[0] != -1 && edge.port[1] != -1)
          {
            curRule.hasPotentialLoop = true;
          }
          curRule.newEdges.emplace_back(std::move(edge));
        }
      }

      auto generateValidPerm = [](UnorderedMap<uint32_t, Vector<uint32_t>> const& iPermSet)
      {
        UnorderedMap<uint32_t, UnorderedSet<uint32_t>> validPerm;
        for (auto const& set : iPermSet)
        {
          Vector<uint32_t> const& nodes = set.second;
          for (uint32_t i = 0; i < nodes.size(); ++i)
          {
            auto& set = validPerm.insert(std::make_pair(nodes[i], UnorderedSet<uint32_t>())).first->second;
            for (uint32_t j = 0; j < nodes.size(); ++j)
            {
              set.insert(nodes[j]);
            }
          }
        }
        return validPerm;
      };

      UnorderedMap<uint32_t, UnorderedSet<uint32_t>> validPermutationsL = generateValidPerm(permutationSetsL);
      UnorderedMap<uint32_t, UnorderedSet<uint32_t>> validPermutationsR = generateValidPerm(permutationSetsR);

      Vector<uint32_t> canonicalL;
      Vector<uint32_t> canonicalR;

      for (uint32_t i = 0; i < curRule.matchNodes.size(); ++i)
      {
        canonicalL.push_back(i);
        if (curRule.matchNodes[i].type == NodeType::Env)
        {
          canonicalR.push_back(i);
        }
      }

      Vector<uint32_t> currentPerm = canonicalR;
      Vector<Vector<uint32_t>> permutationsR;

      while (std::next_permutation(currentPerm.begin(), currentPerm.end()))
      {
        Vector<uint32_t> newNodesPerm;
        for (uint32_t i = 0; i < curRule.newNodes.size(); ++i)
        {
          newNodesPerm.push_back(i + m_NumMatchNodes);
        }

        bool validPerm;
        do
        {
          validPerm = true;
          Vector<uint32_t> remap = canonicalL;
          for (uint32_t i = 0; i < currentPerm.size(); ++i)
          {
            remap[canonicalR[i]] = currentPerm[i];
            if (validPermutationsR[canonicalR[i]].count(currentPerm[i]) == 0)
            {
              validPerm = false;
              break;
            }
          }

          if (!validPerm)
          {
            continue;
          }

          for (uint32_t i = 0; i < m_Edges.size(); ++i)
          {
            auto const& edge = m_Edges[i];
            if (edge.type == EdgeType::Cut)
            {
              continue;
            }

            Edge edgeToLookFor = edge;
            edgeToLookFor.tag = edge.tag;
            if (edgeToLookFor.node[0] < m_NumMatchNodes)
            {
              edgeToLookFor.node[0] = remap[edgeToLookFor.node[0]];
            }
            else
            {
              edgeToLookFor.node[0] = newNodesPerm[edgeToLookFor.node[0] - m_NumMatchNodes];
            }
            if (edgeToLookFor.node[1] < m_NumMatchNodes)
            {
              edgeToLookFor.node[1] = remap[edgeToLookFor.node[1]];
            }
            else
            {
              edgeToLookFor.node[1] = newNodesPerm[edgeToLookFor.node[1] - m_NumMatchNodes];
            }
            auto iter = std::find_if(m_Edges.begin(), m_Edges.end(), [&edgeToLookFor](Edge const& iExisting)
            {return iExisting.IsEquivalent(edgeToLookFor); });
            if (iter == m_Edges.end())
            {
              validPerm = false;
              break;
            }
            else
            {
              eXl_ASSERT(iter->type == edge.type);
            }
          }
        } while (!validPerm && std::next_permutation(newNodesPerm.begin(), newNodesPerm.end()));

        if (validPerm)
        {
          permutationsR.push_back(currentPerm);
        }
      }

      curRule.isSymmetric = true;

      currentPerm = canonicalL;
      while (std::next_permutation(currentPerm.begin(), currentPerm.end()))
      {
        bool validPerm = true;
        for (uint32_t i = 0; i < currentPerm.size(); ++i)
        {
          if (validPermutationsL[i].count(currentPerm[i]) == 0)
          {
            validPerm = false;
            break;
          }
        }

        if (!validPerm)
        {
          continue;
        }

        for (uint32_t i = 0; i < m_Edges.size(); ++i)
        {
          auto const& edge = m_Edges[i];
          if (edge.type == EdgeType::New)
          {
            continue;
          }

          Edge edgeToLookFor = edge;
          edgeToLookFor.tag = edge.tag;
          edgeToLookFor.node[0] = currentPerm[edgeToLookFor.node[0]];
          edgeToLookFor.node[1] = currentPerm[edgeToLookFor.node[1]];

          auto iter = std::find_if(m_Edges.begin(), m_Edges.end(), [&edgeToLookFor](Edge const& iExisting)
          {return iExisting.type != EdgeType::New && iExisting.IsEquivalent(edgeToLookFor); });
          if (iter == m_Edges.end())
          {
            validPerm = false;
            break;
          }
          else
          {
            eXl_ASSERT(iter->type == edge.type);
          }
        }

        if (!validPerm)
        {
          continue;
        }

        validPerm = false;

        for (auto const& candidateR : permutationsR)
        {
          bool foundGoodCandidate = true;
          for (uint32_t i = 0; i < candidateR.size(); ++i)
          {
            if (candidateR[i] != currentPerm[canonicalR[i]])
            {
              foundGoodCandidate = false;
              break;
            }
          }
          if (foundGoodCandidate)
          {
            validPerm = true;
            break;
          }
        }

        if (!validPerm)
        {
          curRule.isSymmetric = false;
          break;
        }
      }
    }

    m_System.CheckConflicts(numRule);

    ES_RuleSystem& system = m_System;

    this->~RuleBuilder();
    new(this) RuleBuilder(system);

    return numRule;
  }

  ES_RuleSystem::ES_RuleSystem()
  {

  }

  void Print(std::ostream& oStream, ES_RuleSystem::Graph const& iGraph, TIndexMap<ES_RuleSystem::Graph> const& iIdxMap, ES_RuleSystem::NodePrintCallback iNode, ES_RuleSystem::EdgePrintCallback iEdge)
  {
    oStream << "BeginPrint\n";
    for (auto edge : EdgesIter(iGraph))
    {
      oStream << boost::get(iIdxMap, edge.m_source);
      if (iNode)
      {
        oStream << "(";
        oStream << iNode(edge.m_source);
        oStream << ")";
      }

      oStream << " <-> ";

      oStream << boost::get(iIdxMap, edge.m_target);
      if (iNode)
      {
        oStream << "(";
        oStream << iNode(edge.m_target);
        oStream << ")";
      }

      if (iEdge)
      {
        oStream << " -- ";
        oStream << iEdge(edge);
      }

      oStream << "\n";
    }
    oStream << "EndPrint\n";
  }

  bool ES_RuleSystem::IsRuleSymmetric(uint32_t iRuleId) const
  {
    eXl_ASSERT_REPAIR_RET(iRuleId < m_Rules.size(), false);
    return m_Rules[iRuleId].isSymmetric;
  }

  struct ES_RuleSystem::VertexEquivalence
  {
    VertexEquivalence(ES_RuleSystem::Graph const& iGraph, ES_RuleSystem::Rule const& iRule, MatchCtx& iCtx)
      : m_Graph(iGraph)
      , m_Rule(iRule)
      , m_MatchCtx(iCtx)
    {}

    typedef GraphVtx param;

    bool operator()(GraphVtx const& iRuleVtx, GraphVtx const& iVtx)
    {
      uint32_t vtxIdx = boost::get(boost::vertex_index, m_Rule.matchGraph, iRuleVtx);
      NodeMeta const& ruleNode = m_Rule.matchNodes[vtxIdx];
      return !ruleNode.checkCb || ruleNode.checkCb(m_MatchCtx, iVtx);
    }

    Graph const& m_Graph;
    Rule const& m_Rule;
    MatchCtx& m_MatchCtx;
  };

  struct ES_RuleSystem::EdgeEquivalence
  {
    EdgeEquivalence(ES_RuleSystem::Graph const& iGraph, ES_RuleSystem::Rule const& iRule, MatchCtx& iCtx)
      : m_Graph(iGraph)
      , m_Rule(iRule)
      , m_MatchCtx(iCtx)
    {}

    bool operator()(GraphEdge const& iRuleEdge, GraphEdge const& iEdge)
    {
      uint32_t edgeIdx = boost::get(boost::edge_index, m_Rule.matchGraph, iRuleEdge);
      Edge const& ruleEdge = m_Rule.matchEdges[edgeIdx];
      return !ruleEdge.checkCb || ruleEdge.checkCb(m_MatchCtx, iEdge);
    }

    Graph const& m_Graph;
    Rule const& m_Rule;
    MatchCtx& m_MatchCtx;
  };

  template <typename GraphT>
  struct ES_RuleSystem::MatchingCallback
  {
    MatchingCallback(GraphT const& iGraph, Vector<VertexMatching>& oMatch, bool iStopOnMatch)
      : m_Graph(iGraph)
      , m_Ctx(nullptr)
      , m_Rule(nullptr)
      , m_Match(oMatch)
      , m_StopOnMatch(iStopOnMatch) {}

    MatchingCallback(GraphT const& iGraph, Rule const& iRule, MatchCtx& iCtx, Vector<VertexMatching>& oMatch, bool iStopOnMatch)
      : m_Graph(iGraph)
      , m_Ctx(&iCtx)
      , m_Rule(&iRule)
      , m_Match(oMatch)
      , m_StopOnMatch(iStopOnMatch) {}

    template <typename CorrespondenceMap1To2,
      typename CorrespondenceMap2To1>
      bool operator()(CorrespondenceMap1To2 f, CorrespondenceMap2To1) const
    {
      m_Match.push_back(ES_RuleSystem::VertexMatching());
      auto& matching = m_Match.back();

      for (ES_RuleSystem::GraphVtx v : VerticesIter(m_Graph))
      {
        uint32_t index = boost::get(boost::vertex_index, m_Graph, v);
        if (index >= matching.size())
        {
          matching.resize(index + 1);
        }
        matching[index] = boost::get(f, v);
      }

      if (m_Rule && !(!m_Rule->finalCb) && !m_Rule->finalCb(*m_Ctx, m_Match.back()))
      {
        m_Match.pop_back();
        return true;
      }

      return !m_StopOnMatch;
    }

  private:
    GraphT const& m_Graph;
    MatchCtx* m_Ctx;
    Rule const* m_Rule;
    Vector<VertexMatching>& m_Match;
    bool m_StopOnMatch;
  };

  template <typename Graph, typename IndexMap>
  Vector<typename Graph::vertex_descriptor>
    Vertex_Order_By_Mult(const Graph& graph, IndexMap const& iMap)
  {
    Vector<typename Graph::vertex_descriptor> vertex_order;
    std::copy(vertices(graph).first, vertices(graph).second, std::back_inserter(vertex_order));

    boost::detail::sort_vertices(graph, iMap, vertex_order);
    return vertex_order;
  }

  Vector<ES_RuleSystem::VertexMatching> ES_RuleSystem::FindRuleMatch(uint32_t iRuleId, Graph const& iGraph, UserMatchContext* iCtx) const
  {
    eXl_ASSERT_REPAIR_RET(iRuleId < m_Rules.size(), Vector<VertexMatching>());

    Rule const& rule = m_Rules[iRuleId];
    MatchCtx ctx(iGraph, iCtx);

    return FindRuleMatch(rule, iGraph, ctx);
  }

  Vector<ES_RuleSystem::VertexMatching> ES_RuleSystem::FindRuleMatch(Rule const& iRule, Graph const& iGraph, MatchCtx& iCtx) const
  {
    Vector<VertexMatching> matches;

    Graph const& ruleGraph = iRule.matchGraph;
    MatchingCallback<Graph> callback(ruleGraph, iRule, iCtx, matches, false);

    boost::vf2_subgraph_iso(ruleGraph, iGraph, callback,
      Vertex_Order_By_Mult(ruleGraph, boost::get(boost::vertex_index, iRule.matchGraph)),
      boost::vertices_equivalent(VertexEquivalence(iGraph, iRule, iCtx))
      .edges_equivalent(EdgeEquivalence(iGraph, iRule, iCtx))
      /*.vertex_index1_map(boost::get(boost::vertex_index, iRule.matchGraph))
      .vertex_index2_map(boost::get(boost::vertex_index, iGraph))*/);

    if (iRule.isSymmetric)
    {
      Vector<VertexMatching> sortedMatches = matches;
      for (auto& match : sortedMatches)
      {
        std::sort(match.begin(), match.end());
      }

      auto compareMatches = [](VertexMatching const& iM1, VertexMatching const& iM2)
      {
        for (uint32_t i = 0; i < iM1.size(); ++i)
        {
          if (iM1[i] < iM2[i])
          {
            return true;
          }
          else if (iM1[i] > iM2[i])
          {
            return false;
          }
        }

        return false;
      };

      Vector<VertexMatching> uniqueMatches = sortedMatches;
      std::sort(uniqueMatches.begin(), uniqueMatches.end(), compareMatches);
      uint32_t uniqueEnd = std::unique(uniqueMatches.begin(), uniqueMatches.end()) - uniqueMatches.begin();

      Vector<VertexMatching> finalMatches;
      for (uint32_t i = 0; i<matches.size(); ++i)
      {
        auto const& testMatch = sortedMatches[i];
        auto curEnd = uniqueMatches.begin() + uniqueEnd;
        auto iter = std::lower_bound(uniqueMatches.begin(), curEnd, testMatch, compareMatches);
        if (iter != curEnd && *iter == testMatch)
        {
          finalMatches.push_back(std::move(matches[i])); 
          uniqueMatches.erase(iter);
          --uniqueEnd;
        }
      }

      return finalMatches;
    }

    return matches;
  }

  struct ES_RuleSystem::PGNode : NodeData
  {
    DECLARE_RTTI(PGNode, NodeData);
    GraphVtx vtx;

    bool operator ==(PGNode const& iOther) const
    {
      return vtx == iOther.vtx;
    }

    int32_t ruleIdx = -1;
    uint32_t idx;
    uint32_t matchIdx;

    static std::unique_ptr<PGNode> NewNode(uint32_t iRule, uint32_t iIdx, uint32_t iMatchIdx)
    {
      auto newNode = std::make_unique<PGNode>();
      newNode->ruleIdx = iRule;
      newNode->idx = iIdx;
      newNode->matchIdx = iMatchIdx;

      return newNode;
    }
  };

  struct ES_RuleSystem::PGPort : NodeData
  {
    DECLARE_RTTI(PGPort, NodeData);
    GraphVtx vtx;
    GraphVtx edge[2];

    bool operator ==(PGPort const& iOther) const
    {
      return vtx == iOther.vtx 
        && ((edge[0] == iOther.edge[0] && edge[1] == iOther.edge[1])
          || (edge[0] == iOther.edge[1] && edge[1] == iOther.edge[0]));
    }

    static PGPort const* NewPort()
    {
      static PGPort s_Dummy;
      return &s_Dummy;
    }
  };

  struct ES_RuleSystem::PGEdge : EdgeData
  {
    DECLARE_RTTI(PGEdge, NodeData);
    GraphVtx edge[2];

    bool operator ==(PGEdge const& iOther) const
    {
      return (edge[0] == iOther.edge[0] && edge[1] == iOther.edge[1])
          || (edge[0] == iOther.edge[1] && edge[1] == iOther.edge[0]);
    }

    uint32_t ruleIdx = -1;
    uint32_t idx;
    uint32_t matchIdx;

    static std::unique_ptr<PGEdge> NewEdge(uint32_t iRuleIdx, uint32_t iIdx, uint32_t iMatchIdx)
    {
      auto newEdge = std::make_unique<PGEdge>();
      newEdge->ruleIdx = iRuleIdx;
      newEdge->idx = iIdx;
      newEdge->matchIdx = iMatchIdx;

      return newEdge;
    }
  };

  IMPLEMENT_RTTI(ES_RuleSystem::PGNode);
  IMPLEMENT_RTTI(ES_RuleSystem::PGPort);
  IMPLEMENT_RTTI(ES_RuleSystem::PGEdge);

  std::size_t hash_value(ES_RuleSystem::PGNode const& iNode)
  {
    size_t res = 0;
    boost::hash_combine(res, iNode.vtx);

    return res;
  }

  std::size_t hash_value(ES_RuleSystem::PGEdge const& iEdge)
  {
    size_t res = 0;
    boost::hash_combine(res, ptrdiff_t(iEdge.edge[0]) ^ ptrdiff_t(iEdge.edge[1]));

    return res;
  }

  std::size_t hash_value(ES_RuleSystem::PGPort const& iPort)
  {
    size_t res = 0;
    boost::hash_combine(res, iPort.vtx);
    boost::hash_combine(res, ptrdiff_t(iPort.edge[0]) ^ ptrdiff_t(iPort.edge[1]));

    return res;
  }

  struct ES_RuleSystem::PreGraph
  {
    Graph preGraph;
    UnorderedMap<PGNode, GraphVtx> vtxMap;
    UnorderedMap<PGPort, GraphVtx> portMap;
    UnorderedMap<PGEdge, GraphEdge> edgeMap;

    Vector<std::unique_ptr<PGNode>> newNodes;
    Vector<std::unique_ptr<PGEdge>> newEdges;
  };

  struct ES_RuleSystem::OnlyPortFilter
  {
    OnlyPortFilter()
    {}

    OnlyPortFilter(Graph const& iGraph)
      : m_Graph(&iGraph)
    {
      for (auto vtx : VerticesIter(iGraph))
      {
        if ((*this)(vtx))
        {
          ++m_NumNodes;
        }
      }
      for (auto edge : EdgesIter(iGraph))
      {
        if ((*this)(edge))
        {
          ++m_NumEdges;
        }
      }
    }

    bool operator()(const GraphVtx& vtx) const
    {
      return PGPort::DynamicCast(boost::get(boost::vertex_name, *m_Graph, vtx)) != nullptr;
    }

    bool operator()(const GraphEdge& iEdge) const
    {
      return PGPort::DynamicCast(boost::get(boost::vertex_name, *m_Graph, iEdge.m_source)) != nullptr
        && PGPort::DynamicCast(boost::get(boost::vertex_name, *m_Graph, iEdge.m_target)) != nullptr;
    }

    Graph const* m_Graph;
    uint32_t m_NumNodes = 0;
    uint32_t m_NumEdges = 0;
  };



  void ES_RuleSystem::MakePreGraph(Graph const& iGraph, PreGraph& oPg) const
  {
    uint32_t numVtx = 0;
    for (auto& vtx : VerticesIter(iGraph))
    {
      GraphVtx newVtx = boost::add_vertex(oPg.preGraph);
      PGNode newNode;
      newNode.vtx = vtx;
      newNode.idx = boost::get(boost::vertex_index, iGraph, vtx);
      auto newIter = oPg.vtxMap.insert(std::make_pair(newNode, newVtx)).first;
      boost::put(boost::vertex_name, oPg.preGraph, newVtx, &newIter->first);
      boost::put(boost::vertex_index, oPg.preGraph, newVtx, boost::get(boost::vertex_index, iGraph, vtx));
    }

    for (auto edge : EdgesIter(iGraph))
    {
      GraphVtx ports[2] = { boost::add_vertex(oPg.preGraph), boost::add_vertex(oPg.preGraph) };

      PGPort newPort;
      newPort.edge[0] = edge.m_source;
      newPort.edge[1] = edge.m_target;
      newPort.vtx = edge.m_source;

      for (auto port : ports)
      {
        auto newIter = oPg.portMap.insert(std::make_pair(newPort, port)).first;
        boost::put(boost::vertex_name, oPg.preGraph, port, &newIter->first);
        newPort.vtx = edge.m_target;
      }

      PGNode lookup;
      lookup.vtx = edge.m_source;
      boost::add_edge(oPg.vtxMap.find(lookup)->second, ports[0], oPg.preGraph);

      lookup.vtx = edge.m_target;
      boost::add_edge(oPg.vtxMap.find(lookup)->second, ports[1], oPg.preGraph);

      GraphEdge replEdge = boost::add_edge(ports[0], ports[1], oPg.preGraph).first;

      PGEdge edgeKey;
      edgeKey.edge[0] = edge.m_source;
      edgeKey.edge[1] = edge.m_target;
      auto newIterEdge = oPg.edgeMap.insert(std::make_pair(edgeKey, replEdge)).first;
      boost::put(boost::edge_name, oPg.preGraph, replEdge, &newIterEdge->first);
      boost::put(boost::edge_index, oPg.preGraph, replEdge, boost::get(boost::edge_index, iGraph, edge));
    }
  }

  void ES_RuleSystem::ApplyRule(Graph const& iGraph, Rule const& iRule, VertexMatching const& iMatch, uint32_t iMatchIdx, PreGraph& oPg, UserRewriteContext* iCtx) const
  {
    uint32_t ruleIdx = std::find_if(m_Rules.begin(), m_Rules.end(),
      [&](Rule const& iSeek) {return &iSeek == &iRule; }) - m_Rules.begin();

    for (auto const& edge : iRule.matchEdges)
    {
      if (edge.type == EdgeType::Cut)
      {
        auto edgeSeek = boost::edge(iMatch[edge.node[0]], iMatch[edge.node[1]], iGraph);
        eXl_ASSERT(edgeSeek.second);
        GraphEdge desc = edgeSeek.first;
        
        PGEdge edgeData;
        edgeData.edge[0] = desc.m_source;
        edgeData.edge[1] = desc.m_target;
        auto iter = oPg.edgeMap.find(edgeData);
        if (iter != oPg.edgeMap.end())
        {
          if (edge.removeCb)
          {
            Graph dummy;
            RewriteCtx rCtx(iGraph, dummy, iMatch, static_cast<uint32_t>(iter->first.ruleIdx), iCtx);
            edge.removeCb(rCtx, desc);
          }
          boost::remove_edge(iter->second, oPg.preGraph);
          oPg.edgeMap.erase(iter);
        }
        else
        {
          // Could have been deleted by another rule, could check.
        }
      }
    }

    uint32_t vtxIdx = 0;
    for (; vtxIdx < iRule.matchNodes.size(); ++vtxIdx)
    {
      NodeMeta const& node = iRule.matchNodes[vtxIdx];
      if (node.type == NodeType::Cut)
      {
        GraphVtx desc = iMatch[vtxIdx];
        PGNode nodeData;
        nodeData.vtx = desc;
        auto iter = oPg.vtxMap.find(nodeData);
        if (iter != oPg.vtxMap.end())
        {
          Vector<GraphVtx> ports;
          for (auto edge : OutEdgesIter(oPg.preGraph, nodeData.vtx))
          {
            ports.push_back(edge.m_target);
          }
          for (auto portVtx : ports)
          {
            boost::clear_vertex(portVtx, oPg.preGraph);
            boost::remove_vertex(portVtx, oPg.preGraph);
          }

          if (node.removeCb)
          {
            Graph dummy;
            RewriteCtx rCtx(iGraph, dummy, iMatch, static_cast<uint32_t>(iter->first.ruleIdx), iCtx);
            node.removeCb(rCtx, desc);
          }

          boost::clear_vertex(iter->second, oPg.preGraph);
          boost::remove_vertex(iter->second, oPg.preGraph);
        }
        else
        {
          // Could have been deleted by another rule, could check.
        }
      }
    }

    Vector<GraphVtx> newNodes;
    for (auto newNode : iRule.newNodes)
    {
      GraphVtx newVtx = boost::add_vertex(oPg.preGraph);
      newNodes.push_back(newVtx);
      oPg.newNodes.emplace_back(PGNode::NewNode(ruleIdx, newNodes.size() - 1, iMatchIdx));
      boost::put(boost::vertex_name, oPg.preGraph, newVtx, oPg.newNodes.back().get());
    }

    uint32_t newEdgeIdx = 0;
    for (auto newEdgeSpec : iRule.newEdges)
    {
      GraphVtx nodes[2];
      GraphVtx ports[2];
      for (uint32_t i = 0; i < 2; ++i)
      {
        uint32_t ruleIdx = newEdgeSpec.node[i];
        if(ruleIdx >= iRule.matchNodes.size())
        {
          nodes[i] = newNodes[ruleIdx - iRule.matchNodes.size()];
        }
        else
        {
          PGNode seekNode;
          seekNode.vtx = iMatch[ruleIdx];
          auto iter = oPg.vtxMap.find(seekNode);
          eXl_ASSERT(iter != oPg.vtxMap.end());
          nodes[i] = iter->second;
        }

        if (newEdgeSpec.port[i] == -1)
        {
          ports[i] = boost::add_vertex(oPg.preGraph);
          boost::put(boost::vertex_name, oPg.preGraph, ports[i], PGPort::NewPort());
          boost::add_edge(nodes[i], ports[i], oPg.preGraph);
        }
        else
        {
          Edge const& edgeSpec = iRule.matchEdges[newEdgeSpec.port[i]];
          PGPort port;
          port.vtx = iMatch[ruleIdx];
          port.edge[0] = iMatch[edgeSpec.node[0]];
          port.edge[1] = iMatch[edgeSpec.node[1]];

          auto iter = oPg.portMap.find(port);
          eXl_ASSERT(iter != oPg.portMap.end());
          ports[i] = iter->second;
        }
      }

      GraphEdge newEdge = boost::add_edge(ports[0], ports[1], oPg.preGraph).first;
      oPg.newEdges.emplace_back(PGEdge::NewEdge(ruleIdx, newEdgeIdx, iMatchIdx));
      boost::put(boost::edge_name, oPg.preGraph, newEdge, oPg.newEdges.back().get());

      ++newEdgeIdx;
    }
  }

  using OnlyPortGraph = boost::filtered_graph<ES_RuleSystem::Graph, ES_RuleSystem::OnlyPortFilter, ES_RuleSystem::OnlyPortFilter>;
}

namespace boost
{
  template <>
  eXl::OnlyPortGraph::vertices_size_type num_vertices(const eXl::OnlyPortGraph& g)
  {
    return g.m_vertex_pred.m_NumNodes;
  }

  template <>
  eXl::OnlyPortGraph::vertices_size_type num_edges(const eXl::OnlyPortGraph& g)
  {
    return g.m_vertex_pred.m_NumEdges;
  }
}

namespace eXl
{
  bool ES_RuleSystem::ComputeFinalGraph(Graph const& iGraph, Graph& oFinalGraph, PreGraph& iPg, Vector<Vector<VertexMatching>> const& iMatches, UserRewriteContext* iCtx) const
  {
    Vector<Vector<VertexMatching>> empty;
    PrintPreGraph(std::cout, iGraph, iPg, empty);

    // Merge equivalent ports.
    {
      OnlyPortFilter portFilter(iPg.preGraph);

      OnlyPortGraph portGraph(iPg.preGraph, portFilter, portFilter);
      auto portIndexMap = MakeIndexMap(portGraph);
      Vector<GraphVtx> allPorts(portIndexMap.m_Map.size());
      for (auto entry : portIndexMap.m_Map)
      {
        allPorts[entry.second] = entry.first;
      }

      // Check no odd loops (no need if no potential of loops)
      if (m_HasToCheckOddCycles)
      {
        Vector<GraphVtx> cycle;
        boost::find_odd_cycle(portGraph, portIndexMap, std::back_inserter(cycle));
        if (!cycle.empty())
        {
          return false;
        }
      }

      auto dummyWhFunc = [](GraphEdge const&) {return 1; };

      Vector<Vector<int32_t>> distanceMatrix(portFilter.m_NumNodes, Vector<int32_t>(portFilter.m_NumNodes, -1.0));

      boost::johnson_all_pairs_shortest_paths(portGraph, distanceMatrix,
        boost::vertex_index_map(portIndexMap)
        .weight_map(boost::make_function_property_map<GraphEdge>(dummyWhFunc)));

      UnorderedMap<GraphVtx, uint32_t> vtxAssignment;
      Vector<Vector<GraphVtx>> equivalenceClasses;
      for (auto vtx : VerticesIter(portGraph))
      {
        if (vtxAssignment.count(vtx) != 0)
        {
          continue;
        }
        Vector<GraphVtx> curClass;
        curClass.push_back(vtx);
        vtxAssignment.insert(std::make_pair(vtx, equivalenceClasses.size()));

        uint32_t vtxIdx = boost::get(portIndexMap, vtx);

        Vector<int32_t> const& allDist = distanceMatrix[vtxIdx];
        for (uint32_t i = 0; i < allDist.size(); ++i)
        {
          if (allDist[i] > 0)
          {
            if (allDist[i] % 2 == 0)
            {
              curClass.push_back(allPorts[i]);
              vtxAssignment.insert(std::make_pair(allPorts[i], equivalenceClasses.size()));
            }
          }
        }
        equivalenceClasses.emplace_back(std::move(curClass));
      }

      for (auto& toMerge : equivalenceClasses)
      {
        if (toMerge.size() > 1)
        {
          GraphVtx representativeVtx = toMerge.back();
          toMerge.pop_back();

          UnorderedSet<GraphVtx> cliqueNodes;
          Vector<GraphEdge> edgeToRemove;
          for (auto vtx : toMerge)
          {
            for (auto edge : OutEdgesIter(iPg.preGraph, vtx))
            {
              eXl_ASSERT(edge.m_target != vtx);
              cliqueNodes.insert(edge.m_target);
              edgeToRemove.push_back(edge);
            }
          }

          for (auto edge : edgeToRemove)
          {
            boost::remove_edge(edge, iPg.preGraph);
          }

          for (auto vtx : toMerge)
          {
            boost::remove_vertex(vtx, iPg.preGraph);
          }

          for (auto node : cliqueNodes)
          {
            if (!boost::edge(representativeVtx, node, iPg.preGraph).second)
            {
              boost::add_edge(representativeVtx, node, iPg.preGraph);
            }
          }
        }
      }
    }

    PrintPreGraph(std::cout, iGraph, iPg, empty);

    // Merge equivalent nodes
    {
      UnorderedMap<GraphVtx, uint32_t> vtxAssignment;
      Vector<UnorderedSet<GraphVtx>> equivalenceClasses;
      for (auto portVtx : VerticesIter(iPg.preGraph))
      {
        PGPort const* port = PGPort::DynamicCast(boost::get(boost::vertex_name, iPg.preGraph, portVtx));
        if (port != nullptr)
        {
          UnorderedSet<uint32_t> foundAssignments;
          UnorderedSet<GraphVtx> classVtx;
          for (auto edge : OutEdgesIter(iPg.preGraph, portVtx))
          {
            eXl_ASSERT(edge.m_target != portVtx);

            if (PGPort::DynamicCast(boost::get(boost::vertex_name, iPg.preGraph, edge.m_target)) != nullptr)
            {
              continue;
            }

            auto foundClass = vtxAssignment.find(edge.m_target);
            if (foundClass != vtxAssignment.end())
            {
              foundAssignments.insert(foundClass->second);
            }
            classVtx.insert(edge.m_target);
          }

          if (foundAssignments.empty())
          {
            uint32_t newAssignment = equivalenceClasses.size();
            for (auto vtx : classVtx)
            {
              vtxAssignment.insert(std::make_pair(vtx, newAssignment));
            }
            equivalenceClasses.emplace_back(std::move(classVtx));
          }
          else
          {
            auto allAssignments = foundAssignments.begin();
            uint32_t keptAssignment = *allAssignments;
            ++allAssignments;
            UnorderedSet<GraphVtx>& finalClass = equivalenceClasses[keptAssignment];
            for (auto vtx : classVtx)
            {
              vtxAssignment.insert(std::make_pair(vtx, keptAssignment));
              finalClass.insert(vtx);
            }
            for (; allAssignments != foundAssignments.end(); ++allAssignments)
            {
              UnorderedSet<GraphVtx>& mergedClass = equivalenceClasses[*allAssignments];
              for (auto vtx : mergedClass)
              {
                vtxAssignment[vtx] = keptAssignment;
                finalClass.insert(vtx);
              }
              mergedClass.clear();
            }
          }
        }
      }

      for (auto& toMerge : equivalenceClasses)
      {
        if (toMerge.size() > 1)
        {
          GraphVtx representativeVtx = *toMerge.begin();
          toMerge.erase(toMerge.begin());

          UnorderedSet<GraphVtx> cliqueNodes;
          Vector<GraphEdge> edgeToRemove;
          for (auto vtx : toMerge)
          {
            for (auto edge : OutEdgesIter(iPg.preGraph, vtx))
            {
              eXl_ASSERT(edge.m_target != vtx);
              cliqueNodes.insert(edge.m_target);
              edgeToRemove.push_back(edge);
            }
          }

          for (auto edge : edgeToRemove)
          {
            boost::remove_edge(edge, iPg.preGraph);
          }

          for (auto vtx : toMerge)
          {
            boost::remove_vertex(vtx, iPg.preGraph);
          }

          for (auto node : cliqueNodes)
          {
            if (!boost::edge(representativeVtx, node, iPg.preGraph).second)
            {
              boost::add_edge(representativeVtx, node, iPg.preGraph);
            }
          }
        }
      }
    }

    PrintPreGraph(std::cout, iGraph, iPg, empty);

    // Compute final graph
    // Sanity check
    Vector<GraphVtx> danglingPorts;
    Map<GraphVtx, GraphVtx> portNodeAssociation;
    for (auto vtx : VerticesIter(iPg.preGraph))
    {
      PGPort const* port = PGPort::DynamicCast(boost::get(boost::vertex_name, iPg.preGraph, vtx));
      if (port == nullptr)
      {
        continue;
      }
      uint32_t numNodeConnected = 0;
      uint32_t numPortConnected = 0;
      GraphVtx node;
      for (auto edge : OutEdgesIter(iPg.preGraph, vtx))
      {
        NodeData const* data = boost::get(boost::vertex_name, iPg.preGraph, edge.m_target);
        if (PGPort::DynamicCast(data) != nullptr)
        {
          ++numPortConnected;
        }
        if (PGNode::DynamicCast(data) != nullptr)
        {
          node = edge.m_target;
          ++numNodeConnected;
        }
      }

      eXl_ASSERT(numPortConnected < 2);
      eXl_ASSERT(numNodeConnected == 1);

      if (numPortConnected == 0)
      {
        danglingPorts.push_back(vtx);
      }
      else
      {
        portNodeAssociation.insert(std::make_pair(vtx, node));
      }
    }

    for (auto port : danglingPorts)
    {
      boost::clear_vertex(port, iPg.preGraph);
      boost::remove_vertex(port, iPg.preGraph);
    }

    Map<GraphVtx, GraphVtx> pgToFinal;
    UnorderedMap<GraphVtx, PGNode const*> newNodes;
    UnorderedMap<GraphEdge, PGEdge const*> newEdges;
    for (auto vtx : VerticesIter(iPg.preGraph))
    {
      NodeData const* data = boost::get(boost::vertex_name, iPg.preGraph, vtx);
      if (PGPort::DynamicCast(data) != nullptr)
      {
        continue;
      }
      PGNode const* nodeData = PGNode::DynamicCast(data);
      auto newVtx = boost::add_vertex(oFinalGraph);
      if (nodeData->ruleIdx != -1)
      {
        newNodes.insert(std::make_pair(newVtx, nodeData));
      }
      else
      {
        NodeData const* origData = boost::get(boost::vertex_name, iPg.preGraph, nodeData->vtx);
        boost::put(boost::vertex_name, oFinalGraph, newVtx, origData);
        boost::put(boost::vertex_index, oFinalGraph, newVtx, boost::get(boost::vertex_index, iGraph, nodeData->vtx));
      }
      pgToFinal.insert(std::make_pair(vtx, newVtx));
    }

    for (auto edge : EdgesIter(iPg.preGraph))
    {
      EdgeData const* data = boost::get(boost::edge_name, iPg.preGraph, edge);
      PGEdge const* edgeData = PGEdge::DynamicCast(data);
      if (edgeData == nullptr)
      {
        continue;
      }

      GraphVtx node1 = pgToFinal[portNodeAssociation[edge.m_source]];
      GraphVtx node2 = pgToFinal[portNodeAssociation[edge.m_target]];
      
      auto newEdge = boost::add_edge(node1, node2, oFinalGraph).first;
      if (edgeData->ruleIdx != -1)
      {
        newEdges.insert(std::make_pair(newEdge, edgeData));
      }
      else
      {
        auto graphEdge = boost::edge(edgeData->edge[0], edgeData->edge[1], iPg.preGraph);
        eXl_ASSERT(graphEdge.second);
        EdgeData const* origData = boost::get(boost::edge_name, iPg.preGraph, graphEdge.first);
        boost::put(boost::edge_name, oFinalGraph, newEdge, origData);
        boost::put(boost::edge_index, oFinalGraph, newEdge, boost::get(boost::edge_index, iGraph, graphEdge.first));
      }
    }

    // Call create callback
    for (auto const& entry : newNodes)
    {
      PGNode const* newNode = entry.second;
      if (auto& createFun = m_Rules[newNode->ruleIdx].newNodes[newNode->idx].createCb)
      {
        RewriteCtx rCtx(iGraph, oFinalGraph, iMatches[newNode->ruleIdx][newNode->matchIdx], static_cast<uint32_t>(newNode->ruleIdx), iCtx);
        createFun(rCtx, entry.first);
      }
    }
    for (auto const& entry : newEdges)
    {
      PGEdge const* newEdge = entry.second;
      if (auto& createFun = m_Rules[newEdge->ruleIdx].newEdges[newEdge->idx].createCb)
      {
        RewriteCtx rCtx(iGraph, oFinalGraph, iMatches[newEdge->ruleIdx][newEdge->matchIdx], static_cast<uint32_t>(newEdge->ruleIdx), iCtx);
        createFun(rCtx, entry.first);
      }
    }
    return true;
  }

  void ES_RuleSystem::PrintPreGraph(std::ostream& oStream, Graph const& iGraph, PreGraph const& iPg, Vector<Vector<VertexMatching>> const& iMatches) const
  {
#ifndef DEBUG_PRINT_PREGRAPH
    return;
#endif

    TIndexMap<Graph> origGraphMap;
    TIndexMap<Graph> pgMap;
    for (auto vtx : iPg.vtxMap)
    {
      boost::put(origGraphMap, vtx.first.vtx, vtx.first.idx);
      boost::put(pgMap, vtx.second, vtx.first.idx);
    }
    uint32_t portIdx = iPg.vtxMap.size();
    for (auto port : iPg.portMap)
    {
      boost::put(pgMap, port.second, portIdx++);
    }
    
    for (auto vtx : VerticesIter(iPg.preGraph))
    {
      if (pgMap.m_Map.count(vtx) == 0)
      {
        boost::put(pgMap, vtx, portIdx++);
      }
    }

    auto printNodeCB = [&](GraphVtx iVtx)
    {
       NodeData const* data = boost::get(boost::vertex_name, iPg.preGraph, iVtx);
       if (auto port = PGPort::DynamicCast(data))
       {
         if (port->vtx != Graph::null_vertex())
         {
           uint32_t idx0 = boost::get(origGraphMap, port->edge[0]);
           uint32_t idx1 = boost::get(origGraphMap, port->edge[1]);

           String desc("Port for (");
           desc.append(StringUtil::FromInt(idx0));
           desc.append(", ");
           desc.append(StringUtil::FromInt(idx1));
           desc.append(")");

           return desc;
         }

         return String("New Port");
       }
       if (auto node = PGNode::DynamicCast(data))
       {
         if (node->ruleIdx == -1)
         {
           return String("Node");
         }
         else
         {
           
           String desc("New Node : R ");
           desc.append(StringUtil::FromInt(node->ruleIdx));
           desc.append(", N ");
           desc.append(StringUtil::FromInt(node->idx));

           return desc;
         }
       }

       return String("<NewPort?>");
    };

    auto printEdgeCB = [&](GraphEdge iEdge)
    {
      if (auto edgeData = PGEdge::DynamicCast(boost::get(boost::edge_name, iPg.preGraph, iEdge)))
      {
        if (edgeData->ruleIdx == -1)
        {
          return String("Edge");
        }
        else
        {
          String desc("New Edge : R ");
          desc.append(StringUtil::FromInt(edgeData->ruleIdx));
          desc.append(", E ");
          desc.append(StringUtil::FromInt(edgeData->idx));

          return desc;
        }
      }
      else
      {
        return String("Port Connection");
      }
    };

    Print(oStream, iPg.preGraph, pgMap, printNodeCB, printEdgeCB);

    if (!iMatches.empty())
    {
      uint32_t ruleNum = 0;
      for (auto const& matches : iMatches)
      {
        oStream << "Matches for rule " << ruleNum << " :\n";
        for (auto const& match : matches)
        {
          if (uint32_t const matchLen = match.size())
          {
            oStream << "{";
            for (uint32_t i = 0; i < matchLen - 1; ++i)
            {
              oStream << boost::get(origGraphMap, match[i]) << ", ";
            }
            oStream << boost::get(origGraphMap, match.back()) << " }\n";
          }
        }
        oStream << "\n";
        ++ruleNum;
      }
    }
  }

  bool ES_RuleSystem::ApplyRule(Graph const& iGraph, Graph& oGraph, uint32_t iRule, VertexMatching const& iMatching, UserRewriteContext* iCtx, bool debug) const
  {
    eXl_ASSERT_REPAIR_RET(iRule < m_Rules.size(), false);

    Rule const& rule = m_Rules[iRule];

    eXl_ASSERT_REPAIR_RET(iMatching.size() == rule.matchNodes.size(), false);

    PreGraph pg;
    MakePreGraph(iGraph, pg);

    Vector<Vector<VertexMatching>> matches(m_Rules.size());
    matches[iRule].push_back(iMatching);

    PrintPreGraph(std::cout, iGraph, pg, matches);

    ApplyRule(iGraph, rule, iMatching, 0, pg, iCtx);

    return ComputeFinalGraph(iGraph, oGraph, pg, matches, iCtx);
  }

  bool ES_RuleSystem::Apply(Graph const& iGraph, Graph& oGraph, UserMatchContext* iMCtx, UserRewriteContext* iRCtx, bool debug) const
  {
    Vector<Vector<VertexMatching>> matches;

    PreGraph pg;
    MakePreGraph(iGraph, pg);

    MatchCtx mCtx(iGraph, iMCtx);
    for (auto const& rule : m_Rules)
    {
      matches.push_back(FindRuleMatch(rule, iGraph, mCtx));
    }

    //PrintPreGraph(std::cout, iGraph, pg, matches);

    //Should try to detect conflicts, if any.
    for (uint32_t i = 0; i < m_Rules.size(); ++i)
    {
      Rule const& rule = m_Rules[i];
      for (uint32_t j = 0; j< matches[i].size(); ++j)
      {
        ApplyRule(iGraph, rule, matches[i][j], j, pg, iRCtx);
      }
    }

    return ComputeFinalGraph(iGraph, oGraph, pg, matches, iRCtx);
  }

  struct NodeFilter
  {
    NodeFilter()
    {}

    NodeFilter(ES_RuleSystem::Graph const& iGraph, UnorderedSet<ES_RuleSystem::GraphVtx> const& iNodes)
      : m_Nodes(&iNodes)
      , m_Graph(&iGraph)
    {
      for (auto const edge : EdgesIter(*m_Graph))
      {
        if ((*this)(edge))
        {
          ++num_edges;
        }
      }
    }

    bool operator()(const ES_RuleSystem::GraphVtx& vtx) const
    {
      return m_Nodes->count(vtx) > 0;
    }

    bool operator()(const ES_RuleSystem::GraphEdge& iEdge) const
    {
      return m_Nodes->count(iEdge.m_source) > 0
        && m_Nodes->count(iEdge.m_target) > 0;
    }

    UnorderedSet<ES_RuleSystem::GraphVtx> const* m_Nodes;
    ES_RuleSystem::Graph const* m_Graph;
    uint32_t num_edges = 0;
  };

  using FilteredGraph = boost::filtered_graph<ES_RuleSystem::Graph, NodeFilter, NodeFilter>;

  struct ES_RuleSystem::VertexEquivalence_Tag
  {
    VertexEquivalence_Tag(ES_RuleSystem::Rule const& iRule1, ES_RuleSystem::Rule const& iRule2)
      : m_Rule1(iRule1)
      , m_Rule2(iRule2)
    {}

    typedef GraphVtx param;

    bool operator()(GraphVtx const& iRule1Vtx, GraphVtx const& iRule2Vtx)
    {
      uint32_t vtx1Idx = boost::get(boost::vertex_index, m_Rule1.matchGraph, iRule1Vtx);
      uint32_t vtx2Idx = boost::get(boost::vertex_index, m_Rule2.matchGraph, iRule2Vtx);
      return m_Rule1.matchNodes[vtx1Idx].tag == m_Rule2.matchNodes[vtx2Idx].tag;
    }

    Rule const& m_Rule1;
    Rule const& m_Rule2;
  };

  struct ES_RuleSystem::EdgeEquivalence_Tag
  {
    EdgeEquivalence_Tag(ES_RuleSystem::Rule const& iRule1, ES_RuleSystem::Rule const& iRule2)
      : m_Rule1(iRule1)
      , m_Rule2(iRule2)
    {}

    bool operator()(GraphEdge const& iRule1Edge, GraphEdge const& iRule2Edge)
    {
      uint32_t edge1Idx = boost::get(boost::edge_index, m_Rule1.matchGraph, iRule1Edge);
      uint32_t edge2Idx = boost::get(boost::edge_index, m_Rule2.matchGraph, iRule2Edge);
      return m_Rule1.matchNodes[edge1Idx].tag == m_Rule2.matchNodes[edge2Idx].tag;
    }

    Rule const& m_Rule1;
    Rule const& m_Rule2;
  };
}

namespace boost
{
  template <>
  eXl::FilteredGraph::vertices_size_type num_vertices(const eXl::FilteredGraph& g)
  {
    return g.m_vertex_pred.m_Nodes->size();
  }
 
  template <>
  eXl::FilteredGraph::vertices_size_type num_edges(const eXl::FilteredGraph& g)
  {
    return g.m_vertex_pred.num_edges;
  }
}

namespace eXl
{

  void ES_RuleSystem::CheckConflicts(uint32_t iRule)
  {
    eXl_ASSERT_REPAIR_RET(iRule < m_Rules.size(), );

    Rule const& ruleToCheck = m_Rules[iRule];

    for (auto& edge : ruleToCheck.newEdges)
    {
      if (edge.port[0] != -1 && edge.port[1] != -1)
      {
        m_HasToCheckOddCycles = true;
      }
    }

    for (uint32_t ruleIdx = 0; ruleIdx < m_Rules.size(); ++ruleIdx)
    {
      auto const& otherRule = m_Rules[ruleIdx];
      if (!ruleToCheck.hasDeletes && !otherRule.hasDeletes)
      {
        continue;
      }

      Rule const* rule1 = &ruleToCheck;
      Rule const* rule2 = &otherRule;
      for(uint32_t iter = 0; iter < 2; ++iter, std::swap(rule1, rule2))
      {
        bool incompatible = false;
        Vector<GraphVtx> allVtx;
        for (GraphVtx vtx : VerticesIter(rule1->matchGraph))
        {
          allVtx.push_back(vtx);
        }

        for (uint32_t numKeptVtx = allVtx.size(); numKeptVtx > 0 && !incompatible; --numKeptVtx)
        {
          for (auto&& combination : TCombinationHelper<GraphVtx>(allVtx, numKeptVtx))
          {
            UnorderedSet<GraphVtx> allowedVtx;
            for (auto&& vtx : combination)
            {
              allowedVtx.insert(vtx);
            }

            NodeFilter filter(rule1->matchGraph, allowedVtx);
            FilteredGraph filtered(rule1->matchGraph, filter, filter);
            Vector<VertexMatching> matches;

            MatchingCallback<FilteredGraph> callback(filtered, matches, false);

            auto filteredIdx = MakeIndexMap(filtered);

            boost::vf2_subgraph_iso(filtered, rule2->matchGraph, callback,
              Vertex_Order_By_Mult(filtered, filteredIdx),
              boost::vertices_equivalent(VertexEquivalence_Tag(*rule1, *rule2))
              .edges_equivalent(EdgeEquivalence_Tag(*rule1, *rule2))
              .vertex_index1_map(filteredIdx));

            for (auto const& match : matches)
            {
              for (auto vtx : allowedVtx)
              {
                uint32_t idx = boost::get(boost::vertex_index, filtered, vtx);
                NodeMeta const& node1 = rule1->matchNodes[boost::get(boost::vertex_index, rule1->matchGraph, vtx)];
                NodeMeta const& node2 = rule2->matchNodes[boost::get(boost::vertex_index, rule2->matchGraph, match[idx])];
                if (node1.type == NodeType::Env && node2.type == NodeType::Cut)
                {
                  incompatible = true;
                  break;
                }
              }
            }

            if (incompatible)
            {
              break;
            }
          }
        }

        if (incompatible)
        {
          m_ConflictingRules.insert(std::make_pair(iRule, ruleIdx));
        }
      }
    }
  }
}
