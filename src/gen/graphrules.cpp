#include <gen/graphrules.hpp>

namespace eXl 
{
  IMPLEMENT_SERIALIZE_METHODS(Rule);
  IMPLEMENT_SERIALIZE_METHODS(Rule::Node);
  IMPLEMENT_SERIALIZE_METHODS(Rule::Edge);
  IMPLEMENT_SERIALIZE_METHODS(Rule::NewEdge);
  IMPLEMENT_SERIALIZE_METHODS(TagDef);
  IMPLEMENT_SERIALIZE_METHODS(RoomLayoutInfo);
  IMPLEMENT_SERIALIZE_METHODS(RewriteSystem);
  IMPLEMENT_SERIALIZE_METHODS(RewriteSystem::SeqItem);

  Err Rule::Node::Serialize(Serializer iSerializer) 
  {
    iSerializer.BeginStruct();
    iSerializer.PushKey("Tag");
    iSerializer &= tag;
    iSerializer.PopKey();
    iSerializer.PushKey("EditorName");
    iSerializer &= tag;
    iSerializer.PopKey();
    iSerializer.PushKey("EditorPosition");
    iSerializer &= tag;
    iSerializer.PopKey();
    return iSerializer.EndStruct();
  }

  Err Rule::Serialize(Serializer iSerializer) 
  {
    iSerializer.BeginStruct();
    iSerializer.PushKey("ContextNodes");
    iSerializer &= m_ContextNodes;
    iSerializer.PopKey();
    iSerializer.PushKey("CreateNodes");
    iSerializer &= m_CreateNodes;
    iSerializer.PopKey();
    iSerializer.PushKey("CutNodes");
    iSerializer &= m_CutNodes;
    iSerializer.PopKey();
    iSerializer.PushKey("ContextEdges");
    iSerializer &= m_ContextEdges;
    iSerializer.PopKey();
    iSerializer.PushKey("CutEdges");
    iSerializer &= m_CutEdge;
    iSerializer.PopKey();
    iSerializer.PushKey("NewEdges");
    iSerializer &= m_NewEdge;
    iSerializer.PopKey();
    iSerializer.PushKey("CustomData");
    iSerializer &= m_RuleCustomData;
    iSerializer.PopKey();
    return iSerializer.EndStruct();
  }

  Err Rule::Edge::Serialize(Serializer iSerializer)
  {
    iSerializer.BeginStruct();
    iSerializer.PushKey("Tag");
    iSerializer &= tag;
    iSerializer.PopKey();
    iSerializer.PushKey("Source");
    iSerializer &= nodes[0];
    iSerializer.PopKey();
    iSerializer.PushKey("Target");
    iSerializer &= nodes[1];
    iSerializer.PopKey();
    return iSerializer.EndStruct();
    
  };

  Err Rule::NewEdge::Serialize(Serializer iSerializer)
  {
    iSerializer.BeginStruct();
    iSerializer.PushKey("Tag");
    iSerializer &= tag;
    iSerializer.PopKey();
    iSerializer.PushKey("Source");
    iSerializer &= nodes[0];
    iSerializer.PopKey();
    iSerializer.PushKey("Target");
    iSerializer &= nodes[1];
    iSerializer.PopKey();
    iSerializer.PushKey("SourcePort");
    iSerializer &= port[0];
    iSerializer.PopKey();
    iSerializer.PushKey("TargetPort");
    iSerializer &= port[1];
    iSerializer.PopKey();
    return iSerializer.EndStruct();
  }

  Err TagDef::Serialize(Serializer iSerializer)
  {
    iSerializer.BeginStruct();
    iSerializer.PushKey("CustomData");
    iSerializer &= m_TagCustomData;
    iSerializer.PopKey();
    iSerializer.PushKey("IsNodeTag");
    iSerializer &= m_IsNodeTag;
    iSerializer.PopKey();
    return iSerializer.EndStruct();
  }

  Err RoomLayoutInfo::Serialize(Serializer iSerializer)
  {
    iSerializer.BeginStruct();
    iSerializer.PushKey("CollapseNode");
    iSerializer &= m_CollapseNode;
    iSerializer.PopKey();
    iSerializer.PushKey("TerrainType");
    iSerializer &= m_TerrainType;
    iSerializer.PopKey();
    iSerializer.PushKey("RoomSizes");
    iSerializer &= m_RoomSizes;
    iSerializer.PopKey();
    iSerializer.PushKey("Layout");
    iSerializer &= m_Layout;
    iSerializer.PopKey();
    return iSerializer.EndStruct();
  }

  Err RewriteSystem::Serialize(Serializer iSerializer)
  {
    iSerializer.BeginStruct();
    iSerializer.PushKey("Tags");
    iSerializer.HandleMapSorted(m_Tags);
    iSerializer.PopKey();
    iSerializer.PushKey("Rules");
    iSerializer.HandleMapSorted(m_Rules);
    iSerializer.PopKey();
    iSerializer.PushKey("CurrentSequence");
    iSerializer &= m_CurSequence;
    iSerializer.PopKey();
    return iSerializer.EndStruct();
  }

  Err RewriteSystem::SeqItem::Serialize(Serializer iSerializer)
  {
    iSerializer.BeginStruct();
    iSerializer.PushKey("Rule");
    iSerializer &= m_Rule;
    iSerializer.PopKey();
    iSerializer.PushKey("Application");
    String appl;
    if (iSerializer.IsReading())
    {
      iSerializer &= appl;
      if(appl == "AllMatch")
      {
        m_Appl = RuleApplication::AllMatch;
      }
      else
      {
        m_Appl = RuleApplication::OneMatch;
      }
    }
    else
    {
      if (m_Appl == RuleApplication::AllMatch)
      {
        appl = "AllMatch";
      }
      else
      {
        appl = "OneMatch";
      }
      iSerializer &= appl;
    }
    iSerializer.PopKey();
    return iSerializer.EndStruct();
  }

  Name RewriteSystem::GetAnyTag()
  {
    static Name s_Tag("<Any>");
    return s_Tag;
  }

}

