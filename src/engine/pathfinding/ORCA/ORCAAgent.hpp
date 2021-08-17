/*
 * Agent.h
 * RVO2 Library
 *
 * Copyright 2008 University of North Carolina at Chapel Hill
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Please send all bug reports to <geom@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Jur van den Berg, Stephen J. Guy, Jamie Snape, Ming C. Lin, Dinesh Manocha
 * Dept. of Computer Science
 * 201 S. Columbia St.
 * Frederick P. Brooks, Jr. Computer Science Bldg.
 * Chapel Hill, N.C. 27599-3175
 * United States of America
 *
 * <http://gamma.cs.unc.edu/RVO2/>
 */

#pragma once

#include <math/vector2.hpp>
#include <math/segment.hpp>
#include <engine/pathfinding/navigator.hpp>

namespace RVO {

  struct Line
  {
    eXl::Vector2f point;
    eXl::Vector2f direction;
  };

  inline float det(eXl::Vector2f const& iVec1, eXl::Vector2f const& iVec2)
  {
    return eXl::Segmentf::Cross(iVec1, iVec2);
  }

  inline float sqr(float iVal)
  {
    return iVal * iVal;
  }

  inline float absSq(eXl::Vector2f const& iVec)
  {
    return iVec.SquaredLength();
  }

  inline float abs(eXl::Vector2f const& iVec)
  {
    return iVec.Length();
  }

  inline float distSqPointLineSegment(const eXl::Vector2f &a, const eXl::Vector2f &b,
    const eXl::Vector2f &c)
  {
    const float r = ((c - a).Dot(b - a)) / absSq(b - a);

    if (r < 0.0f) {
      return absSq(c - a);
    }
    else if (r > 1.0f) {
      return absSq(c - b);
    }
    else {
      return absSq(c - (a + r * (b - a)));
    }
  }

  inline eXl::Vector2f normalize(eXl::Vector2f const& iVec)
  {
    eXl::Vector2f normVec = iVec;
    normVec.Normalize();

    return normVec;
  }

	/**
	 * \brief      Defines an agent in the simulation.
	 */
	class ORCAAgent 
  {
    public:

    ORCAAgent();

    void Init(eXl::NavigatorSystem::Agent& iAgent);

		/**
		 * \brief      Computes the new velocity of this agent.
		 */
		void computeNewVelocity(float timeStep);

    struct AgentInfo
    {
      eXl::Vector2f position;
      eXl::Vector2f velocity;
      float radius;
    };

		/**
		 * \brief      Inserts an agent neighbor into the set of neighbors of
		 *             this agent.
		 * \param      agent           A pointer to the agent to be inserted.
		 * \param      rangeSq         The squared range around this agent.
		 */
		//void insertAgentNeighbor(const eXl::NavigatorSystem::Agent *agent, const eXl::NavigatorSystem::Agent *agentObstacle, float &rangeSq);
    void insertAgentNeighbor(const AgentInfo& iAgent, float distSq);

		/**
		 * \brief      Inserts a static obstacle neighbor into the set of neighbors
		 *             of this agent.
		 * \param      obstacle        The number of the static obstacle to be
		 *                             inserted.
		 * \param      rangeSq         The squared range around this agent.
		 */
		//void insertObstacleNeighbor(const eXl::NavigatorSystem::Obstacle *obstacle, float rangeSq);
    void insertObstacleNeighbor(eXl::Segmentf const&, float distSq);

		eXl::Vector<std::pair<float, AgentInfo >> agentNeighbors_;
		size_t maxNeighbors_;
		float maxSpeed_;
		float neighborDist_;
    eXl::Vector2f newVelocity_;
		//eXl::Vector<std::pair<float, const eXl::NavigatorSystem::Obstacle *> > obstacleNeighbors_;
    eXl::Vector<std::pair<float, eXl::Segmentf> > obstacleNeighbors_;
		eXl::Vector<Line> orcaLines_;
		eXl::Vector2f position_;
		eXl::Vector2f prefVelocity_;
		float radius_;
		float timeHorizon_;
		float timeHorizonObst_;
    eXl::Vector2f velocity_;

		size_t id_;
	};

	/**
	 * \relates    Agent
	 * \brief      Solves a one-dimensional linear program on a specified line
	 *             subject to linear constraints defined by lines and a circular
	 *             constraint.
	 * \param      lines         Lines defining the linear constraints.
	 * \param      lineNo        The specified line constraint.
	 * \param      radius        The radius of the circular constraint.
	 * \param      optVelocity   The optimization velocity.
	 * \param      directionOpt  True if the direction should be optimized.
	 * \param      result        A reference to the result of the linear program.
	 * \return     True if successful.
	 */
	bool linearProgram1(const eXl::Vector<Line> &lines, size_t lineNo,
						float radius, const eXl::Vector2f &optVelocity,
						bool directionOpt, eXl::Vector2f &result);

	/**
	 * \relates    Agent
	 * \brief      Solves a two-dimensional linear program subject to linear
	 *             constraints defined by lines and a circular constraint.
	 * \param      lines         Lines defining the linear constraints.
	 * \param      radius        The radius of the circular constraint.
	 * \param      optVelocity   The optimization velocity.
	 * \param      directionOpt  True if the direction should be optimized.
	 * \param      result        A reference to the result of the linear program.
	 * \return     The number of the line it fails on, and the number of lines if successful.
	 */
	size_t linearProgram2(const eXl::Vector<Line> &lines, float radius,
						  const eXl::Vector2f &optVelocity, bool directionOpt,
    eXl::Vector2f &result);

	/**
	 * \relates    Agent
	 * \brief      Solves a two-dimensional linear program subject to linear
	 *             constraints defined by lines and a circular constraint.
	 * \param      lines         Lines defining the linear constraints.
	 * \param      numObstLines  Count of obstacle lines.
	 * \param      beginLine     The line on which the 2-d linear program failed.
	 * \param      radius        The radius of the circular constraint.
	 * \param      result        A reference to the result of the linear program.
	 */
	void linearProgram3(const eXl::Vector<Line> &lines, size_t numObstLines, size_t beginLine,
						float radius, eXl::Vector2f &result);
}

