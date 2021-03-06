/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <engine/common/world.hpp>
#include <engine/common/transforms.hpp>
#include <engine/common/neighbours.hpp>
#include <engine/common/debugtool.hpp>
#include <engine/pathfinding/navmesh.hpp>

namespace RVO
{
  class ORCAAgent;
}

namespace eXl
{
  class KinematicController;
  class Random;
  class VelocityObstacle;

  class EXL_ENGINE_API NavigatorSystem : public ComponentManager
  {
    DECLARE_RTTI(NavigatorSystem, ComponentManager);
  public:

    struct Obstacle;
    struct Agent;

    typedef ObjectTable<Obstacle> Obstacles;
    typedef ObjectTable<Agent> Agents;

    struct Obstacle : public HeapObject
    {
      enum ObstacleKind
      {
        Sphere,
        Box,
      };

      ObjectHandle m_Object;
      Vec3 m_Dims;
      ObstacleKind m_ObsKind;

      Vec3 m_Dir;
      //Vec3 m_LinVel;
      float m_Speed = 0;
      float m_ImmobilityFactor = 0.0;
      float m_ImmTimestamp;

      static uint32_t constexpr s_WindowSize = 8;
      float m_PrevSpeed[s_WindowSize];
      uint32_t m_NumSamples;

      Agents::Handle m_Agent;
    };

    struct Beacon;

    struct Agent
    {
      // public data
      Vec3 m_Dest;
      Vec3 m_CorrectedVelocity;
      
      // private data
      AABB2Df m_CurFace;
      int m_CurPathStep = -1;
      uint32_t m_CurComponent = -1;
      uint32_t m_CurFaceIdx;
      NavMesh::Path m_CurrentPath;

      // Anti-stuck mechanism
      Vec3 m_PriorityAvoidanceDir;
      Vec3 m_PrevPos;
      Vec2 m_TreadmillDir;
      float m_AvoidanceFactor = 0.0;
      float m_AvoidanceDelta = 0.0;
      uint32_t m_Rank = 0;
      uint32_t m_RankTimestamp = 0;
      bool m_HasPriority = false;
      bool m_HasDest = true;
      bool m_EnableAvoidance = true;
      bool m_PriorityAvoidance = false;
      bool m_UnstuckEnabled = false;

      Obstacles::Handle m_Obstacle;
    };

    NavigatorSystem(Transforms& iTransforms);

    void SetNavMesh(NavMesh& iMesh);

    NavMesh* GetNavMesh() { return m_NavMesh; }

    Agent const* AddNavigator(ObjectHandle iObject, float iRadius, float iSpeed, bool iAvoidanceEnabled = true);

    void DeleteComponent(ObjectHandle iObject) override;

    Err SetDestination(ObjectHandle iObject, Vec3 iDest);

    Obstacle const* AddObstacle(ObjectHandle iObject, float iRadius);

    //void SetObstacleSpeed(ObjectHandle iObject, Vec3 iSpeed);

    Obstacle const* AddObstacle(ObjectHandle iObject, Vec3 iBoxDims);

    void Tick(float iTime, NeighborhoodExtraction& iNeighbours);

    Obstacle const* GetObstacle(ObjectHandle iObject) const
    {
      return GetObstacle_Internal(iObject);
    }

    Agent const* GetAgent(ObjectHandle iObject) const
    {
      return GetAgent_Internal(iObject);
    }

    Agents const& GetAgents() const { return m_Agents; }
    Obstacles const& GetObstacles() const { return m_Obstacles; }

    KinematicController& GetController() {return *m_Controller;}

    struct Event
    {
      enum Kind
      {
        DestinationReached
      };

      ObjectHandle m_Object;
      Kind m_EvtKind;
    };

    Vector<Event> DispatchEvents();

  protected:

    void HandleAgentPathfinding(Agent& agent, float iTime);
    void HandleAgentAvoidance(NeighborhoodExtraction& iNeigh, VelocityObstacle& vo, RVO::ORCAAgent&, Agent& agent, float iTime);
    void HandleAgentDiffusion(Agent& agent, float iTime);

    Obstacle* GetObstacle_Internal(ObjectHandle iObject) const;

    Agent* GetAgent_Internal(ObjectHandle iObject) const;

    class NavAgentsController;

    float const m_LookaheadDistance = 4.0;

    KinematicController* m_Controller;

    // Handle to obstacle;
    UnorderedMap<ObjectHandle, Obstacles::Handle> m_ObjectMap;

    Obstacles m_Obstacles;
    Agents m_Agents;
    Vector<Event> m_Events;

    Transforms& m_Transforms;
    NavMesh* m_NavMesh = nullptr;
    Random* m_Rand;

    uint32_t m_Timestamp = 0;
  };
}