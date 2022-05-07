#include <engine/gui/guisystem.hpp>
#include <engine/gui/guilib.hpp>
#include <engine/gfx/gfxguirendernode.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(GUISystem);

  struct GUISystem::Impl
  {
    Impl()
    {
      m_WinSetup.m_DesignDimension = Vec2i(1024, 768);
    }

    struct DlgEntry
    {
      ObjectHandle m_WorldParent;
      IntrusivePtr<GUI::Dialog> m_Dialog;
    };

    GfxRenderNodeHandle m_RenderHandle;
    GUI::WindowSetup m_WinSetup;
    Vector<DlgEntry> m_Dialogs;
  };

  GUISystem::GUISystem() = default;
  GUISystem::~GUISystem() = default;

  void GUISystem::SetViewport(Vec2i iSize)
  {
    m_Impl->m_WinSetup.SetWindowSize(iSize);
  }

  void GUISystem::Register(World& iWorld)
  {
    WorldSystem::Register(iWorld);
    m_Impl = std::make_unique<Impl>();
    GfxSystem* gfx = iWorld.GetSystem<GfxSystem>();
    if (gfx)
    {
      m_Impl->m_RenderHandle = gfx->AddRenderNode(std::make_unique<GfxGUIRenderNode>());
    }
  }

  void GUISystem::ProcessInputs(InputSystem& iInputs)
  {

  }

  void GUISystem::AddDialog(IntrusivePtr<GUI::Dialog> iDlg, Optional<ObjectHandle> iParent)
  {
    if (!iDlg ||
      std::find_if(m_Impl->m_Dialogs.begin(), m_Impl->m_Dialogs.end(), [&iDlg](Impl::DlgEntry const& iEntry)
      {
        return iEntry.m_Dialog == iDlg;
      })
      != m_Impl->m_Dialogs.end())
    {
      return;
    }

    Impl::DlgEntry newEntry;
    newEntry.m_Dialog = iDlg;
    if (iParent)
    {
      newEntry.m_WorldParent = *iParent;
    }

    GfxSystem* gfx = GetWorld().GetSystem<GfxSystem>();
    if (gfx)
    {
      GfxGUIRenderNode& renderer = *GfxGUIRenderNode::DynamicCast(gfx->GetRenderNode(m_Impl->m_RenderHandle));
      // Center dialog on world objects by default.
      GUI::DlgDim baseDim{AABB2Di::FromCenterAndSize(Zero<Vec2i>(), m_Impl->m_WinSetup.m_WinDimension), 0};

      GUI::LayoutCtx ctx{ GetWorld(), renderer, m_Impl->m_WinSetup, ObjectHandle(), iParent};

      iDlg->Layout(baseDim, ctx);
    }

    m_Impl->m_Dialogs.push_back(newEntry);
  }

  void TryPickDialog(Transforms& iTrans, GUI::Dialog& iDlg, Vec2i const& iPointerPos)
  {
    if (!iDlg.IsPickable())
    {
      return;
    }

    ObjectHandle obj = iDlg.GetObject();
    Mat4 const& trans = iTrans.GetWorldTransform(obj);
    AABB2Di box = iDlg.GetLayoutBox();
    box.Translate(trans[3]);

    if (!box.Contains(iPointerPos))
    {
      return;
    }
    if (iDlg.m_Pick)
    {
      iDlg.m_Pick();
    }
    for (auto const& childPtr : iDlg.GetChildren())
    {
      TryPickDialog(iTrans, *childPtr, iPointerPos);
    }
  }

  void GUISystem::Pick(Vec2i const& iPointerPos)
  {
    if (GfxSystem* gfx = GetWorld().GetSystem<GfxSystem>())
    {
      Transforms& transforms = *m_World->GetSystem<Transforms>();
      for (auto const& dlgEntry : m_Impl->m_Dialogs)
      {
        Vec2i pointerPos = iPointerPos;
        pointerPos.y = m_Impl->m_WinSetup.m_WinDimension.y - iPointerPos.y;
        if (dlgEntry.m_WorldParent.IsAssigned())
        {
          Mat4 const& parentPos = transforms.GetWorldTransform(dlgEntry.m_WorldParent);
          Vec2i screenPos = gfx->WorldToScreen(parentPos[3]);
          screenPos.y = m_Impl->m_WinSetup.m_WinDimension.y - screenPos.y;
          pointerPos -= screenPos;
        }

        TryPickDialog(transforms, *dlgEntry.m_Dialog, pointerPos);
      }
    }
  }
}