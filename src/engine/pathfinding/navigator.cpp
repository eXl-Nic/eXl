#include <engine/pathfinding/navigator.hpp>
#include <engine/common/debugtool.hpp>

#include <core/log.hpp>

#include <engine/physics/physicsys.hpp>
#include <engine/physics/physiccomponent.hpp>
#include <math/mathtools.hpp>
//#include <engine/pathfinding/penumbratools.hpp>
#include <engine/pathfinding/velocityobstacle.hpp>
#include <engine/game/commondef.hpp>
#include "ORCA/ORCAAgent.hpp"

#include <core/random.hpp>

//#pragma optimize("", off)

namespace eXl
{
  IMPLEMENT_RTTI(NavigatorSystem);

  namespace
  {
    bool TestBoxSegmentExcept(AABB2Df const& iBox, Segmentf const& iSeg, Segmentf const& iSegToExclude, float iTrim)
    {
      // Trim box for allowance.
      AABB2Df trimmedBox = iBox;
      trimmedBox.m_Data[0] += Vector2f::ONE * iTrim;
      trimmedBox.m_Data[1] -= Vector2f::ONE * iTrim;

      Segmentf segs[] = {
        {iBox.m_Data[0], Vector2f(iBox.m_Data[0].X(), iBox.m_Data[1].Y())},
        {iBox.m_Data[0], Vector2f(iBox.m_Data[1].X(), iBox.m_Data[0].Y())},
        {iBox.m_Data[1], Vector2f(iBox.m_Data[0].X(), iBox.m_Data[1].Y())},
        {iBox.m_Data[1], Vector2f(iBox.m_Data[1].X(), iBox.m_Data[0].Y())}
      };

      for(auto seg : segs)
      {
        Vector2f segDir = seg.m_Ext2 - seg.m_Ext1;
        float segLength = segDir.Normalize();

        Vector2f interPt;
        auto intersectRes = iSeg.Intersect(seg, interPt);
        if(intersectRes & Segmentf::ConfoundSegments)
        {
          return true;
        }
        if((intersectRes & Segmentf::PointOnSegments) == Segmentf::PointOnSegments)
        {
          if(!iSegToExclude.IsOnSegment(interPt))
          {
            return true;
          }
        }
      }

      return false;
    }
  }

  struct NavigatorSystem::Beacon
  {
    bool IsActive() { return m_Holder.IsAssigned(); }

    enum Influence
    {
      None,
      Aligned,
      Opposed
    };

    void Claim(Vector2f const& iPos, Vector2f const& iDir)
    {
      m_Position = iPos;
      m_Direction = iDir;
      m_Radius = 5.0;
    }

    void Update(Vector2f const& iPos, float iGrowth)
    {
      m_Position = iPos;
      m_Radius *= iGrowth;
    }

    Influence Test(Vector2f const& iPos, Vector2f const& iDir)
    {
      float const threshold = 1.0 / Mathf::Sqrt(2.0);
      float cone = iDir.Dot(m_Direction);
      Influence res = None;
      if (cone > threshold)
      {
        res = Aligned;
      }
      else if(cone < -threshold)
      {
        res = Opposed;
      }

      if (res != None)
      {
        if ((iPos - m_Position).SquaredLength() > m_Radius * m_Radius)
        {
          return None;
        }
      }

      return res;
    }

    Obstacles::Handle m_Holder;
    Vector2f m_Position;
    Vector2f m_Direction;
    uint32_t m_Idx;
    float m_Radius;
  };

  class NavigatorSystem::NavAgentsController : public KinematicController
  {
  public:

    NavAgentsController(NavigatorSystem& iNav)
      : m_Nav(iNav)
    {

    }

  protected:

    NavigatorSystem& m_Nav;

    void Step(PhysicsSystem* iSys, float iTime) override
    {
      GameDataView<Vector3f>* velocities = DunAtk::GetVelocities(m_Nav.GetWorld());
      m_Nav.Tick(iTime, iSys->GetNeighborhoodExtraction());
      m_Nav.GetAgents().Iterate([&](Agent& agent, Agents::Handle)
      {
        Obstacle& agentObs = m_Nav.m_Obstacles.Get(agent.m_Obstacle);
        if(auto physComp = iSys->GetCompImpl(agentObs.m_Object))
        {
          if(agent.m_CorrectedVelocity != Vector3f::ZERO)
          {
            Vector3f curPos = GetPosition(*physComp);
            //float distance = Mathf::Max((curPos - agent.m_PrevPos).Dot(agentObs.m_Dir), 0.0);
            float distance = (curPos - agent.m_PrevPos).Dot(agentObs.m_Dir);

            //agentObs.m_ImmobilityFactor *= pow(0.01, iTime);
            //agentObs.m_ImmobilityFactor += distance / iTime;
            uint32_t nextEntry = agentObs.m_NumSamples % Obstacle::s_WindowSize;
            // -> distance / iTime, not exactly "prev speed" at first glance
            // but we cannot know the actual speed until the ph engine stepped, and we have a fixed time step.
            agentObs.m_PrevSpeed[nextEntry] = distance / iTime;
            agentObs.m_NumSamples++;
            if (agentObs.m_NumSamples < Obstacle::s_WindowSize)
            {
              agentObs.m_ImmobilityFactor = agentObs.m_Speed;
            }
            else
            {
              float score = 0.0;
              for (auto speed : agentObs.m_PrevSpeed)
              {
                score += speed;
              }
              score /= Obstacle::s_WindowSize;
              agentObs.m_ImmobilityFactor = score;
            }

            Vector3f linVel;
            bool nearDest = false;

            if(agent.m_HasDest)
            {
              Vector3f destDir = agent.m_Dest - curPos;
              destDir.Z() = 0;
              float distDest = destDir.Normalize();

              Vector3f linVel = agentObs.m_Dir * agentObs.m_Speed;

              if (distDest < Mathf::ZERO_TOLERANCE
                || agentObs.m_Speed * iTime > distDest)
              {
                nearDest = true;
                linVel = (destDir * distDest) / iTime;

                //m_CurrentMesh = nullptr;
                agent.m_HasDest = false;
                agent.m_CurrentPath.m_Edges.clear();
                agent.m_CurrentPath.m_EdgeDirs.clear();
                agent.m_CurFace = AABB2Df();
                agentObs.m_Dir = Vector3f::ZERO;
                agent.m_CorrectedVelocity = Vector3f::ZERO;

                velocities->GetOrCreate(agentObs.m_Object) = Vector3f::ZERO;

                NavigatorSystem::Event reachedEvt;
                reachedEvt.m_EvtKind = Event::DestinationReached;
                reachedEvt.m_Object = agentObs.m_Object;

                m_Nav.m_Events.push_back(reachedEvt);
              }
            }
            
            if(!nearDest)
            {
              if(agent.m_CorrectedVelocity == Vector3f::ZERO)
              {
                linVel = Vector3f::ZERO;
              }
              else
              {
                Vector3f prevVel = velocities->GetOrCreate(agentObs.m_Object);
                //prevDir.Normalize();
                //float dotFactor = agent->m_CorrectedVelocity.Dot(agent->m_Dir);
                //if(dotFactor < 0.0)
                //{
                //  linVel = (prevVel + agent->m_CorrectedVelocity) * 0.5;
                //  //linVel.Normalize();
                //  //linVel *= agent->m_Speed * (1.0 + dotFactor);
                //}
                //else
                {
                  //linVel = (agent->m_CorrectedVelocity + prevVel) * 0.5;
                  linVel = agent.m_CorrectedVelocity;
                  float vel = linVel.Normalize();
                  linVel *= Mathf::Min(agentObs.m_Speed, vel);
                }
              }

              //agentObs.m_LinVel = linVel;
            }

            velocities->GetOrCreate(agentObs.m_Object) = linVel;
            ApplyLinearVelocity(*physComp, linVel, iTime);
            agent.m_PrevPos = curPos;
          }
          else
          {
            velocities->GetOrCreate(agentObs.m_Object) = Vector3f::ZERO;
            ApplyLinearVelocity(*physComp, Vector3f::ZERO, iTime);
          }
        }
      });
    }
  };

  NavigatorSystem::NavigatorSystem(Transforms& iTransforms)
    : m_Transforms(iTransforms)
    , m_Controller(new NavAgentsController(*this))
    , m_Rand(Random::CreateDefaultRNG(0))
  {
    
  }

  void NavigatorSystem::SetNavMesh(NavMesh& iMesh)
  {
    m_NavMesh = &iMesh;
  }

  NavigatorSystem::Agent const* NavigatorSystem::AddNavigator(ObjectHandle iObject, float iRadius, float iSpeed, bool iAvoidanceEnabled)
  {
    if (GetWorld().IsObjectValid(iObject))
    {
      auto agentHandle = m_Agents.Alloc();
      auto obstacleHandle = m_Obstacles.Alloc();

      Agent* newAgent = m_Agents.TryGet(agentHandle);
      Obstacle* newObstacle = m_Obstacles.TryGet(obstacleHandle);
      newObstacle->m_Object = iObject;
      newObstacle->m_ObsKind = Obstacle::Sphere;
      newObstacle->m_Dims.m_X = iRadius;
      newObstacle->m_Speed = iSpeed;
      newAgent->m_HasDest = false;
      //newAgent->m_AgentIdx = m_Agents.size();
      newAgent->m_EnableAvoidance = iAvoidanceEnabled;
      
      //auto& newObs = m_Obstacles.Get(obstacleHandle);
      //newObs = *newAgent;
      newObstacle->m_Agent = agentHandle;
      newAgent->m_Obstacle = obstacleHandle;

      m_ObjectMap.insert(std::make_pair(iObject, obstacleHandle));

      ComponentManager::CreateComponent(iObject);

      return newAgent;
    }

    return nullptr;
  }

  void NavigatorSystem::DeleteComponent(ObjectHandle iObject)
  {
    auto iter = m_ObjectMap.find(iObject);
    if(iter != m_ObjectMap.end())
    {
      Obstacle& comp = m_Obstacles.Get(iter->second);
      if (comp.m_Agent.IsAssigned())
      {
        m_Agents.Release(comp.m_Agent);
      }
      m_Obstacles.Release(iter->second);

      ComponentManager::DeleteComponent(iObject);
    }
  }

  NavigatorSystem::Obstacle const* NavigatorSystem::AddObstacle(ObjectHandle iObject, float iRadius)
  {
    if (GetWorld().IsObjectValid(iObject))
    {
      auto obstacleHandle = m_Obstacles.Alloc();

      Obstacle* newObstacle = m_Obstacles.TryGet(obstacleHandle);
      newObstacle->m_Object = iObject;
      newObstacle->m_ObsKind = Obstacle::Sphere;
      newObstacle->m_Dims.m_X = iRadius;

      m_ObjectMap.insert(std::make_pair(iObject, obstacleHandle));

      ComponentManager::CreateComponent(iObject);

      return newObstacle;
    }
    return nullptr;
  }

  //void NavigatorSystem::SetObstacleSpeed(ObjectHandle iObject, Vector3f iSpeed)
  //{
  //  if (Obstacle* obstacle = GetObstacle_Internal(iObject))
  //  {
  //    obstacle->m_LinVel = iSpeed;
  //    obstacle->m_Dir = iSpeed;
  //    obstacle->m_Speed = obstacle->m_Dir.Normalize();
  //  }
  //}

  NavigatorSystem::Obstacle* NavigatorSystem::GetObstacle_Internal(ObjectHandle iObject) const
  {
    if(GetWorld().IsObjectValid(iObject))
    {
      auto objEntry = m_ObjectMap.find(iObject);
      if(objEntry != m_ObjectMap.end())
      {
        return m_Obstacles.TryGet(objEntry->second);
      }
    }

    return nullptr;
  }

  NavigatorSystem::Agent* NavigatorSystem::GetAgent_Internal(ObjectHandle iObject) const
  {
    if(auto obs = GetObstacle_Internal(iObject))
    {
      if(obs->m_Agent.IsAssigned())
      {
        return m_Agents.TryGet(obs->m_Agent);
      }
    }

    return nullptr;
  }

  Err NavigatorSystem::SetDestination(ObjectHandle iObject, Vector3f iDest)
  {
    if(auto agent = GetAgent_Internal(iObject))
    {
      auto const& transform = m_Transforms.GetWorldTransform(iObject);

      Vector2f const& curPos2D = *reinterpret_cast<Vector2f const*>(transform.m_Data + 12);
      Vector2f destPos2D(iDest.X(), iDest.Y());

      if(auto path = m_NavMesh->FindPath(curPos2D, destPos2D))
      {
        agent->m_HasDest = true;
        agent->m_Dest = iDest;
        agent->m_Dest.Z() = 0;
        agent->m_CurrentPath = std::move(*path);
        agent->m_CurPathStep = agent->m_CurrentPath.m_Edges.size() - 1;
        return Err::Success;
      }
    }
    return Err::Failure;
  }

  Vector<NavigatorSystem::Event> NavigatorSystem::DispatchEvents()
  {
    return std::move(m_Events);
  }

//#define USE_ORCA

  void NavigatorSystem::Tick(float iTime, NeighborhoodExtraction& iNeigh)
  {
    VelocityObstacle vo(*m_Rand);
    RVO::ORCAAgent orca;

    m_Agents.Iterate([&](Agent& agent, Agents::Handle)
    {
      if (!(agent.m_HasDest || agent.m_EnableAvoidance))
      {
        return;
      }

      Obstacle& agentObs = m_Obstacles.Get(agent.m_Obstacle);
      auto const& transform = m_Transforms.GetWorldTransform(agentObs.m_Object);
      Vector3f const& curPos = *reinterpret_cast<Vector3f const*>(transform.m_Data + 12);

      Vector2f curPos2D(curPos.X(), curPos.Y());

      if (!agent.m_CurFace.Contains(curPos2D))
      {
        if (auto faceIdx = m_NavMesh->FindFace(curPos2D))
        {
          agent.m_CurComponent = faceIdx->m_Component;
          agent.m_CurFaceIdx = faceIdx->m_Face;
          agent.m_CurFace = m_NavMesh->GetFaces(faceIdx->m_Component)[faceIdx->m_Face].m_Box;
        }
        //else
        //{
        //  eXl_ASSERT(false);
        //  // path invalid
        //  return;
        //}
      }

      agent.m_TreadmillDir = GetNavMesh()->GetFaces(agent.m_CurComponent)[agent.m_CurFaceIdx].GetTreadmillDir(curPos2D);
      agent.m_TreadmillDir *= m_Timestamp % 10000 > 5000 ? -1.0 : 1.0;
    });

    m_Agents.Iterate([this, iTime](Agent& agent, Agents::Handle)
    {
      HandleAgentPathfinding(agent, iTime);
    });

    m_Agents.Iterate([this, &iNeigh, iTime, &vo, &orca](Agent& agent, Agents::Handle)
    {
      HandleAgentAvoidance(iNeigh, vo, orca, agent, iTime);
    });

    m_Timestamp++;
    if (m_Timestamp == 0)
    {
      m_Agents.Iterate([](Agent& agent, Agents::Handle)
      {
        agent.m_RankTimestamp = 0;
      });
      ++m_Timestamp;
    }

    m_Agents.Iterate([this, iTime](Agent& agent, Agents::Handle)
    {
      HandleAgentDiffusion(agent, iTime);
    });
  }

  void NavigatorSystem::HandleAgentPathfinding(Agent& agent, float iTime)
  {
    if (!(agent.m_HasDest || agent.m_EnableAvoidance))
    {
      return;
    }

    Obstacle& agentObs = m_Obstacles.Get(agent.m_Obstacle);

    auto debugContext = DebugTool::BeginDebug(DebugTool::Nav_Pathfinding, agentObs.m_Object);

    auto const& transform = m_Transforms.GetWorldTransform(agentObs.m_Object);
    Vector3f const& curPos = *reinterpret_cast<Vector3f const*>(transform.m_Data + 12);

    Vector2f curPos2D(curPos.X(), curPos.Y());

    float const agentRadius = agentObs.m_Dims.X();

    if (agent.m_HasDest)
    {
      Vector3f destDir = agent.m_Dest - curPos;
      destDir.Z() = 0;
      float distDest = destDir.Normalize();

      auto repath = [&]() -> NavMesh::Edge const*
      {
        if (auto path = m_NavMesh->FindPath(curPos2D, Vector2f(agent.m_Dest.X(), agent.m_Dest.Y())))
        {
          LOG_WARNING << "Replanned actor " << agentObs.m_Object.GetId() << "'s path" << "\n";

          agent.m_CurrentPath = std::move(*path);
          agent.m_CurPathStep = agent.m_CurrentPath.m_Edges.size() - 1;
          if (agent.m_CurPathStep >= 0)
          {
            return &m_NavMesh->GetEdges(agent.m_CurComponent)[agent.m_CurrentPath.m_Edges[agent.m_CurPathStep]];
          }
          else
          {
            LOG_WARNING << "Could not find a path for actor " << agentObs.m_Object.GetId() << "\n";

            return nullptr;
          }
        }
        else
        {
          LOG_WARNING << "Could not find a path for actor " << agentObs.m_Object.GetId() << "\n";

          return nullptr;
        }
      };

      if (agent.m_CurPathStep < 0)
      {
        if (!agent.m_CurFace.Contains(MathTools::As2DVec(agent.m_Dest)))
        {
          repath();
        }
        else
        {
          agentObs.m_Dir = destDir;
        }
      }

      if (agent.m_CurPathStep >= 0)
      {
        uint32_t numIter = 0;

        while (agent.m_CurPathStep >= 0 && numIter < 50)
        {
          ++numIter;

          auto const* targetDesc = &m_NavMesh->GetEdges(agent.m_CurComponent)[agent.m_CurrentPath.m_Edges[agent.m_CurPathStep]];

          if (!(targetDesc->face1 == agent.m_CurFaceIdx || targetDesc->face2 == agent.m_CurFaceIdx))
          {
            int curStep = -1;
            //Try to find the step, if we are jittering between faces
            for (int i = agent.m_CurrentPath.m_Edges.size() - 1; i > 0; --i)
            {
              auto const& edgeSrc = m_NavMesh->GetEdges(agent.m_CurComponent)[agent.m_CurrentPath.m_Edges[i]];
              auto const& edgeDst = m_NavMesh->GetEdges(agent.m_CurComponent)[agent.m_CurrentPath.m_Edges[i - 1]];
              auto faceToTraverse = edgeSrc.CommonFace(edgeDst);
              if (faceToTraverse && *faceToTraverse == agent.m_CurFaceIdx)
              {
                curStep = i;
                break;
              }
            }

            if (curStep != -1)
            {
              agent.m_CurPathStep = curStep;
              targetDesc = &m_NavMesh->GetEdges(agent.m_CurComponent)[agent.m_CurrentPath.m_Edges[agent.m_CurPathStep]];
            }
            else
            {
              targetDesc = repath();
              if (!targetDesc)
              {
                continue;
              }
            }
          }

          unsigned int targetFaceIdx = targetDesc->face1 == agent.m_CurFaceIdx ? targetDesc->face2 : targetDesc->face1;

          Segmentf targetSeg = targetDesc->segment;

          Vector2f segDir = targetSeg.m_Ext2 - targetSeg.m_Ext1;
          float segLength = segDir.Normalize();

          // Trim segment for allowance.
          targetSeg.m_Ext1 += segDir * agentRadius;
          targetSeg.m_Ext2 -= segDir * agentRadius;

          Vector2f oDir;
          float distance = targetSeg.NearestPointSeg(curPos2D, oDir);

          float dirIndicator = oDir.Dot(agent.m_CurrentPath.m_EdgeDirs[agent.m_CurPathStep]);

          if (Mathf::Abs(dirIndicator) > Mathf::ZERO_TOLERANCE && dirIndicator < 0.0)
          {
            //Going backward !! pop path item.
            agent.m_CurPathStep--;
          }
          else
          {

            Vector2f pointToReach = curPos2D + oDir * distance;

            //Aiming for the point after the segment to traverse, in the other face
            pointToReach += agent.m_CurrentPath.m_EdgeDirs[agent.m_CurPathStep] * agentRadius;

            Segmentf path = { curPos2D, pointToReach };

            if (TestBoxSegmentExcept(agent.m_CurFace, path, targetSeg, agentRadius)
              || TestBoxSegmentExcept(m_NavMesh->GetFaces(agent.m_CurComponent)[targetFaceIdx].m_Box, path, targetSeg, agentRadius))
            {
              //Aiming for the point before the segment to traverse, in the current face
              pointToReach -= agent.m_CurrentPath.m_EdgeDirs[agent.m_CurPathStep] * 2 * agentRadius;


              // Check if we are cutting corners
              {
                AABB2Df trimmedBox = agent.m_CurFace;
                trimmedBox.m_Data[0] += Vector2f::ONE * agentRadius;
                trimmedBox.m_Data[1] -= Vector2f::ONE * agentRadius;

                if (!trimmedBox.Contains(curPos2D))
                {
                  Segmentf segs[] = {
                    {trimmedBox.m_Data[0], Vector2f(trimmedBox.m_Data[0].X(), trimmedBox.m_Data[1].Y())},
                    {trimmedBox.m_Data[0], Vector2f(trimmedBox.m_Data[1].X(), trimmedBox.m_Data[0].Y())},
                    {trimmedBox.m_Data[1], Vector2f(trimmedBox.m_Data[0].X(), trimmedBox.m_Data[1].Y())},
                    {trimmedBox.m_Data[1], Vector2f(trimmedBox.m_Data[1].X(), trimmedBox.m_Data[0].Y())}
                  };

                  Vector2f dirToPtIn;
                  float distToPtIn = segs[0].NearestPointSeg(curPos2D, dirToPtIn);
                  for (int seg = 1; seg < 4; ++seg)
                  {
                    Vector2f potDir;
                    float otherDist = segs[seg].NearestPointSeg(curPos2D, potDir);
                    if (otherDist < distToPtIn)
                    {
                      dirToPtIn = potDir;
                      distToPtIn = otherDist;
                    }
                  }
                  if (distToPtIn > Mathf::ZERO_TOLERANCE)
                  {
                    pointToReach = curPos2D + dirToPtIn * distToPtIn;
                  }
                }
              }
            }

            Vector2f dir = pointToReach - curPos2D;
            dir.Normalize();

            agentObs.m_Dir = Vector3f(dir.X(), dir.Y(), 0.0);

            break;
          }
        }
      }
    }
    else
    {
      //agentObs.m_Dir = Vector3f::ZERO;
      //agentObs.m_Dir = MathTools::To3DVec(agent.m_TreadmillDir);
    }

    Vector2f agentDir2D = MathTools::As2DVec(agentObs.m_Dir);

    float treadmillFactor = agentDir2D.Dot(agent.m_TreadmillDir);
    agent.m_HasPriority = treadmillFactor > 1.0 / Mathf::Sqrt(2.0);
  }

  void NavigatorSystem::HandleAgentAvoidance(NeighborhoodExtraction& iNeigh, VelocityObstacle& vo, RVO::ORCAAgent&, Agent& agent, float iTime)
  {
    if (!(agent.m_HasDest || agent.m_EnableAvoidance))
    {
      return;
    }

    Obstacle& agentObs = m_Obstacles.Get(agent.m_Obstacle);

    auto debugContext = DebugTool::BeginDebug(DebugTool::Nav_Avoidance, agentObs.m_Object);

    auto const& transform = m_Transforms.GetWorldTransform(agentObs.m_Object);
    Vector3f const& curPos = *reinterpret_cast<Vector3f const*>(transform.m_Data + 12);

    Vector2f curPos2D(curPos.X(), curPos.Y());

    float avoidanceToTransmit = 0.0;
    if (agent.m_AvoidanceFactor > 0.5)
    {
      avoidanceToTransmit = agent.m_AvoidanceFactor - 0.5;
      agent.m_AvoidanceFactor = 0.5 + Mathf::EPSILON;
      avoidanceToTransmit = Mathf::Min(1.0, avoidanceToTransmit);
    }

    if (agent.m_UnstuckEnabled)
    {
      agent.m_Rank = 0;
      agent.m_RankTimestamp = m_Timestamp;
    }

    float const agentRadius = agentObs.m_Dims.X();

    Vector2f agentDir2D = MathTools::As2DVec(agentObs.m_Dir);

    if (!agent.m_EnableAvoidance)
    {
      agent.m_CorrectedVelocity = agentObs.m_Dir * agentObs.m_Speed;
    }
    else
    {
      auto foundNeighEntry = iNeigh.GetObjects().find(agentObs.m_Object);

      uint32_t const regularAttempt = 0;
      uint32_t const priorityAvoidanceAttempt = 1;
      uint32_t const numAttempts = agent.m_PriorityAvoidance ? 2 : 1;

      for (uint32_t attempt = 0; attempt < numAttempts; ++attempt)
      {
#ifndef USE_ORCA
        vo.Start(&agent, curPos2D, agentRadius, agent.m_PriorityAvoidance ? Vector2f::ZERO : agentDir2D, agentObs.m_Speed);
#else
        orca.agentNeighbors_.clear();
        orca.obstacleNeighbors_.clear();
        orca.position_ = curPos2D;
        orca.maxSpeed_ = agentObs.m_Speed;
        orca.radius_ = agentRadius;
        orca.prefVelocity_ = agentDir2D * agentObs.m_Speed;
        orca.timeHorizonObst_ = 1.0f;
        orca.timeHorizon_ = 0.5f;
#endif
        if (0 && agent.m_PriorityAvoidance)
        {
          Segmentf treadMillSeg;
          treadMillSeg.m_Ext1 = curPos2D - agent.m_TreadmillDir * agentObs.m_Dims.X() + MathTools::GetPerp(agent.m_TreadmillDir) * agentObs.m_Speed;
          treadMillSeg.m_Ext2 = curPos2D - agent.m_TreadmillDir * agentObs.m_Dims.X() - MathTools::GetPerp(agent.m_TreadmillDir) * agentObs.m_Speed;
          vo.AddSegment(treadMillSeg, 0.0);
        }

        auto const& face = m_NavMesh->GetFaces(agent.m_CurComponent)[agent.m_CurFaceIdx];

        if (debugContext)
        {
          vo.DrawDebug();
        }

        for (auto const& wall : face.m_Walls)
        {
          Vector2f segDir = wall.m_Ext2 - wall.m_Ext1;
          float segLength = segDir.Normalize();

          Vector2f pt1Dir = wall.m_Ext1 - curPos2D;
          Vector2f ptProjDir;
          float pt1DirProj = pt1Dir.Dot(segDir);
          if (pt1DirProj > Mathd::ZERO_TOLERANCE)
          {
            ptProjDir = pt1Dir;
          }
          else if (-pt1DirProj > (segLength - Mathd::ZERO_TOLERANCE))
          {
            ptProjDir = wall.m_Ext2 - curPos2D;
          }
          else
          {
            ptProjDir = pt1Dir - (pt1DirProj * segDir);
          }

          float dist = ptProjDir.Normalize();

          if (dist < m_LookaheadDistance)
          {
            float remainingDist = agentObs.m_Speed/*m_LookaheadDistance * Mathf::Sqrt(1.0 - dist*dist / (m_LookaheadDistance * m_LookaheadDistance))*/;

            Segmentf correctedSegment;
            correctedSegment.m_Ext1 = curPos2D + ptProjDir * dist - segDir * Mathf::Min(remainingDist, Mathf::Max(-pt1DirProj, 0.0));
            correctedSegment.m_Ext2 = curPos2D + ptProjDir * dist + segDir * Mathf::Min(remainingDist, Mathf::Max(pt1DirProj + segLength, 0.0));

            if (debugContext)
            {
              auto drawer = DebugTool::GetDrawer();
              drawer->DrawLine(MathTools::To3DVec(correctedSegment.m_Ext1), MathTools::To3DVec(correctedSegment.m_Ext2), Vector4f(1.0, 1.0, 0.0, 1.0));
            }
#ifndef USE_ORCA
            vo.AddSegment(correctedSegment, 0.0);
#else
            orca.insertObstacleNeighbor(wall, dist * dist);
#endif
          }
        }

        GameDataView<Vector3f>* velocities = DunAtk::GetVelocities(GetWorld());
        Vector3f agentLinVel = velocities->GetOrCreate(agentObs.m_Object);
        if (foundNeighEntry != iNeigh.GetObjects().end())
        {
          auto const& neighbours = iNeigh.GetNeigh()[foundNeighEntry->second];

          if (agent.m_UnstuckEnabled
            || (avoidanceToTransmit > 0.1 /*&& agent.m_PriorityAvoidanceDir != Vector3f::ZERO*/))
          {
            Vector2f dir = MathTools::As2DVec(agent.m_UnstuckEnabled
              ? agentObs.m_Dir
              : (agent.m_PriorityAvoidanceDir != Vector3f::ZERO ? agent.m_PriorityAvoidanceDir : agentLinVel));
            dir.Normalize();

            for (uint32_t neighNum = 0; neighNum < neighbours.numNeigh; ++neighNum)
            {
              ObjectHandle otherObj = iNeigh.GetHandles()[neighbours.neighbors[neighNum]];
              auto objEntry = m_ObjectMap.find(otherObj);

              if (Obstacle* otherObs = objEntry != m_ObjectMap.end() ? m_Obstacles.TryGet(objEntry->second) : nullptr)
              {
                if (otherObs->m_Agent.IsAssigned())
                {
                  if (Agent* otherAgent = m_Agents.TryGet(otherObs->m_Agent))
                  {
                    if (otherAgent->m_PriorityAvoidance
                      && otherAgent->m_RankTimestamp < agent.m_RankTimestamp)
                    {
                      otherAgent->m_Rank = agent.m_Rank + 1;
                      otherAgent->m_RankTimestamp = agent.m_RankTimestamp;
                    }

                    auto const& otherObjTrans = m_Transforms.GetWorldTransform(otherObj);
                    Vector3f const& otherPos = *reinterpret_cast<Vector3f const*>(otherObjTrans.m_Data + 12);
                    Vector2f otherPos2D(otherPos.X(), otherPos.Y());
                    Vector2f dirOther = (otherPos2D - curPos2D);
                    dirOther.Normalize();

                    bool isInCone = dirOther.Dot(dir) > 0.7;

                    if (isInCone 
                      && agent.m_UnstuckEnabled 
                      &&otherAgent->m_HasPriority)
                    {
                      otherAgent->m_UnstuckEnabled = true;
                    }

                    if (!otherAgent->m_HasPriority
                      && (!otherAgent->m_PriorityAvoidance
                        || otherAgent->m_Rank > agent.m_Rank))
                    {
                      float amountToTransmit = (agent.m_UnstuckEnabled ? 1.0 : avoidanceToTransmit);
                      if (!isInCone)
                      {
                        amountToTransmit *= 0.01;
                      }
                      otherAgent->m_AvoidanceDelta += amountToTransmit;
                    }
                  }
                }
              }
            }
          }

          Vector2f avoidanceVector;
          for (uint32_t neighNum = 0; neighNum < neighbours.numNeigh; ++neighNum)
          {
            ObjectHandle otherObj = iNeigh.GetHandles()[neighbours.neighbors[neighNum]];
            auto objEntry = m_ObjectMap.find(otherObj);

            if (Obstacle* otherObs = objEntry != m_ObjectMap.end() ? m_Obstacles.TryGet(objEntry->second) : nullptr)
            {
              auto const& otherObjTrans = m_Transforms.GetWorldTransform(otherObj);
              Vector3f const& otherPos = *reinterpret_cast<Vector3f const*>(otherObjTrans.m_Data + 12);
              Vector2f otherPos2D(otherPos.X(), otherPos.Y());

              Vector2f obsDir2D(otherObs->m_Dir.X(), otherObs->m_Dir.Y());

              if (otherObs->m_ObsKind == Obstacle::Sphere)
              {
                
                Vector2f otherVel = MathTools::As2DVec(velocities->GetOrCreate(otherObs->m_Object));

#ifndef USE_ORCA
                if (otherObs->m_Agent.IsAssigned())
                {
                  if (Agent* otherAgent = m_Agents.TryGet(otherObs->m_Agent))
                  {
                    if (attempt == priorityAvoidanceAttempt
                      && !otherAgent->m_UnstuckEnabled
                      && !otherAgent->m_HasPriority)
                    {
                      if (otherAgent->m_PriorityAvoidance
                        //&& otherAgent->m_PriorityAvoidanceDir != Vector3f::ZERO
                        )
                      {
                        //Vector2f testDir = MathTools::As2DVec(otherAgent->m_PriorityAvoidanceDir != Vector3f::ZERO 
                        //  ? otherAgent->m_PriorityAvoidanceDir
                        //  : otherObs->m_LinVel);
                        //Vector2f dirOther = (otherPos2D - curPos2D);
                        //dirOther.Normalize();
                        //if (dirOther.Dot(testDir) < 0.7)
                        if (otherAgent->m_Rank >= agent.m_Rank)
                        {
                          continue;
                        }
                      }
                      else
                      {
                        continue;
                      }
                    }

                    if (otherAgent->m_EnableAvoidance)
                    {
                      if (agent.m_PriorityAvoidance)
                      {
                        if (otherAgent->m_UnstuckEnabled /*&& attempt == priorityAvoidanceAttempt*/)
                        {
                          otherVel = MathTools::As2DVec(agentLinVel + otherObs->m_Dir * otherObs->m_Speed) * 0.5;
                        }
                        else if (otherAgent->m_PriorityAvoidanceDir != Vector3f::ZERO)
                        {
                          otherVel = (MathTools::As2DVec(agentLinVel) + MathTools::As2DVec(otherAgent->m_PriorityAvoidanceDir)) * 0.5;
                        }
                        else
                        {
                          otherVel = (MathTools::As2DVec(agentLinVel) + otherVel) * 0.5;
                        }
                      }
                      else
                      {
                        otherVel = (MathTools::As2DVec(agentLinVel) + otherVel) * 0.5;
                      }
                    }
                  }
                }

                vo.AddPoint(otherPos2D, otherObs->m_Dims.X(), otherVel);
#else
                RVO::ORCAAgent::AgentInfo info;
                info.position = otherPos2D;
                info.velocity = otherVel;
                info.radius = otherObs->m_Dims.X();
                orca.insertAgentNeighbor(info, (curPos2D - otherPos2D).SquaredLength());
#endif
              }
              else
              {
                //penumbra.AddBox(MathTools::As2DVec(otherObs->m_Dims), otherPos2D, Vector2f::UNIT_X, otherObs->m_Speed * iTime);
              }
            }
          }
#ifndef USE_ORCA
          Vector3f bestVelocity = MathTools::To3DVec(vo.FindBestVelocity(MathTools::As2DVec(agentLinVel)));
#else
          orca.computeNewVelocity(iTime);
          Vector3f bestVelocity = MathTools::To3DVec(orca.newVelocity_);
#endif
          Vector3f diff = bestVelocity - agentLinVel;
          // 0.5 speed per sec
          float maxAccel = agentObs.m_Speed * 0.5 * iTime;
          float desiredAccel = diff.Normalize();
          //agent.m_CorrectedVelocity = agentObs.m_LinVel + diff * Mathf::Min(maxAccel, desiredAccel);

          if (attempt == regularAttempt)
          {
            agent.m_CorrectedVelocity = bestVelocity;
            if (agent.m_PriorityAvoidance && agent.m_CorrectedVelocity.Length() < 0.1 * agentObs.m_Speed)
            {

            }
            else
            {
              agent.m_PriorityAvoidanceDir = Vector3f::ZERO;
              break;
            }
          }

          if (attempt == priorityAvoidanceAttempt)
          {
            agent.m_PriorityAvoidanceDir = bestVelocity;
          }
        }
      }
    }

    if (debugContext)
    {
      debugContext.Print() << "HasDest : " << agent.m_HasDest << "\n";
      debugContext.Print() << "Destination : " << agent.m_Dest << "\n";
      debugContext.Print() << "Direction : " << agentObs.m_Dir << "\n";
      debugContext.Print() << "CurFaceIdx : " << agent.m_CurFaceIdx << "\n";
      debugContext.Print() << "CurPathStep : " << agent.m_CurPathStep << "\n";
    }
  }

  void NavigatorSystem::HandleAgentDiffusion(Agent& agent, float iTime)
  {
    Obstacle& agentObs = m_Obstacles.Get(agent.m_Obstacle);

    if (agent.m_AvoidanceFactor > 0)
    {
      agent.m_AvoidanceFactor *= 0.99;
    }

    if (!agent.m_PriorityAvoidance
      && agentObs.m_NumSamples >= Obstacle::s_WindowSize
      && agentObs.m_ImmobilityFactor < 0.1 * agentObs.m_Speed
      && agent.m_HasPriority
      && !agent.m_UnstuckEnabled)
    {
      agent.m_AvoidanceFactor += 0.5f * iTime;
      if (agent.m_AvoidanceFactor > 0.5f)
      {
        agent.m_UnstuckEnabled = true;
      }
    }

    if (agent.m_UnstuckEnabled
      && agentObs.m_ImmobilityFactor > 0.6 * agentObs.m_Speed)
    {
      agent.m_UnstuckEnabled = false;
      agent.m_AvoidanceDelta = 0;
      agent.m_AvoidanceFactor = 0;
    }

    if (agent.m_AvoidanceDelta > 0)
    {
      agent.m_AvoidanceFactor += agent.m_AvoidanceDelta;
      agent.m_AvoidanceDelta = 0.0;
    }

    if (agent.m_AvoidanceFactor > 0.5)
    {
      if (!agent.m_PriorityAvoidance && !agent.m_HasPriority)
      {
        agent.m_PriorityAvoidance = true;
      }
    }

    if (agent.m_PriorityAvoidance
      && (agent.m_AvoidanceFactor < 0.1
        || agent.m_HasPriority))
    {
      agent.m_AvoidanceFactor = 0;
      agent.m_PriorityAvoidance = false;
      Obstacle& agentObs = m_Obstacles.Get(agent.m_Obstacle);
      agentObs.m_NumSamples = 0;
    }
  }

  //void NavigatorSystem::DrawBeacons(DebugTool::Drawer& iDrawer)
  //{
  //  for (uint32_t i = 0; i < m_ActiveBeacons; ++i)
  //  {
  //    Beacon* beacon = m_Beacons[i];
  //    Vector3f center(beacon->m_Position.X(), beacon->m_Position.Y(), 0.0);
  //    uint32_t const numArcs = 32;
  //    float const angleIncrease = (Mathf::PI / numArcs) * 2;
  //    for (uint32_t arcSeg = 0; arcSeg < numArcs; ++arcSeg)
  //    {
  //      Vector4f color(0.0, 0.0, 1.0, 1.0);
  //      Vector3f pt1(Mathf::Cos(angleIncrease * arcSeg), Mathf::Sin(angleIncrease * arcSeg), 0.0);
  //      Vector3f pt2(Mathf::Cos(angleIncrease * (arcSeg + 1)), Mathf::Sin(angleIncrease * (arcSeg + 1)), 0.0);
  //
  //      iDrawer.DrawLine(pt1 * beacon->m_Radius + center, pt2 * beacon->m_Radius + center, color);
  //    }
  //  }
  //}
}