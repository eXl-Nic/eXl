#pragma once

#include <core/input.hpp>
#include <engine/common/world.hpp>
#include <engine/gui/guilib.hpp>

namespace eXl
{
  namespace GUI
  {
    class Dialog;
  }

  class EXL_ENGINE_API GUISystem : public WorldSystem
  {
    DECLARE_RTTI(GUISystem, WorldSystem);

  public:

    GUISystem();
    ~GUISystem();

    void Register(World& iWorld) override;

    void ProcessInputs(InputSystem& iInputs);

    void AddDialog(IntrusivePtr<GUI::Dialog> iDlg, Optional<ObjectHandle> iWorldParent = {});

    void RemoveDialog(IntrusivePtr<GUI::Dialog> const& iDlg);

    void RefreshDialog(IntrusivePtr<GUI::Dialog> const& iDlg);

    void SetViewport(Vec2i iSize);

    void Pick(Vec2i const& iPointerPos);

  protected:
    struct Impl;
    UniquePtr<Impl> m_Impl;
  };
}