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
    Vector2f PickRandomPosInBox(AABB2Df const& iBox, Random& iRand)
    {
      AABB2Df trimmedFace = iBox;
      trimmedFace.m_Data[0] += Vector2f::ONE * 2.0;
      trimmedFace.m_Data[1] -= Vector2f::ONE * 2.0;
      Vector2f size = trimmedFace.GetSize();

      return trimmedFace.m_Data[0] + Vector2f(
        size.X() * (iRand.Generate() % 1000) / 1000.0, 
        size.Y() * (iRand.Generate() % 1000) / 1000.0);
    };

    Vector3f PickRandomPos(NavMesh const& iNavMesh, uint32_t iComponent, Random& iRand)
    {
      uint32_t faceIdx = iRand.Generate() % iNavMesh.GetFaces(iComponent).size();
      AABB2Df curFace = iNavMesh.GetFaces(iComponent)[faceIdx].m_Box;
      Vector2f randPos = PickRandomPosInBox(curFace, iRand);
      return Vector3f(randPos.X(), randPos.Y(), 0.0);
    };

    Vector3f PickRandomDest(Vector3f const& iCurPos, NavMesh const& iNavMesh, uint32_t iComponent, Random& iRand, NavigatorBench::ProbaTable const& iProbaTable)
    {
      auto faceIdx = iNavMesh.FindFace(Vector2f(iCurPos.X(), iCurPos.Y()));
      uint32_t destFaceIdx = faceIdx->m_Face;
      if (iNavMesh.GetFaces(iComponent).size() > 1)
      {
        do
        {
          //destFaceIdx = iRand.Generate() % iNavMesh.GetFaces(iComponent).size();
          float sample = float(iRand.Generate() % 10000) / 10000;
          auto iter = std::lower_bound(iProbaTable.begin(), iProbaTable.end(), std::make_pair(sample, 0u));
          destFaceIdx = iter == iProbaTable.end() ? iProbaTable.back().second : iter->second;
        } while (destFaceIdx == faceIdx->m_Face);
      }

      AABB2Df destFace = iNavMesh.GetFaces(iComponent)[destFaceIdx].m_Box;

      Vector2f randPos = PickRandomPosInBox(destFace, iRand);
      return Vector3f(randPos.X(), randPos.Y(), 0.0);
    };


    Data BuildCrossingTest(World& iWorld, CharacterSystem::Desc& ioBaseDesc, NavMesh const& iNavMesh, uint32_t iComponent)
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
      float perimeter = radius * 2 * Mathf::PI;

      uint32_t numActors = (perimeter / (1.2 * 4));
      float increment = Mathf::PI * 2 / numActors;

      Vector<ObjectHandle> autonomousAgents;
      auto& transforms = *iWorld.GetSystem<Transforms>();
      auto& gfxSys = *iWorld.GetSystem<GfxSystem>();
      auto& phSys = *iWorld.GetSystem<PhysicsSystem>();
      auto& navigator = *iWorld.GetSystem<NavigatorSystem>();

      ioBaseDesc.kind = CharacterSystem::PhysicKind::Simulated;
      ioBaseDesc.controlKind = CharacterSystem::ControlKind::Navigation;
      ioBaseDesc.size = 1.0;
      ioBaseDesc.maxSpeed = 10.0;

      for (unsigned int i = 0; i < numActors / 2; ++i)
      {
        Vector3f curPos(Mathf::Cos(i * increment) * radius + center.X(), Mathf::Sin(i * increment) * radius + center.Y(), 0.0);
        Vector3f destPos(Mathf::Cos(i * increment + Mathf::PI) * radius + center.X(), Mathf::Sin(i * increment + Mathf::PI) * radius + center.Y(), 0.0);

        ObjectHandle truc = CharacterSystem::Build(iWorld, curPos, ioBaseDesc);
        navigator.SetDestination(truc, destPos);

        ObjectHandle truc2 = CharacterSystem::Build(iWorld, destPos, ioBaseDesc);
        navigator.SetDestination(truc2, curPos);

        autonomousAgents.push_back(truc);
        autonomousAgents.push_back(truc2);
      }

      Data outData;
      outData.m_Agents = std::move(autonomousAgents);
      outData.m_Component = iComponent;

      return outData;
    }

    Data NavigatorBench::BuildFullScaleTest(World& iWorld, CharacterSystem::Desc& ioBaseDesc, uint32_t iNumNavAgents, NavMesh const& iNavMesh, uint32_t iComponent, Random& iRand)
    {
      Vector<ObjectHandle> autonomousAgents;
      auto& transforms = *iWorld.GetSystem<Transforms>();
      auto& gfxSys = *iWorld.GetSystem<GfxSystem>();
      auto& phSys = *iWorld.GetSystem<PhysicsSystem>();
      auto& navigator = *iWorld.GetSystem<NavigatorSystem>();

      ioBaseDesc.kind = CharacterSystem::PhysicKind::Simulated;
      ioBaseDesc.controlKind = CharacterSystem::ControlKind::Navigation;
      ioBaseDesc.size = 1.0;
      ioBaseDesc.maxSpeed = 10.0;

      ProbaTable probaTable;
      float totalArea = 0;
      for (uint32_t i = 0; i< iNavMesh.GetFaces(iComponent).size(); ++i)
      {
        auto const& Face = iNavMesh.GetFaces(iComponent)[i];
        Vector2f size = Face.m_Box.GetSize();
        float area = size.X() * size.Y();
        totalArea += area;
        probaTable.push_back(std::make_pair(area, i));
      }

      std::sort(probaTable.begin(), probaTable.end());

      float accum = 0;
      for (auto& entry : probaTable)
      {
        if (entry.first < 36)
        {
          totalArea -= entry.first;
          entry.first = 0;
          continue;
        }
        float curProba = accum / totalArea;
        accum += entry.first;
        entry.first = curProba;
      }

      for (unsigned int i = 0; i < iNumNavAgents; ++i)
      {
        //auto curPos = PickRandomPos(iNavMesh, iComponent, iRand);

        uint32_t faceIdx = iRand.Generate() % iNavMesh.GetFaces(iComponent).size();
        AABB2Df curFace = iNavMesh.GetFaces(iComponent)[faceIdx].m_Box;
        Vector2f randPos = PickRandomPosInBox(curFace, iRand);
        auto curPos = Vector3f(randPos.X(), randPos.Y(), 0.0);

        auto destPos = PickRandomDest(curPos, iNavMesh, iComponent, iRand, probaTable);
      
        ObjectHandle truc = CharacterSystem::Build(iWorld, curPos, ioBaseDesc);
        navigator.SetDestination(truc, destPos);
        autonomousAgents.push_back(truc);
      }

      Data outData;
      outData.m_Agents = std::move(autonomousAgents);
      outData.m_Component = iComponent;
      outData.m_ProbaTable = std::move(probaTable);

      return outData;
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

    void StepFullScaleTest(World& world, float iDelta, NavMesh const& iNavMesh, uint32_t iComponent, Random& iRand, Data& iData)
    {
      auto& transforms = *world.GetSystem<Transforms>();
      auto& navigator = *world.GetSystem<NavigatorSystem>();
      for (auto evt : navigator.DispatchEvents())
      {
        Matrix4f transform = transforms.GetWorldTransform(evt.m_Object);
        Vector3f const& curPos = *reinterpret_cast<Vector3f*>(transform.m_Data + 12);
        navigator.SetDestination(evt.m_Object, PickRandomDest(curPos, iNavMesh, iComponent, iRand, iData.m_ProbaTable));
      }
    }
  }
}

#include <engine/common/app.hpp>
#include <engine/common/menumanager.hpp>
#include <engine/game/scenariobase.hpp>
#include <imgui.h>

namespace eXl
{
  namespace NavigatorBench
  {
    class NavigatorBenchPanel : public MenuManager::Panel
    {
    public:
      NavigatorBenchPanel(World& iWorld) : m_World(iWorld)
      {

      }
    private:

      void Clear()
      {
        for (auto obj : m_Data.m_Agents)
        {
          m_World.DeleteObject(obj);
        }
        m_Data.m_Agents.clear();
        m_Data.m_ProbaTable.clear();
      }

      void Display() override
      {
        if (ImGui::Button("Crossing Test"))
        {
          Clear();
          Engine_Application& app = Engine_Application::GetAppl();
          Scenario* scenario = app.GetScenario();
          BuildCrossingTest(m_World, );
        }
      }

      World& m_World;
      Data m_Data;
    };
  }
}