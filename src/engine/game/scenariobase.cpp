/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/game/scenariobase.hpp>

#include <engine/game/characteranimation.hpp>
#include <engine/game/ability.hpp>
#include <engine/game/walkability.hpp>

#include <engine/common/transforms.hpp>
#include <engine/game/character.hpp>
#include <engine/pathfinding/navigator.hpp>
#include <engine/gfx/gfxsystem.hpp>
#include <engine/gfx/gfxcomponent.hpp>

#include <engine/gui/guisystem.hpp>
#include <engine/gui/guilib.hpp>

#include <core/input.hpp>

#include <math/mathtools.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(Scenario_Base);


  Scenario_Base::Scenario_Base() = default;
  Scenario_Base::~Scenario_Base() = default;

  void Scenario_Base::Init(World& iWorld)
  {
    m_DefaultAnim = std::make_unique<CharacterAnimation>();
    m_DefaultAnim->Register(iWorld);

    Engine_Application& appl = static_cast<Engine_Application&>(Application::GetAppl());

    auto& abilitySys = *iWorld.GetSystem<AbilitySystem>();
    abilitySys.RegisterAbility(new WalkAbility);

    if (MapResource const* map = m_Map.GetOrLoad())
    {
      m_InstatiatedMap = map->Instantiate(iWorld);
    }

    if (m_InstatiatedMap.navMesh)
    {
      auto components = m_InstatiatedMap.navMesh->GetComponents();
      if (components.size() > 0)
      {
        eXl_ASSERT_REPAIR_RET(components[0].m_Faces.size() > 0, );
        m_SpawnPos = components[0].m_Faces[0].m_Box.GetCenter();
      }
    }

    iWorld.AddTick(World::FrameStart, [this](World& iWorld, float)
    {
      ProcessInputs(iWorld);
    });

    StartLocal(iWorld);
  }

  ObjectHandle Scenario_Base::SpawnCharacter(World& iWorld, Vec3 const& iPos, EngineCommon::CharacterControlKind iControl, ObjectCreationInfo const& iInfo)
  {
    Archetype const* toSpawn = nullptr;
    if ((toSpawn = m_MainCharacter.GetOrLoad()) == nullptr
      || !toSpawn->HasComponent(EngineCommon::CharacterComponentName()))
    {
      return ObjectHandle();
    }

    DynObject var;
    var.SetType(TypeManager::GetType<EngineCommon::CharacterControlKind>(), &iControl);

    CustomizationData customData;
    customData.m_PropertyCustomization = { {EngineCommon::CharacterDesc::PropertyName(),
      CustomizationData::FieldsMap({ {eXl_FIELD_NAME(EngineCommon::CharacterDesc, m_Control), var } })} };

    ObjectHandle newChar = iWorld.CreateObject(iInfo);
    Transforms& trans = *iWorld.GetSystem<Transforms>();
    trans.AddTransform(newChar, translate(Identity<Mat4>(), iPos));
    toSpawn->Instantiate(newChar, iWorld, &customData);

#if 0
    GUISystem* gui = iWorld.GetSystem<GUISystem>();
    if (gui)
    {
      Resource::UUID id = { 120981379, 340138741, 394802740, 782939172 };

      auto charName = MakeRefCounted<GUI::Text>(GUI::Size(GUI::Dim(1, 0), GUI::Dim(1, 0)));
      charName->m_Font.SetUUID(id);
      charName->m_Text = "Blah";

      auto frame = MakeRefCounted<GUI::Image>(GUI::Size(GUI::Dim(1, 0), GUI::Dim(1, 0)));
      frame->m_ImgDesc.m_Tileset.Set(Tileset::GetWhiteTexture());
      frame->m_ImgDesc.m_TileName = Tileset::GetWhiteTexture()->begin()->first;
      frame->m_ImgDesc.m_RotateSprite = true;
      frame->m_StretchToSize = true;
      frame->m_ImgDesc.m_Tint = Vec4(0,0,0,0.25);
      frame->m_Pick = []
      {
        LOG_INFO << "Picked button";
      };

      auto stack = MakeRefCounted<GUI::Dialog>(GUI::Size(GUI::Dim(1, 0), GUI::Dim(0.2, 0)));
      stack->m_Children.push_back(frame);
      stack->m_Children.push_back(charName);

      auto window = MakeRefCounted<GUI::Container>(GUI::Size(GUI::Dim(0,100), GUI::Dim(0,100)));
      window->AddDialog(stack, GUI::Position(GUI::Dim(0, 0), GUI::Dim(0, 0), GUI::Position::UpperLeft));

      gui->AddDialog(window, newChar);
    }
#endif

    return newChar;
  }

  void Scenario_Base::StartLocal(World& iWorld)
  {
    m_MainChar = SpawnCharacter(iWorld, Vec3(m_SpawnPos, 0), EngineCommon::CharacterControlKind::PlayerControl);

    auto& transforms = *iWorld.GetSystem<Transforms>();
    //transforms.Attach(GetCamera().cameraObj, m_MainChar, Transforms::Position);
  }

  void Scenario_Base::ProcessInputs(World& iWorld)
  {
    Engine_Application& app = Engine_Application::GetAppl();
    InputSystem& iInputs = app.GetInputSystem();

    GfxSystem* gfxSys = iWorld.GetSystem<GfxSystem>();
    Vec2i vptSize = gfxSys ? gfxSys->GetViewportSize() : One<Vec2i>();

    AbilitySystem& abilities = *iWorld.GetSystem<AbilitySystem>();
    
    for (MouseMoveEvent const& evt : iInputs.m_MouseMoveEvts)
    {
      curMousePos.x = evt.absX;
      curMousePos.y = evt.absY;
    }
    GUISystem* gui = iWorld.GetSystem<GUISystem>();
    if (gui)
    {
      for (int i = 0; i < (int)iInputs.m_MouseEvts.size(); ++i)
      {
        MouseEvent const& evt = iInputs.m_MouseEvts[i];
        if (evt.button == MouseButton::Left
          && !evt.pressed)
        {
          gui->Pick(curMousePos);
        }
      }
    }

    if (m_MainChar.IsAssigned())
    {
      for (int i = 0; i < (int)iInputs.m_KeyEvts.size(); ++i)
      {
        KeyboardEvent& evt = iInputs.m_KeyEvts[i];
        if (!evt.pressed)
        {
          if (evt.key == K_UP)
          {
            dirMask &= ~(1 << 2);
            keyChanged = true;
          }
          if (evt.key == K_DOWN)
          {
            dirMask &= ~(1 << 3);
            keyChanged = true;
          }
          if (evt.key == K_LEFT)
          {
            dirMask &= ~(1 << 1);
            keyChanged = true;
          }
          if (evt.key == K_RIGHT)
          {
            dirMask &= ~(1 << 0);
            keyChanged = true;
          }
        }
        else
        {
          if (evt.key == K_UP)
          {
            dirMask |= 1 << 2;
            keyChanged = true;
          }
          if (evt.key == K_DOWN)
          {
            dirMask |= 1 << 3;
            keyChanged = true;
          }
          if (evt.key == K_LEFT)
          {
            dirMask |= 1 << 1;
            keyChanged = true;
          }
          if (evt.key == K_RIGHT)
          {
            dirMask |= 1 << 0;
            keyChanged = true;
          }
        }

        if (keyChanged)
        {
          iInputs.m_KeyEvts.erase(iInputs.m_KeyEvts.begin() + i);
          --i;
        }
      }
    }
    if (keyChanged)
    {
      static const Vec3 dirs[] =
      {
        UnitX<Vec3>() *  1.0,
        UnitX<Vec3>() * -1.0,
        UnitY<Vec3>() *  1.0,
        UnitY<Vec3>() * -1.0,
      };
      Vec3 dir = Zero<Vec3>();
      for (unsigned int i = 0; i < 4; ++i)
      {
        if (dirMask & (1 << i))
        {
          dir += dirs[i];
        }
      }

      if (dirMask == 0)
      {
        abilities.StopUsingAbility(m_MainChar, WalkAbility::Name());
      }
      else
      {
        WalkAbility::SetWalkDirection(&abilities, m_MainChar, MathTools::As2DVec(dir));
        abilities.UseAbility(m_MainChar, WalkAbility::Name());
      }
      keyChanged = false;
    }

    GetCamera().ProcessInputs(iWorld, iInputs);
  }
}