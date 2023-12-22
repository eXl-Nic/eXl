#pragma once

#include <gen/gen_exp.hpp>
#include <core/coredef.hpp>
#include <core/name.hpp>
#include <core/stream/serializer.hpp>
#include <math/aabb2d.hpp>

namespace eXl
{
  class Streamer;
  class Unstreamer;

  struct EXL_GEN_API Rule
  {
    
    struct EXL_GEN_API Node
    {
      Name tag;
      Name editorName;
      Vec2 editorPosition;
      SERIALIZE_METHODS;
    };

    Vector<Name> m_ContextNodes;
    Vector<Name> m_CreateNodes;
    Vector<Name> m_CutNodes;

    struct EXL_GEN_API Edge
    {
      Name tag;
      uint32_t nodes[2];
      SERIALIZE_METHODS;
    };

    struct EXL_GEN_API NewEdge
    {
      Name tag;
      uint32_t nodes[2];
      uint32_t port[2];
      SERIALIZE_METHODS;
    };

    Vector<Edge> m_ContextEdges;
    Vector<Edge> m_CutEdge;
    Vector<NewEdge> m_NewEdge;
    String m_RuleCustomData;
    SERIALIZE_METHODS;
  };

  struct EXL_GEN_API TagDef
  {
    String m_TagCustomData;
    bool m_IsNodeTag;
    SERIALIZE_METHODS;
  };

  struct EXL_GEN_API RoomLayoutInfo
  {
    bool m_CollapseNode;
    String m_TerrainType;
    Vector<Vec2i> m_RoomSizes;
    AABB2Di m_Layout;
    SERIALIZE_METHODS;
  };

  enum class RuleApplication
  {
    OneMatch,
    AllMatch
  };

  class EXL_GEN_API RewriteSystem
  {
  public:

    UnorderedMap<String, Rule> m_Rules;
    UnorderedMap<Name, TagDef> m_Tags;

    struct EXL_GEN_API SeqItem
    {
      String m_Rule;
      RuleApplication m_Appl;
      SERIALIZE_METHODS;
    };

    Vector<SeqItem> m_CurSequence;

    static Name GetAnyTag();

    SERIALIZE_METHODS;
  };
}
