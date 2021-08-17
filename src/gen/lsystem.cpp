#include <gen/lsystem.hpp>
#include <gen/multigrid.hpp>
#include <gen/gridrule.hpp>

namespace eXl
{
  static String names [] = {"Node", "Atom"};

  LSystem::LSystem()
  {
    m_Grammar[0] = new LevelGrammar_Old(2, names);
    m_Grammar[1] = new LevelGrammar_Old(2, names);

    m_Grammar[0]->StartRule().AddNode(1, true)
      .EndMatch()
      .BeginRepl()
      .AddReplNode(0, 0).AddNode(1).AddNode(1)
      .AddConnection(2,0, MultiGrid::LeftDir).AddConnection(0,2, MultiGrid::RightDir)
      .AddConnection(0,1, MultiGrid::LeftDir).AddConnection(1,0, MultiGrid::RightDir)
      .EndRepl().End();

    m_Grammar[1]->StartRule().AddNode(1, true)
      .EndMatch()
      .BeginRepl()
      .AddReplNode(0, 0).AddNode(1).AddNode(1)
      .AddConnection(2,0, MultiGrid::DownDir).AddConnection(0,2, MultiGrid::UpDir)
      .AddConnection(0,1, MultiGrid::DownDir).AddConnection(1,0, MultiGrid::UpDir)
      .EndRepl().End();

    for(unsigned int i = 0; i<2; ++i)
    {
      for(unsigned int j = 0; j<2; ++j)
      {
        MultiGrid::Direction base = (MultiGrid::Direction)(2*j + i);
        m_Grammar[1 - j]->StartRule().AddNode(1, true).AddNode()
          .AddConnection(0,1, GridRule::LocalToWorld(MultiGrid::LeftDir, base)).AddConnection(1,0, GridRule::LocalToWorld(MultiGrid::RightDir, base))
          .EndMatch()
          .BeginRepl()
          .AddReplNode(0, 0).AddReplNode(1).AddNode(1).AddNode(1)/*.AddNode(0)*/
          .AddConnection(0,1, GridRule::LocalToWorld(MultiGrid::LeftDir, base)).AddConnection(1,0, GridRule::LocalToWorld(MultiGrid::RightDir, base))
          //.AddConnection(0,4, GridRule::LocalToWorld(MultiGrid::LeftDir, base)).AddConnection(4,0, GridRule::LocalToWorld(MultiGrid::RightDir, base))
          //.AddConnection(4,1, GridRule::LocalToWorld(MultiGrid::LeftDir, base)).AddConnection(1,4, GridRule::LocalToWorld(MultiGrid::RightDir, base))
          .AddConnection(0,2, GridRule::LocalToWorld(MultiGrid::UpDir, base))  .AddConnection(2,0, GridRule::LocalToWorld(MultiGrid::DownDir, base))
          .AddConnection(0,3, GridRule::LocalToWorld(MultiGrid::DownDir, base)).AddConnection(3,0, GridRule::LocalToWorld(MultiGrid::UpDir, base))
          .EndRepl()
          .End();
      }
    }

    
    for(unsigned int j = 0; j<2; ++j)
    {
      MultiGrid::Direction base = (MultiGrid::Direction)(2*j);
      m_Grammar[j]->StartRule().AddNode(0).AddNode(0)
        .AddConnection(0,1, GridRule::LocalToWorld(MultiGrid::LeftDir, base)).AddConnection(1,0, GridRule::LocalToWorld(MultiGrid::RightDir, base))
        .EndMatch()
        .BeginRepl()
        .AddReplNode(0).AddReplNode(1).AddNode(0)
        .AddConnection(0,2, GridRule::LocalToWorld(MultiGrid::LeftDir, base)).AddConnection(2,0, GridRule::LocalToWorld(MultiGrid::RightDir, base))
        .AddConnection(2,1, GridRule::LocalToWorld(MultiGrid::LeftDir, base)).AddConnection(1,2, GridRule::LocalToWorld(MultiGrid::RightDir, base))
        .EndRepl()
        .End();
    }

    for(unsigned int i = 0; i<0; ++i)
    {
      MultiGrid::Direction base = (MultiGrid::Direction)(i);
      m_Grammar[0]->StartRule().AddNode(-1, true).AddNode().AddNode().AddNode()
        .AddConnection(0,1, GridRule::LocalToWorld(MultiGrid::LeftDir, base)).AddConnection(1,0, GridRule::LocalToWorld(MultiGrid::RightDir, base))
        .AddConnection(0,2, GridRule::LocalToWorld(MultiGrid::RightDir, base)).AddConnection(2,0, GridRule::LocalToWorld(MultiGrid::LeftDir, base))
        .AddConnection(0,3, GridRule::LocalToWorld(MultiGrid::DownDir, base)).AddConnection(3,0, GridRule::LocalToWorld(MultiGrid::UpDir, base))
        .EndMatch()
        .BeginRepl()
        .AddReplNode(0).AddReplNode(1).AddReplNode(2).AddReplNode(3).AddNode(0).AddNode(0).AddNode(0)
        .AddConnection(0,4, GridRule::LocalToWorld(MultiGrid::LeftDir, base)).AddConnection(4,0, GridRule::LocalToWorld(MultiGrid::RightDir, base))
        .AddConnection(0,5, GridRule::LocalToWorld(MultiGrid::RightDir, base)).AddConnection(5,0, GridRule::LocalToWorld(MultiGrid::LeftDir, base))
        .AddConnection(0,6, GridRule::LocalToWorld(MultiGrid::DownDir, base)).AddConnection(6,0, GridRule::LocalToWorld(MultiGrid::UpDir, base))
        .AddConnection(4,1, GridRule::LocalToWorld(MultiGrid::LeftDir, base)).AddConnection(1,4, GridRule::LocalToWorld(MultiGrid::RightDir, base))
        .AddConnection(5,2, GridRule::LocalToWorld(MultiGrid::RightDir, base)).AddConnection(2,5, GridRule::LocalToWorld(MultiGrid::LeftDir, base))
        .AddConnection(6,3, GridRule::LocalToWorld(MultiGrid::DownDir, base)).AddConnection(3,6, GridRule::LocalToWorld(MultiGrid::UpDir, base))
        .EndRepl()
      .End();
    }

    for(unsigned int i = 0; i<0; ++i)
    {
      MultiGrid::Direction base = (MultiGrid::Direction)(2*i);
      m_Grammar[1]->StartRule().AddNode(0, true).AddNode(0, true).AddNode().AddNode()
        .AddConnection(2,0, GridRule::LocalToWorld(MultiGrid::LeftDir, base)).AddConnection(0,2, GridRule::LocalToWorld(MultiGrid::RightDir, base))
        .AddConnection(0,1, GridRule::LocalToWorld(MultiGrid::LeftDir, base)).AddConnection(1,0, GridRule::LocalToWorld(MultiGrid::RightDir, base))
        .AddConnection(1,3, GridRule::LocalToWorld(MultiGrid::LeftDir, base)).AddConnection(3,1, GridRule::LocalToWorld(MultiGrid::RightDir, base))
        .EndMatch()
        .BeginRepl()
        .AddReplNode(0).AddReplNode(1).AddReplNode(2).AddReplNode(3).AddNode(0).AddNode(0)
        .AddConnection(2,0, GridRule::LocalToWorld(MultiGrid::LeftDir, base)).AddConnection(0,2, GridRule::LocalToWorld(MultiGrid::RightDir, base))
        .AddConnection(0,4, GridRule::LocalToWorld(MultiGrid::LeftDir, base)).AddConnection(4,0, GridRule::LocalToWorld(MultiGrid::RightDir, base))
        .AddConnection(4,5, GridRule::LocalToWorld(MultiGrid::LeftDir, base)).AddConnection(5,4, GridRule::LocalToWorld(MultiGrid::RightDir, base))
        .AddConnection(5,1, GridRule::LocalToWorld(MultiGrid::LeftDir, base)).AddConnection(1,5, GridRule::LocalToWorld(MultiGrid::RightDir, base))
        .AddConnection(1,3, GridRule::LocalToWorld(MultiGrid::LeftDir, base)).AddConnection(3,1, GridRule::LocalToWorld(MultiGrid::RightDir, base))
        .EndRepl()
        .End();
    }
  }

  void LSystem::Apply(Graph& ioGraph) const
  {
    LevelGrammar_Old::Graph tempGraph;
    LevelGrammar_Old::VertexMatching matching;
    m_Grammar[0]->Apply(ioGraph, tempGraph, matching);
    m_Grammar[1]->Apply(tempGraph, ioGraph, matching);
    //m_Grammar3.Apply(ioGraph, tempGraph, matching);
    //ioGraph.swap(ioGraph);
  }
}