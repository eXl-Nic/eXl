#include "navigatorbench.hpp"

#include <math/mathtools.hpp>

#include <engine/common/world.hpp>

#include <engine/game/projectile.hpp>

#include <engine/physics/physicsys.hpp>
#include <engine/physics/physiccomponent.hpp>
#include <engine/pathfinding/navmesh.hpp>
#include <engine/pathfinding/navigator.hpp>

namespace eXl
{
  namespace NavigatorBench
  {
    struct Data
    {
      UniquePtr<Random> m_Rand;
      Vector<ObjectHandle> m_Agents;
      ProbaTable m_ProbaTable;
      uint32_t m_Component;
      bool m_StepAgents = false;
    };

    Optional<Vector2f> PickRandomPosInBox(AABB2Df const& iBox, Random& iRand)
    {
      Vector2f size = iBox.GetSize();
      if (size.X() <= 4 || size.Y() <= 4)
      {
        return {};
      }
      AABB2Df trimmedFace = iBox;
      trimmedFace.m_Data[0] += Vector2f::ONE * 2.0;
      trimmedFace.m_Data[1] -= Vector2f::ONE * 2.0;
      size = trimmedFace.GetSize();

      return trimmedFace.m_Data[0] + Vector2f(
        size.X() * (iRand.Generate() % 1000) / 1000.0, 
        size.Y() * (iRand.Generate() % 1000) / 1000.0);
    };

    Vector3f PickRandomPos(NavMesh const& iNavMesh, uint32_t iComponent, Random& iRand)
    {
      Optional<Vector2f> randPos;
      while(!randPos)
      {
        uint32_t faceIdx = iRand.Generate() % iNavMesh.GetFaces(iComponent).size();
        AABB2Df curFace = iNavMesh.GetFaces(iComponent)[faceIdx].m_Box;
        randPos = PickRandomPosInBox(curFace, iRand);
      }
      
      return Vector3f(randPos->X(), randPos->Y(), 0.0);
    };

    Vector3f PickRandomDest(Vector3f const& iCurPos, NavMesh const& iNavMesh, uint32_t iComponent, Random& iRand, NavigatorBench::ProbaTable const& iProbaTable)
    {
      auto faceIdx = iNavMesh.FindFace(Vector2f(iCurPos.X(), iCurPos.Y()));
      if (!faceIdx)
      {
        return iCurPos;
      }
      Vector2f randPos;
      uint32_t destFaceIdx = faceIdx->m_Face;
      if (iNavMesh.GetFaces(iComponent).size() > 1)
      {
        do
        {
          //destFaceIdx = iRand.Generate() % iNavMesh.GetFaces(iComponent).size();
          float sample = float(iRand.Generate() % 10000) / 10000;
          auto iter = std::lower_bound(iProbaTable.begin(), iProbaTable.end(), std::make_pair(sample, 0u));
          destFaceIdx = iter == iProbaTable.end() ? iProbaTable.back().second : iter->second;

          if (auto potentialPos = PickRandomPosInBox(iNavMesh.GetFaces(iComponent)[destFaceIdx].m_Box, iRand))
          {
            randPos = *potentialPos;
          }
          else
          {
            destFaceIdx = faceIdx->m_Face;
          }

        } while (destFaceIdx == faceIdx->m_Face);
      }
      return Vector3f(randPos.X(), randPos.Y(), 0.0);
    };


    void BuildCrossingTest(World& iWorld, Archetype const& iArch, CharacterSystem::Desc& ioBaseDesc, NavMesh const& iNavMesh, uint32_t iComponent, Data& ioData)
    {
      auto const& faces = iNavMesh.GetFaces(iComponent);
      float curScore = -FLT_MAX;
      uint32_t biggestFace = 0;
      for (uint32_t i = 0; i < faces.size(); ++i)
      {
        auto const& face = faces[i];
        Vector2f size = face.m_Box.GetSize();
        float score = Mathf::Min(size.X(), size.Y()) - Mathf::Abs(size.X() - size.Y());
        if (score > curScore)
        {
          biggestFace = i;
          curScore = score;
        }
      }

      auto const& face = faces[biggestFace];
      Vector2f center = face.m_Box.GetCenter();
      Vector2f size = face.m_Box.GetSize() - (Vector2f::ONE * 4 * 4);
      float radius = Mathf::Min(size.X(), size.Y()) * 0.5;
      float perimeter = radius * 2 * Mathf::Pi();

      uint32_t numActors = (perimeter / (1.2 * 4));
      float increment = Mathf::Pi() * 2 / numActors;

      Vector<ObjectHandle> autonomousAgents;
      auto& transforms = *iWorld.GetSystem<Transforms>();
      auto& navigator = *iWorld.GetSystem<NavigatorSystem>();


      ioBaseDesc.kind = CharacterSystem::PhysicKind::Simulated;
      ioBaseDesc.controlKind = CharacterSystem::ControlKind::Navigation;
      ioBaseDesc.maxSpeed = 10.0;

      for (unsigned int i = 0; i < numActors / 2; ++i)
      {
        Vector3f curPos(Mathf::Cos(i * increment) * radius + center.X(), Mathf::Sin(i * increment) * radius + center.Y(), 0.0);
        Vector3f destPos(Mathf::Cos(i * increment + Mathf::Pi()) * radius + center.X(), Mathf::Sin(i * increment + Mathf::Pi()) * radius + center.Y(), 0.0);

        ObjectHandle truc = iWorld.CreateObject();
        transforms.AddTransform(truc, Matrix4f::FromPosition(curPos));
        iArch.Instantiate(truc, iWorld, nullptr);
        navigator.SetDestination(truc, destPos);

        ObjectHandle truc2 = iWorld.CreateObject();
        transforms.AddTransform(truc2, Matrix4f::FromPosition(destPos));
        iArch.Instantiate(truc2, iWorld, nullptr);
        navigator.SetDestination(truc2, curPos);

        autonomousAgents.push_back(truc);
        autonomousAgents.push_back(truc2);
      }

      ioData.m_Agents = std::move(autonomousAgents);
      ioData.m_Component = iComponent;
      ioData.m_StepAgents = false;
    }

    void NavigatorBench::BuildFullScaleTest(World& iWorld, Archetype const& iArch, CharacterSystem::Desc& ioBaseDesc, uint32_t iNumNavAgents, NavMesh const& iNavMesh, uint32_t iComponent, Data& ioData)
    {
      Vector<ObjectHandle> autonomousAgents;
      auto& transforms = *iWorld.GetSystem<Transforms>();
      auto& gfxSys = *iWorld.GetSystem<GfxSystem>();
      auto& phSys = *iWorld.GetSystem<PhysicsSystem>();
      auto& navigator = *iWorld.GetSystem<NavigatorSystem>();

      ioBaseDesc.kind = CharacterSystem::PhysicKind::Simulated;
      ioBaseDesc.controlKind = CharacterSystem::ControlKind::Navigation;
      ioBaseDesc.maxSpeed = 10.0;

      ProbaTable probaTable;
      float totalArea = 0;
      for (uint32_t i = 0; i< iNavMesh.GetFaces(iComponent).size(); ++i)
      {
        auto const& Face = iNavMesh.GetFaces(iComponent)[i];
        Vector2f size = Face.m_Box.GetSize();
        float area = size.X() * size.Y();
        if (area < 36)
        {
          continue;
        }
        totalArea += area;
        probaTable.push_back(std::make_pair(area, i));
      }

      std::sort(probaTable.begin(), probaTable.end());

      float accum = 0;
      for (auto& entry : probaTable)
      {
        float curProba = accum / totalArea;
        accum += entry.first;
        entry.first = curProba;
      }

      for (unsigned int i = 0; i < iNumNavAgents; ++i)
      {
        //auto curPos = PickRandomPos(iNavMesh, iComponent, iRand);

        uint32_t faceIdx = ioData.m_Rand->Generate() % iNavMesh.GetFaces(iComponent).size();
        AABB2Df curFace = iNavMesh.GetFaces(iComponent)[faceIdx].m_Box;
        Vector3f randPos = PickRandomPos(iNavMesh, iComponent, *ioData.m_Rand);
        auto curPos = Vector3f(randPos.X(), randPos.Y(), 0.0);

        auto destPos = PickRandomDest(curPos, iNavMesh, iComponent, *ioData.m_Rand, probaTable);
      
        ObjectHandle truc = iWorld.CreateObject();
        transforms.AddTransform(truc, Matrix4f::FromPosition(curPos));
        navigator.SetDestination(truc, destPos);
        iArch.Instantiate(truc, iWorld, nullptr);

        if (curPos != destPos)
        {
          navigator.SetDestination(truc, destPos);
        }
        autonomousAgents.push_back(truc);
      }
      ioData.m_Agents = std::move(autonomousAgents);
      ioData.m_Component = iComponent;
      ioData.m_ProbaTable = std::move(probaTable);
      ioData.m_StepAgents = true;

    }

    ObjectHandle CreateProjectile(World& iWorld, Archetype const& iArch, Vector3f const& iPos, Vector3f const& iDir)
    {
      auto& transforms = *iWorld.GetSystem<Transforms>();
      auto& projectiles = *iWorld.GetSystem<ProjectileSystem>();

      ObjectHandle projectile = iWorld.CreateObject();
      Matrix4f objPos = Matrix4f::IDENTITY;
      MathTools::GetPosition(objPos) = iPos;
      transforms.AddTransform(projectile, &objPos);
      iArch.Instantiate(projectile, iWorld, nullptr);

      ProjectileSystem::Desc projDesc;
      projDesc.kind = ProjectileSystem::PhysicKind::Kinematic;
      projDesc.size = 1.0;
      projDesc.rotateSprite = true;
      projDesc.registerNav = true;

      projectiles.AddProjectile(projectile, projDesc, iDir * 20);

      iWorld.AddTimer(10.0, false, [projectile](World& iWorld)
      {
        iWorld.DeleteObject(projectile);
      });

      return projectile;
    }

    void StepFullScaleTest(World& world, float iDelta, NavMesh const& iNavMesh, uint32_t iComponent, Data& iData)
    {
      if (!iData.m_StepAgents)
      {
        return;
      }
      auto& transforms = *world.GetSystem<Transforms>();
      auto& navigator = *world.GetSystem<NavigatorSystem>();
      for (auto evt : navigator.DispatchEvents())
      {
        Matrix4f transform = transforms.GetWorldTransform(evt.m_Object);
        Vector3f const& curPos = *reinterpret_cast<Vector3f*>(transform.m_Data + 12);
        navigator.SetDestination(evt.m_Object, PickRandomDest(curPos, iNavMesh, iComponent, *iData.m_Rand, iData.m_ProbaTable));
      }
    }
  }
}

#include <engine/common/app.hpp>
#include <engine/common/menumanager.hpp>
#include <engine/game/scenariobase.hpp>
#include <imgui.h>

#include <core/resource/resourcemanager.hpp>

namespace eXl
{
  namespace NavigatorBench
  {
    class NavigatorBenchPanel : public MenuManager::Panel
    {
    public:
      NavigatorBenchPanel(World& iWorld) : m_World(iWorld)
      {
        m_Data.reset(new Data);
        m_Data->m_Rand.reset(Random::CreateDefaultRNG(0));
        for (auto const& rsc : ResourceManager::ListResources())
        {
          if (rsc.m_LoaderName == Archetype::StaticLoaderName())
          {
            m_Archetypes.push_back(rsc);
          }
        }
        m_World.AddTick(World::PostPhysics, [dataRef = m_Data](World& iWorld, float iDelta)
          {
            Engine_Application& app = Engine_Application::GetAppl();
            if (Scenario_Base* scenario = Scenario_Base::DynamicCast(app.GetScenario()))
            {
              StepFullScaleTest(iWorld, iDelta, *scenario->GetMapData().navMesh, 0, *dataRef);
            }
          });
      }
      ~NavigatorBenchPanel()
      {
        Clear();
      }
    private:

      void Clear()
      {
        for (auto obj : m_Data->m_Agents)
        {
          m_World.DeleteObject(obj);
        }
        m_Data->m_Agents.clear();
        m_Data->m_ProbaTable.clear();
      }

      void Display() override
      {
        char const* selRsc("<none>");
        if (m_SelectedArch != -1)
        {
          selRsc = m_Archetypes[m_SelectedArch].m_ResourceName.c_str();
        }
        if (ImGui::BeginCombo("Agent Archetype", selRsc))
        {
          int32_t selected = m_SelectedArch;
          for (int32_t option = 0; option < m_Archetypes.size(); ++option)
          {
            if (ImGui::Selectable(m_Archetypes[option].m_ResourceName.c_str(), m_SelectedArch == option))
            {
              selected = option;
            }
          }
          ImGui::EndCombo();
          m_SelectedArch = selected;
        }

        if (ImGui::Button("Crossing Test") && m_SelectedArch != -1)
        {
          Archetype const* arch = ResourceManager::Load<Archetype>(m_Archetypes[m_SelectedArch].m_ResourceId);
          if (arch != nullptr && arch->HasComponent(EngineCommon::CharacterComponentName()))
          {
            Clear();
            Engine_Application& app = Engine_Application::GetAppl();
            if (Scenario_Base* scenario = Scenario_Base::DynamicCast(app.GetScenario()))
            {
              CharacterSystem::Desc agentDesc;

              ConstDynObject const& obj = arch->GetProperty(EngineCommon::ObjectShapeData::PropertyName());
              if (obj.IsValid())
              {
                agentDesc.size = obj.CastBuffer<EngineCommon::ObjectShapeData>()->ComputeBoundingCircle2DRadius();
              }
              else
              {
                agentDesc.size = 1.0;
              }

              agentDesc.animation = &scenario->GetDefaultAnimation();
              BuildCrossingTest(m_World, *arch, agentDesc, *scenario->GetMapData().navMesh, 0, *m_Data);
            }
          }
        }

        ImGui::DragInt("# of agents", &m_NumAgents);

        if (ImGui::Button("FullScale Test") && m_SelectedArch != -1)
        {
          Archetype const* arch = ResourceManager::Load<Archetype>(m_Archetypes[m_SelectedArch].m_ResourceId);
          if (arch != nullptr && arch->HasComponent(EngineCommon::CharacterComponentName()))
          {
            Clear();
            Engine_Application& app = Engine_Application::GetAppl();
            if (Scenario_Base* scenario = Scenario_Base::DynamicCast(app.GetScenario()))
            {
              CharacterSystem::Desc agentDesc;

              ConstDynObject const& obj = arch->GetProperty(EngineCommon::ObjectShapeData::PropertyName());
              if (obj.IsValid())
              {
                agentDesc.size = obj.CastBuffer<EngineCommon::ObjectShapeData>()->ComputeBoundingCircle2DRadius();
              }
              else
              {
                agentDesc.size = 1.0;
              }

              agentDesc.animation = &scenario->GetDefaultAnimation();
              BuildFullScaleTest(m_World, *arch, agentDesc, m_NumAgents, *scenario->GetMapData().navMesh, 0, *m_Data);
            }
          }
        }
      }
      
      Vector<Resource::Header> m_Archetypes;
      int32_t m_SelectedArch = -1;
#ifdef _DEBUG
      int32_t m_NumAgents = 10;
#else
      int32_t m_NumAgents = 100;
#endif

      World& m_World;
      std::shared_ptr<Data> m_Data;
    };

    void AddNavigatorBenchMenu(MenuManager& iMenus, World& iWorld)
    {
      iMenus.AddMenu("Navigator")
        .AddOpenPanelCommand("Benchmark", [&iWorld]() { return eXl_NEW NavigatorBenchPanel(iWorld); })
        .EndMenu();
    }
  }
}