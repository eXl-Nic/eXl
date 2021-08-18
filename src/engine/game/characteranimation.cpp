/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/game/characteranimation.hpp>

#include <engine/gfx/gfxcomponent.hpp>
#include <engine/gfx/gfxsystem.hpp>

#include <engine/game/grabability.hpp>
#include <engine/game/pickability.hpp>
#include <engine/game/throwability.hpp>
#include <engine/game/swordability.hpp>

#include <bitset>

namespace eXl
{
  namespace
  {
    struct DataTable
    {
      TileName m_InvalidTileName = TileName("INVALID");
      TileName m_StateAnimTable[12];
      TileName m_GrabStateAnimTable[12];
      TileName m_Grab_Opposed_StateAnimTable[12];
      TileName m_CarryAnimTable[12];
      TileName m_LiftingAnimTable[8];
      TileName m_DropAnimTable[8];
      TileName m_SwordAnimTable[12];

      DataTable()
      {
        m_StateAnimTable[0] = m_InvalidTileName;
        m_StateAnimTable[1] = m_InvalidTileName;
        m_StateAnimTable[2] = m_InvalidTileName;
        m_StateAnimTable[3] = m_InvalidTileName;
        m_StateAnimTable[4] = TileName("Idle_Left");
        m_StateAnimTable[5] = TileName("Idle_Right");
        m_StateAnimTable[6] = TileName("Idle_Down");
        m_StateAnimTable[7] = TileName("Idle_Up");
        m_StateAnimTable[8] = TileName("Walk_Left");
        m_StateAnimTable[9] = TileName("Walk_Right");
        m_StateAnimTable[10] = TileName("Walk_Down");
        m_StateAnimTable[11] = TileName("Walk_Up");

        m_GrabStateAnimTable[0] = m_InvalidTileName;
        m_GrabStateAnimTable[1] = m_InvalidTileName;
        m_GrabStateAnimTable[2] = m_InvalidTileName;
        m_GrabStateAnimTable[3] = m_InvalidTileName;
        m_GrabStateAnimTable[4] = TileName("Grab_Left");
        m_GrabStateAnimTable[5] = TileName("Grab_Right");
        m_GrabStateAnimTable[6] = TileName("Grab_Down");
        m_GrabStateAnimTable[7] = TileName("Grab_Up");
        m_GrabStateAnimTable[8] = TileName("Push_Left");
        m_GrabStateAnimTable[9] = TileName("Push_Right");
        m_GrabStateAnimTable[10] = TileName("Push_Down");
        m_GrabStateAnimTable[11] = TileName("Push_Up");

        m_Grab_Opposed_StateAnimTable[0] = m_InvalidTileName;
        m_Grab_Opposed_StateAnimTable[1] = m_InvalidTileName;
        m_Grab_Opposed_StateAnimTable[2] = m_InvalidTileName;
        m_Grab_Opposed_StateAnimTable[3] = m_InvalidTileName;
        m_Grab_Opposed_StateAnimTable[4] = TileName("Grab_Right");
        m_Grab_Opposed_StateAnimTable[5] = TileName("Grab_Left");
        m_Grab_Opposed_StateAnimTable[6] = TileName("Grab_Up");
        m_Grab_Opposed_StateAnimTable[7] = TileName("Grab_Down");
        m_Grab_Opposed_StateAnimTable[8] = TileName("Pull_Left");
        m_Grab_Opposed_StateAnimTable[9] = TileName("Pull_Right");
        m_Grab_Opposed_StateAnimTable[10] = TileName("Pull_Down");
        m_Grab_Opposed_StateAnimTable[11] = TileName("Pull_Up");

        m_CarryAnimTable[0] = m_InvalidTileName;
        m_CarryAnimTable[1] = m_InvalidTileName;
        m_CarryAnimTable[2] = m_InvalidTileName;
        m_CarryAnimTable[3] = m_InvalidTileName;
        m_CarryAnimTable[4] = TileName("Carry_Left");
        m_CarryAnimTable[5] = TileName("Carry_Right");
        m_CarryAnimTable[6] = TileName("Carry_Down");
        m_CarryAnimTable[7] = TileName("Carry_Up");
        m_CarryAnimTable[8] = TileName("Carry_Walk_Left");
        m_CarryAnimTable[9] = TileName("Carry_Walk_Right");
        m_CarryAnimTable[10] = TileName("Carry_Walk_Down");
        m_CarryAnimTable[11] = TileName("Carry_Walk_Up");

        m_LiftingAnimTable[0] = m_InvalidTileName;
        m_LiftingAnimTable[1] = m_InvalidTileName;
        m_LiftingAnimTable[2] = m_InvalidTileName;
        m_LiftingAnimTable[3] = m_InvalidTileName;
        m_LiftingAnimTable[4] = TileName("Lift_Left");
        m_LiftingAnimTable[5] = TileName("Lift_Right");
        m_LiftingAnimTable[6] = TileName("Lift_Down");
        m_LiftingAnimTable[7] = TileName("Lift_Up");

        m_DropAnimTable[0] = m_InvalidTileName;
        m_DropAnimTable[1] = m_InvalidTileName;
        m_DropAnimTable[2] = m_InvalidTileName;
        m_DropAnimTable[3] = m_InvalidTileName;
        m_DropAnimTable[4] = TileName("Drop_Left");
        m_DropAnimTable[5] = TileName("Drop_Right");
        m_DropAnimTable[6] = TileName("Drop_Down");
        m_DropAnimTable[7] = TileName("Drop_Up");

        m_SwordAnimTable[0] = m_InvalidTileName;
        m_SwordAnimTable[1] = m_InvalidTileName;
        m_SwordAnimTable[2] = m_InvalidTileName;
        m_SwordAnimTable[3] = m_InvalidTileName;
        m_SwordAnimTable[4] = TileName("Swing_Left");
        m_SwordAnimTable[5] = TileName("Swing_Right");
        m_SwordAnimTable[6] = TileName("Swing_Down");
        m_SwordAnimTable[7] = TileName("Swing_Up");
        m_SwordAnimTable[8] = TileName("Swing_Left");
        m_SwordAnimTable[9] = TileName("Swing_Right");
        m_SwordAnimTable[10] = TileName("Swing_Down");
        m_SwordAnimTable[11] = TileName("Swing_Up");
      }
    };
  }

  boost::optional<DataTable> s_DataTable;

  void CharacterAnimation_StaticInit()
  {
    s_DataTable.emplace();
  }

  void CharacterAnimation_StaticDestroy()
  {
    s_DataTable.reset();
  }

  void CharacterAnimation::UpdateAnimation(ObjectHandle iObj, CharacterAnimEntry& iEntry)
  {
#ifdef EXL_WITH_OGL
    uint32_t stateBits = (uint32_t)iEntry.m_CurDir;

    if (!iEntry.m_CurrentActionCue)
    {
      iEntry.m_GfxComp->SetTileName(s_DataTable->m_StateAnimTable[(uint32_t)iEntry.m_CurDir]);
    }
    else
    {
      if (*iEntry.m_CurrentActionCue == GrabAbility::GrabCue())
      {
         Vector2f dir = GrabAbility::GetGrabDirection(m_World->GetSystem<AbilitySystem>(), iObj);
         uint32_t grabState;
         if (dir.X() == 0)
         {
           if (dir.Y() > 0)
           {
             grabState = (uint32_t)CharacterSystem::StateFlags::DirUp;
           }
           else
           {
             grabState = (uint32_t)CharacterSystem::StateFlags::DirDown;
           }
         }
         else
         {
           if (dir.X() > 0)
           {
             grabState = (uint32_t)CharacterSystem::StateFlags::DirRight;
           }
           else
           {
             grabState = (uint32_t)CharacterSystem::StateFlags::DirLeft;
           }
         }

         if (grabState == ((uint32_t)iEntry.m_CurDir & (uint32_t)CharacterSystem::StateFlags::DirMask))
         {
           iEntry.m_GfxComp->SetTileName(s_DataTable->m_GrabStateAnimTable[(uint32_t)iEntry.m_CurDir]);
         }
         else
         {
           iEntry.m_GfxComp->SetTileName(s_DataTable->m_Grab_Opposed_StateAnimTable[(uint32_t)iEntry.m_CurDir]);
         }
      }
      else if (*iEntry.m_CurrentActionCue == PickAbility::CarryingCue())
      {
        iEntry.m_GfxComp->SetTileName(s_DataTable->m_CarryAnimTable[(uint32_t)iEntry.m_CurDir]);
      }
      else if (*iEntry.m_CurrentActionCue == PickAbility::LiftingCue())
      {
        iEntry.m_GfxComp->SetTileName(s_DataTable->m_LiftingAnimTable[(uint32_t)iEntry.m_CurDir]);
      }
      else if (*iEntry.m_CurrentActionCue == ThrowAbility::ThrowingCue())
      {
        iEntry.m_GfxComp->SetTileName(s_DataTable->m_DropAnimTable[(uint32_t)iEntry.m_CurDir]);
      }
      else if (*iEntry.m_CurrentActionCue == SwordAbility::UseSwordCue())
      {
        iEntry.m_GfxComp->SetTileName(s_DataTable->m_SwordAnimTable[(uint32_t)iEntry.m_CurDir]);
      }
    }
#endif
  }

  void CharacterAnimation::OnWalkingStateChange(ObjectHandle iObj, uint32_t iNewState)
  {
    auto objectIter = m_Objects.find(iObj);
    if (objectIter == m_Objects.end())
    {
      return;
    }

    auto& entry = m_Entries.Get(objectIter->second);
    entry.m_CurDir = (CharacterSystem::StateFlags)iNewState;
    UpdateAnimation(iObj, entry);
  }

  void CharacterAnimation::OnCueChange(ObjectHandle iObject, GameCueChange const& iChange)
  {
    auto objectIter = m_Objects.find(iObject);
    if (objectIter == m_Objects.end())
    {
      return;
    }

    auto& entry = m_Entries.Get(objectIter->second);
    if (iChange.m_Change == GameCueChange::Added)
    {
      entry.m_CurrentActionCue = iChange.m_Object;
    }
    if (iChange.m_Change == GameCueChange::Removed)
    {
      entry.m_CurrentActionCue = {};
    }
    if (iChange.m_Change == GameCueChange::Fired)
    {

    }

    UpdateAnimation(iObject, entry);
  }

  void CharacterAnimation::Register(World& iWorld)
  {
    m_World = &iWorld;
    auto& abilities = *iWorld.GetSystem<AbilitySystem>();
    std::function<void(ObjectHandle iObject, GameCueChange const& iChange)> callback = [this](ObjectHandle iObject, GameCueChange const& iChange)
    {
      OnCueChange(iObject, iChange);
    };
    abilities.AddCueChangeCallback(std::move(callback));
  }

  void CharacterAnimation::Tick(World& iWorld)
  {

  }

  void CharacterAnimation::AddCharacter(ObjectHandle iObject)
  {
    if (m_Objects.count(iObject) != 0)
    {
      return;
    }
#ifdef EXL_WITH_OGL
    GfxSystem* gfxSys = m_World->GetSystem<GfxSystem>();
    if (gfxSys)
    {
      GfxSpriteComponent* comp = gfxSys->GetSpriteComponent(iObject);
      if (comp == nullptr)
      {
        return;
      }

      auto newHandle = m_Entries.Alloc();
      auto& newEntry = m_Entries.Get(newHandle);
      m_Objects.insert(std::make_pair(iObject, newHandle));

      newEntry.m_GfxComp = comp;
    }
#endif
  }

  void CharacterAnimation::RemoveCharacter(ObjectHandle iObject)
  {
    auto objectIter = m_Objects.find(iObject);
    if(objectIter == m_Objects.end())
    {
      return;
    }

    m_Entries.Release(objectIter->second);
    m_Objects.erase(objectIter);
  }
}