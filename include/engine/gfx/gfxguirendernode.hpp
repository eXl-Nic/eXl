#pragma once

#include <engine/gfx/gfxsystem.hpp>
#include <engine/common/gamedata.hpp>

namespace eXl
{
  class FontResource;

  class EXL_ENGINE_API GfxGUIRenderNode : public GfxRenderNode
  {
    DECLARE_RTTI(GfxGUIRenderNode, GfxRenderNode);
  public:
    GfxGUIRenderNode();
    ~GfxGUIRenderNode();
    GfxGUIRenderNode(GfxGUIRenderNode const&) = delete;

    void AddText(ObjectHandle iObject, String const& iText, uint32_t iSize, FontResource const* iFont);

  protected:
    void Init(GfxSystem& iSys, GfxRenderNodeHandle iHandle) override;
    TransformUpdateCallback GetTransformUpdateCallback() override;
    UpdateCallback GetDeleteCallback() override;
    void Push(OGLDisplayList& iList, float iDelta);

    struct Impl;
    UniquePtr<Impl> m_Impl;
  };
}