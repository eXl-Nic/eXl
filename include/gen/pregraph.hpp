/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <gen/gen_exp.hpp>

#include <core/containers.hpp>
#include <core/rtti.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>

//#include "graphutils.hpp"

namespace eXl
{
  
  class EXL_GEN_API ES_RuleSystem
  {
  public:

    struct EXL_GEN_API NodeData : public RttiObject
    {
      DECLARE_RTTI(NodeData, RttiObject);
    };

    struct EXL_GEN_API EdgeData : public RttiObject
    {
      DECLARE_RTTI(EdgeData, RttiObject);
    };

    using Graph = boost::adjacency_list<boost::listS, boost::listS, boost::undirectedS, 
      boost::property<boost::vertex_name_t, NodeData const*, boost::property<boost::vertex_index_t, uint32_t>>,
      boost::property<boost::edge_name_t, EdgeData const*, boost::property<boost::edge_index_t, uint32_t>>>;

    using GraphVtx = Graph::vertex_descriptor;
    using GraphEdge = Graph::edge_descriptor;

    using VertexMatching = Vector<GraphVtx>;

    struct EXL_GEN_API UserMatchContext : public RttiObject
    {
      DECLARE_RTTI(UserMatchContext, RttiObject);
    };

    struct EXL_GEN_API UserRewriteContext : public RttiObject
    {
      DECLARE_RTTI(UserRewriteContext, RttiObject);
    };

    struct MatchCtx
    {
      MatchCtx(Graph const& iGraph, UserMatchContext* iUserCtx)
        : graph(iGraph)
        , userCtx(iUserCtx)
      {}
      Graph const& graph;
      UserMatchContext* userCtx = nullptr;
    };

    struct RewriteCtx
    {
      RewriteCtx(Graph const& iGraph, Graph& iFinalGraph,
        VertexMatching const& iMatch, uint32_t iRuleIdx,
        UserRewriteContext* iUserCtx)
        : graph(iGraph)
        , finalGraph(iFinalGraph)
        , match(iMatch)
        , ruleIdx(iRuleIdx)
        , userCtx(iUserCtx)
      {}

      Graph const& graph;
      Graph& finalGraph;
      VertexMatching const& match;
      uint32_t ruleIdx;
      UserRewriteContext* userCtx = nullptr;
    };
    
    using NodePrintCallback = std::function<String(GraphVtx iVtx)>;
    using EdgePrintCallback = std::function<String(GraphEdge iVtx)>;
    using NodeCheckCallback = std::function<bool(MatchCtx&, GraphVtx iVtx)>;
    using EdgeCheckCallback = std::function<bool(MatchCtx&, GraphEdge iEdge)>;
    using MatchCheckCallback = std::function<bool(MatchCtx&, Vector<GraphVtx> const&)>;
    using NodeCreateCallback = std::function<void(RewriteCtx&, GraphVtx iVtx)>;
    using EdgeCreateCallback = std::function<void(RewriteCtx&, GraphEdge iEdge)>;
    using NodeRemoveCallback = std::function<void(RewriteCtx&, GraphVtx iVtx)>;
    using EdgeRemoveCallback = std::function<void(RewriteCtx&, GraphEdge iEdge)>;
    using NodeMergeCallback = std::function<void(RewriteCtx&, Vector<GraphVtx> const& iVtx)>;

  private:

    enum class NodeType
    {
      Env, Cut, New
    };

    enum class EdgeType
    {
      Env, Cut, New
    };

    struct NodeMeta
    {
      NodeType type;
      uint32_t tag;
      NodeCheckCallback checkCb;
      NodeCreateCallback createCb;
      NodeRemoveCallback removeCb;
    };

    Vector<NodeType> m_Nodes;

    struct EdgeMeta
    {
      EdgeType type;
      uint32_t tag;
      EdgeCheckCallback checkCb;
      EdgeCreateCallback createCb;
      EdgeRemoveCallback removeCb;
    };

    struct Edge : EdgeMeta
    {
      bool IsEquivalent(Edge const& iOther) const;
      uint32_t node[2];
      uint32_t port[2];      
    };

  public:

    class EXL_GEN_API RuleBuilder
    {
      friend class ES_RuleSystem;
    public:

      RuleBuilder& AddNode(uint32_t iTag = 0, NodeCheckCallback iCb = {});
      RuleBuilder& AddCutNode(uint32_t iTag = 0, NodeCheckCallback iCheckCb = {}, NodeRemoveCallback iRemoveCb = {});
      RuleBuilder& AddNewNode(uint32_t iTag = 0, NodeCreateCallback iCb = {});

      RuleBuilder& AddConnection(uint32_t iNode1, uint32_t iNode2, uint32_t iTag = 0, EdgeCheckCallback iCb = {});
      RuleBuilder& AddCutConnection(uint32_t iNode1, uint32_t iNode2, uint32_t iTag = 0, EdgeCheckCallback iCheckCb = {}, EdgeRemoveCallback iRemoveCb = {});
      // Port == -1 wil add a new port
      // Port != -1 will reuse connection port from cut connections
      RuleBuilder& AddNewConnection(uint32_t iNode1, uint32_t iNode2
        , uint32_t iPort1 = -1, uint32_t iPort2 = -1
        , uint32_t iTag = 0, EdgeCreateCallback iCb = {});

      int End(MatchCheckCallback iFinalCb = {});

    protected:
      inline RuleBuilder(ES_RuleSystem& iSys) : m_System(iSys){}
      ES_RuleSystem& m_System;

      Vector<NodeMeta> m_Nodes;
      Vector<Edge> m_Edges;
      uint32_t m_NumMatchNodes = 0;
      uint32_t m_NumMatchEdges = 0;
    };

    ES_RuleSystem();

    //void Print(std::ostream& oStream, Graph const& iGraph, TIndexMap<Graph> const& iIdxMap, NodePrintCallback, EdgePrintCallback) const;

    bool IsRuleSymmetric(uint32_t iRuleId) const;

    bool HasConflicts() const { return !m_ConflictingRules.empty(); }

    Vector<VertexMatching> FindRuleMatch(uint32_t iRule, Graph const& iGraph, UserMatchContext* iCtx = nullptr) const;

    bool ApplyRule(Graph const& iGraph, Graph& oGraph, uint32_t iRule, VertexMatching const& iMatching, UserRewriteContext* iCtx = nullptr, bool debug = false) const;

    bool Apply(Graph const& iGraph, Graph& oGraph, UserMatchContext* iMCtx = nullptr, UserRewriteContext* iRCtx = nullptr, bool debug = false) const;
    
    inline RuleBuilder StartRule(){return RuleBuilder(*this);};

    struct PGNode;
    struct PGPort;
    struct PGEdge;
    struct OnlyPortFilter;

  protected:

    struct PreGraph;

    struct Rule
    {
      Rule() = default;
      Rule(Rule&&);
      Rule& operator=(Rule&&);
      Rule(Rule const&);
      Rule& operator =(Rule const&);

      Graph matchGraph;
      MatchCheckCallback finalCb;

      Vector<NodeMeta> matchNodes;
      Vector<Edge> matchEdges;

      Vector<NodeMeta> newNodes;
      Vector<Edge> newEdges;

      bool isSymmetric = false; 
      bool hasPotentialLoop = false;
      //bool selfConflicting = false;
      bool hasDeletes = false;
    };

    Vector<VertexMatching> FindRuleMatch(Rule const& iRule, Graph const& iGraph, MatchCtx& iCtx) const;
    void MakePreGraph(Graph const& iGraph, PreGraph& oPg) const;
    void ApplyRule(Graph const& iGraph, Rule const& iRule, VertexMatching const& iMatch, uint32_t iMatchIdx, PreGraph& oPg, UserRewriteContext* iCtx) const;
    bool ComputeFinalGraph(Graph const& iGraph, Graph& oFinalGraph, PreGraph& oPg, Vector<Vector<VertexMatching>> const& iMatches, UserRewriteContext* iCtx) const;
    void PrintPreGraph(std::ostream& oStream, Graph const& iGraph, PreGraph const& iPg, Vector<Vector<VertexMatching>> const& iMatches) const;

    void CheckConflicts(uint32_t iRule);

    struct VertexEquivalence;
    struct EdgeEquivalence;
    struct VertexEquivalence_Tag;
    struct EdgeEquivalence_Tag;
    template <typename Graph>
    struct MatchingCallback;

    Vector<Rule> m_Rules;
    UnorderedSet<Pair<uint32_t, uint32_t>> m_ConflictingRules;
    bool m_HasToCheckOddCycles = false;
  };
}
