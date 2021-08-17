
#include <gtest/gtest.h>

#include <gen/pregraph.hpp>

#include <math/mathtools.hpp>

#include <boost/graph/iteration_macros.hpp>

using namespace eXl;

TEST(DunAtk, GraphTest)
{
  {
    ES_RuleSystem sys;
    sys.StartRule().AddNode().AddNode().AddConnection(0, 1)
      .AddNewNode().AddNewConnection(1, 2).End();

    ASSERT_TRUE(!sys.IsRuleSymmetric(0));

    sys.StartRule().AddNode().AddNode().AddConnection(0, 1)/*.AddConnection(1, 0)*/
      .AddNewNode().AddNewConnection(1, 2)/*.AddNewConnection(2, 1)*/.AddNewConnection(0, 2)/*.AddNewConnection(2, 0)*/.End();

    ASSERT_TRUE(sys.IsRuleSymmetric(1));

    sys.StartRule().AddNode().AddNode().AddConnection(0, 1)/*.AddConnection(1, 0)*/
      .AddNewNode().AddNewConnection(1, 2)/*.AddNewConnection(2, 1)*/.End();

    ASSERT_TRUE(!sys.IsRuleSymmetric(2));

    sys.StartRule().AddNode(1).AddCutNode(1).AddNode(1)
      .AddCutConnection(0, 1)/*.AddCutConnection(1, 0)*/
      .AddCutConnection(1, 2)/*.AddCutConnection(2, 1)*/
      .AddNewConnection(0, 2, 0, 1)/*.AddNewConnection(2, 0, 1, 3)*/.End();

    ASSERT_TRUE(sys.IsRuleSymmetric(3));
  }

  uint32_t numNodes = 0;

  auto allocIndex = [&numNodes](ES_RuleSystem::RewriteCtx& iCtx, ES_RuleSystem::GraphVtx iNewVtx)
  {
    boost::put(boost::vertex_index, iCtx.finalGraph, iNewVtx, numNodes++);
  };

  ES_RuleSystem sys;
  sys.StartRule().AddNode().AddNode().AddNode()
    .AddCutConnection(0, 1).AddCutConnection(1, 2).AddCutConnection(2, 0)
    .AddNewNode(0, allocIndex).AddNewNode(0, allocIndex).AddNewNode(0, allocIndex)
    .AddNewConnection(3, 4).AddNewConnection(4, 5).AddNewConnection(5, 3)
    .AddNewConnection(0, 3, 0, -1).AddNewConnection(3, 1, -1, 0)
    .AddNewConnection(1, 4, 1, -1).AddNewConnection(4, 2, -1, 1)
    .AddNewConnection(2, 5, 2, -1).AddNewConnection(5, 0, -1, 2)
    .End();

  ASSERT_TRUE(sys.IsRuleSymmetric(0));

  ES_RuleSystem::Graph testGraph;

  auto vtx1 = boost::add_vertex(testGraph);
  auto vtx2 = boost::add_vertex(testGraph);
  auto vtx3 = boost::add_vertex(testGraph);

  boost::put(boost::vertex_index, testGraph, vtx1, numNodes++);
  boost::put(boost::vertex_index, testGraph, vtx2, numNodes++);
  boost::put(boost::vertex_index, testGraph, vtx3, numNodes++);

  boost::add_edge(vtx1, vtx2, testGraph);
  boost::add_edge(vtx2, vtx3, testGraph);
  boost::add_edge(vtx3, vtx1, testGraph);

  auto browseGraph = [](ES_RuleSystem::Graph const& testGraph)
  {
    auto idxMap = MakeIndexMap(testGraph);

    for (auto edge : EdgesIter(testGraph))
    {
      boost::get(idxMap, edge.m_source);
      boost::get(idxMap, edge.m_target);
    }

    for (auto vtx : VerticesIter(testGraph))
    {
      for (auto edge : OutEdgesIter(testGraph, vtx))
      {
        boost::get(idxMap, edge.m_source);
        boost::get(idxMap, edge.m_target);
      }

      for (auto edge : InEdgesIter(testGraph, vtx))
      {
        boost::get(idxMap, edge.m_source);
        boost::get(idxMap, edge.m_target);
      }

      BGL_FORALL_INEDGES_T(vtx, edge, testGraph, ES_RuleSystem::Graph)
      {
        boost::get(idxMap, edge.m_source);
        boost::get(idxMap, edge.m_target);
      }
    }
  };

  browseGraph(testGraph);
  //sys.Print(std::cout, testGraph, {}, {});

  ES_RuleSystem::Graph newGraph;
  sys.Apply(testGraph, newGraph);

  browseGraph(newGraph);
  //sys.Print(std::cout, newGraph, {}, {});

  ES_RuleSystem::Graph newGraph2;
  sys.Apply(newGraph, newGraph2);

  browseGraph(newGraph2);
  //sys.Print(std::cout, newGraph2, {}, {});
}