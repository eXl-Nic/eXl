/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "ORCAAgent.hpp"

#include <engine/common/debugtool.hpp>
#include <math/mathtools.hpp>

const float RVO_EPSILON = 0.00001f;

namespace RVO {
	ORCAAgent::ORCAAgent() 
    : maxNeighbors_(32)
    , maxSpeed_(0.0f)
    , neighborDist_(0.0f)
    , radius_(0.0f)
    , timeHorizon_(0.0f)
    , timeHorizonObst_(0.0f)
    , id_(0) 
  { }

  void Init(eXl::NavigatorSystem::Agent&)
  {

  }

  struct Obstacle 
  {
    bool isConvex_ = true;
    Obstacle *nextObstacle_;
    eXl::Vector2f point_;
    Obstacle *prevObstacle_;
    eXl::Vector2f unitDir_;
  };

	/* Search for the best new velocity. */
	void ORCAAgent::computeNewVelocity(float timeStep)
	{
		orcaLines_.clear();

		const float invTimeHorizonObst = 1.0f / timeHorizonObst_;

		/* Create obstacle ORCA lines. */
		for (size_t i = 0; i < obstacleNeighbors_.size(); ++i) 
    {
      Obstacle obsL;
      Obstacle obsW1;
      Obstacle obsW2;
      Obstacle obsOL;

      obsL.nextObstacle_ = &obsW2;
      obsL.prevObstacle_ = &obsW1;
      obsW1.nextObstacle_ = &obsL;
      obsW1.prevObstacle_ = &obsOL;
      obsW2.nextObstacle_ = &obsOL;
      obsW2.prevObstacle_ = &obsL;
      obsOL.nextObstacle_ = &obsW1;
      obsOL.prevObstacle_ = &obsW2;

      eXl::Segmentf wallDesc = obstacleNeighbors_[i].second;

      eXl::Vector2f wallDir = wallDesc.m_Ext1 - wallDesc.m_Ext2;
      wallDir.Normalize();
      eXl::Vector2f outDir(wallDir.Y(), -wallDir.X());

      obsL.unitDir_ = wallDir;
      obsOL.unitDir_ = -wallDir;
      obsW1.unitDir_ = -outDir;
      obsW2.unitDir_ = outDir;
      obsL.point_ = wallDesc.m_Ext1;
      obsW1.point_ = wallDesc.m_Ext1 + outDir;
      obsW2.point_ = wallDesc.m_Ext2;
      obsOL.point_ = wallDesc.m_Ext2 + outDir;

      const Obstacle *obstacle1 = &obsL;
      const Obstacle *obstacle2 = obstacle1->nextObstacle_;

			const eXl::Vector2f relativePosition1 = obstacle1->point_ - position_;
			const eXl::Vector2f relativePosition2 = obstacle2->point_ - position_;

			/*
			 * Check if velocity obstacle of obstacle is already taken care of by
			 * previously constructed obstacle ORCA lines.
			 */
			bool alreadyCovered = false;

			for (size_t j = 0; j < orcaLines_.size(); ++j) 
      {
				if (det(invTimeHorizonObst * relativePosition1 - orcaLines_[j].point, orcaLines_[j].direction) - invTimeHorizonObst * radius_ >= -RVO_EPSILON 
          && det(invTimeHorizonObst * relativePosition2 - orcaLines_[j].point, orcaLines_[j].direction) - invTimeHorizonObst * radius_ >=  -RVO_EPSILON) 
        {
					alreadyCovered = true;
					break;
				}
			}

			if (alreadyCovered) 
      {
				continue;
			}

			/* Not yet covered. Check for collisions. */

			const float distSq1 = absSq(relativePosition1);
			const float distSq2 = absSq(relativePosition2);

			const float radiusSq = sqr(radius_);

			const eXl::Vector2f obstacleVector = relativePosition2 - relativePosition1;
			const float s = (-relativePosition1.Dot(obstacleVector)) / absSq(obstacleVector);
			const float distSqLine = absSq(-relativePosition1 - s * obstacleVector);

			Line line;

			if (s < 0.0f && distSq1 <= radiusSq) 
      {
				/* Collision with left vertex. Ignore if non-convex. */
				if (obstacle1->isConvex_) 
        {
					line.point = eXl::Vector2f(0.0f, 0.0f);
					line.direction = normalize(eXl::Vector2f(-relativePosition1.Y(), relativePosition1.X()));
					orcaLines_.push_back(line);
				}

				continue;
			}
			else if (s > 1.0f && distSq2 <= radiusSq) 
      {
				/* Collision with right vertex. Ignore if non-convex
				 * or if it will be taken care of by neighoring obstace */
				if (obstacle2->isConvex_ && det(relativePosition2, obstacle2->unitDir_) >= 0.0f) 
        {
					line.point = eXl::Vector2f(0.0f, 0.0f);
					line.direction = normalize(eXl::Vector2f(-relativePosition2.Y(), relativePosition2.X()));
					orcaLines_.push_back(line);
				}

				continue;
			}
			else if (s >= 0.0f && s < 1.0f && distSqLine <= radiusSq) 
      {
				/* Collision with obstacle segment. */
				line.point = eXl::Vector2f(0.0f, 0.0f);
				line.direction = -obstacle1->unitDir_;
				orcaLines_.push_back(line);
				continue;
			}

			/*
			 * No collision.
			 * Compute legs. When obliquely viewed, both legs can come from a single
			 * vertex. Legs extend cut-off line when nonconvex vertex.
			 */

			eXl::Vector2f leftLegDirection, rightLegDirection;

			if (s < 0.0f && distSqLine <= radiusSq) 
      {
				/*
				 * Obstacle viewed obliquely so that left vertex
				 * defines velocity obstacle.
				 */
				//if (!obstacle1->isConvex_) 
        //{
				//	/* Ignore obstacle. */
				//	continue;
				//}

				obstacle2 = obstacle1;

				const float leg1 = std::sqrt(distSq1 - radiusSq);
				leftLegDirection = eXl::Vector2f(relativePosition1.X() * leg1 - relativePosition1.Y() * radius_, relativePosition1.X() * radius_ + relativePosition1.Y() * leg1) / distSq1;
				rightLegDirection = eXl::Vector2f(relativePosition1.X() * leg1 + relativePosition1.Y() * radius_, -relativePosition1.X() * radius_ + relativePosition1.Y() * leg1) / distSq1;
			}
			else if (s > 1.0f && distSqLine <= radiusSq) 
      {
				/*
				 * Obstacle viewed obliquely so that
				 * right vertex defines velocity obstacle.
				 */
				if (!obstacle2->isConvex_) 
        {
					/* Ignore obstacle. */
					continue;
				}

				obstacle1 = obstacle2;

				const float leg2 = std::sqrt(distSq2 - radiusSq);
				leftLegDirection = eXl::Vector2f(relativePosition2.X() * leg2 - relativePosition2.Y() * radius_, relativePosition2.X() * radius_ + relativePosition2.Y() * leg2) / distSq2;
				rightLegDirection = eXl::Vector2f(relativePosition2.X() * leg2 + relativePosition2.Y() * radius_, -relativePosition2.X() * radius_ + relativePosition2.Y() * leg2) / distSq2;
			}
			else 
      {
				/* Usual situation. */
				if (obstacle1->isConvex_) 
        {
					const float leg1 = std::sqrt(distSq1 - radiusSq);
					leftLegDirection = eXl::Vector2f(relativePosition1.X() * leg1 - relativePosition1.Y() * radius_, relativePosition1.X() * radius_ + relativePosition1.Y() * leg1) / distSq1;
				}
				else 
        {
					/* Left vertex non-convex; left leg extends cut-off line. */
					leftLegDirection = -obstacle1->unitDir_;
				}

				if (obstacle2->isConvex_) 
        {
					const float leg2 = std::sqrt(distSq2 - radiusSq);
					rightLegDirection = eXl::Vector2f(relativePosition2.X() * leg2 + relativePosition2.Y() * radius_, -relativePosition2.X() * radius_ + relativePosition2.Y() * leg2) / distSq2;
				}
				else 
        {
					/* Right vertex non-convex; right leg extends cut-off line. */
					rightLegDirection = obstacle1->unitDir_;
				}
			}

			/*
			 * Legs can never point into neighboring edge when convex vertex,
			 * take cutoff-line of neighboring edge instead. If velocity projected on
			 * "foreign" leg, no constraint is added.
			 */

			const Obstacle *const leftNeighbor = obstacle1->prevObstacle_;

			bool isLeftLegForeign = false;
			bool isRightLegForeign = false;

			if (obstacle1->isConvex_ && det(leftLegDirection, -leftNeighbor->unitDir_) >= 0.0f) 
      {
				/* Left leg points into obstacle. */
				leftLegDirection = -leftNeighbor->unitDir_;
				isLeftLegForeign = true;
			}

			if (obstacle2->isConvex_ && det(rightLegDirection, obstacle2->unitDir_) <= 0.0f) 
      {
				/* Right leg points into obstacle. */
				rightLegDirection = obstacle2->unitDir_;
				isRightLegForeign = true;
			}

			/* Compute cut-off centers. */
			const eXl::Vector2f leftCutoff = invTimeHorizonObst * (obstacle1->point_ - position_);
			const eXl::Vector2f rightCutoff = invTimeHorizonObst * (obstacle2->point_ - position_);
			const eXl::Vector2f cutoffVec = rightCutoff - leftCutoff;

			/* Project current velocity on velocity obstacle. */

			/* Check if current velocity is projected on cutoff circles. */
			const float t = (obstacle1 == obstacle2 ? 0.5f : ((velocity_ - leftCutoff).Dot(cutoffVec)) / absSq(cutoffVec));
			const float tLeft = ((velocity_ - leftCutoff).Dot(leftLegDirection));
			const float tRight = ((velocity_ - rightCutoff).Dot(rightLegDirection));

			if ((t < 0.0f && tLeft < 0.0f) || (obstacle1 == obstacle2 && tLeft < 0.0f && tRight < 0.0f)) 
      {
				/* Project on left cut-off circle. */
				const eXl::Vector2f unitW = normalize(velocity_ - leftCutoff);

				line.direction = eXl::Vector2f(unitW.Y(), -unitW.X());
				line.point = leftCutoff + radius_ * invTimeHorizonObst * unitW;
				orcaLines_.push_back(line);
				continue;
			}
			else if (t > 1.0f && tRight < 0.0f) 
      {
				/* Project on right cut-off circle. */
				const eXl::Vector2f unitW = normalize(velocity_ - rightCutoff);

				line.direction = eXl::Vector2f(unitW.Y(), -unitW.X());
				line.point = rightCutoff + radius_ * invTimeHorizonObst * unitW;
				orcaLines_.push_back(line);
				continue;
			}

			/*
			 * Project on left leg, right leg, or cut-off line, whichever is closest
			 * to velocity.
			 */
			const float distSqCutoff = ((t < 0.0f || t > 1.0f || obstacle1 == obstacle2) ? std::numeric_limits<float>::infinity() : absSq(velocity_ - (leftCutoff + t * cutoffVec)));
			const float distSqLeft = ((tLeft < 0.0f) ? std::numeric_limits<float>::infinity() : absSq(velocity_ - (leftCutoff + tLeft * leftLegDirection)));
			const float distSqRight = ((tRight < 0.0f) ? std::numeric_limits<float>::infinity() : absSq(velocity_ - (rightCutoff + tRight * rightLegDirection)));

			if (distSqCutoff <= distSqLeft && distSqCutoff <= distSqRight) 
      {
				/* Project on cut-off line. */
				line.direction = -obstacle1->unitDir_;
				line.point = leftCutoff + radius_ * invTimeHorizonObst * eXl::Vector2f(-line.direction.Y(), line.direction.X());
				orcaLines_.push_back(line);
				continue;
			}
			else if (distSqLeft <= distSqRight) 
      {
				/* Project on left leg. */
				if (isLeftLegForeign) 
        {
					continue;
				}

				line.direction = leftLegDirection;
				line.point = leftCutoff + radius_ * invTimeHorizonObst * eXl::Vector2f(-line.direction.Y(), line.direction.X());
				orcaLines_.push_back(line);
				continue;
			}
			else 
      {
				/* Project on right leg. */
				if (isRightLegForeign) 
        {
					continue;
				}

				line.direction = -rightLegDirection;
				line.point = rightCutoff + radius_ * invTimeHorizonObst * eXl::Vector2f(-line.direction.Y(), line.direction.X());
				orcaLines_.push_back(line);
				continue;
			}
		}

    const size_t numObstLines = orcaLines_.size();

		const float invTimeHorizon = 1.0f / timeHorizon_;

		/* Create agent ORCA lines. */
		for (size_t i = 0; i < agentNeighbors_.size(); ++i) 
    {
			const AgentInfo& other = agentNeighbors_[i].second;

			const eXl::Vector2f relativePosition = other.position - position_;
			const eXl::Vector2f relativeVelocity = velocity_ - other.velocity;
			const float distSq = relativePosition.SquaredLength();
			const float combinedRadius = radius_ + other.radius;
			const float combinedRadiusSq = combinedRadius * combinedRadius;

			Line line;
			eXl::Vector2f u;

			if (distSq > combinedRadiusSq) 
      {
				/* No collision. */
				const eXl::Vector2f w = relativeVelocity - invTimeHorizon * relativePosition;
				/* Vector from cutoff center to relative velocity. */
				const float wLengthSq = absSq(w);

				const float dotProduct1 = w.Dot(relativePosition);

				if (dotProduct1 < 0.0f && sqr(dotProduct1) > combinedRadiusSq * wLengthSq) 
        {
					/* Project on cut-off circle. */
					const float wLength = std::sqrt(wLengthSq);
					const eXl::Vector2f unitW = w / wLength;

					line.direction = eXl::Vector2f(unitW.Y(), -unitW.X());
					u = (combinedRadius * invTimeHorizon - wLength) * unitW;
				}
				else 
        {
					/* Project on legs. */
					const float leg = std::sqrt(distSq - combinedRadiusSq);

					if (det(relativePosition, w) > 0.0f) 
          {
						/* Project on left leg. */
						line.direction = eXl::Vector2f(relativePosition.X() * leg - relativePosition.Y() * combinedRadius, relativePosition.X() * combinedRadius + relativePosition.Y() * leg) / distSq;
					}
					else 
          {
						/* Project on right leg. */
						line.direction = -eXl::Vector2f(relativePosition.X() * leg + relativePosition.Y() * combinedRadius, -relativePosition.X() * combinedRadius + relativePosition.Y() * leg) / distSq;
					}

					const float dotProduct2 = relativeVelocity.Dot(line.direction);

					u = dotProduct2 * line.direction - relativeVelocity;
				}
			}
			else 
      {
				/* Collision. Project on cut-off circle of time timeStep. */
				const float invTimeStep = 1.0f / timeStep;

				/* Vector from cutoff center to relative velocity. */
				const eXl::Vector2f w = relativeVelocity - invTimeStep * relativePosition;

				const float wLength = abs(w);
				const eXl::Vector2f unitW = w / wLength;

				line.direction = eXl::Vector2f(unitW.Y(), -unitW.X());
				u = (combinedRadius * invTimeStep - wLength) * unitW;
			}

			line.point = velocity_ + 0.5f * u;
			orcaLines_.push_back(line);
		}

		size_t lineFail = linearProgram2(orcaLines_, maxSpeed_, prefVelocity_, false, newVelocity_);

		if (lineFail < orcaLines_.size()) {
			linearProgram3(orcaLines_, numObstLines, lineFail, maxSpeed_, newVelocity_);
		}
	}

	void ORCAAgent::insertAgentNeighbor(const AgentInfo& agent, float distSq)
	{
		if (agentNeighbors_.size() < maxNeighbors_) 
    {
			agentNeighbors_.push_back(std::make_pair(distSq, agent));
		}

		size_t i = agentNeighbors_.size() - 1;

		while (i != 0 && distSq < agentNeighbors_[i - 1].first) 
    {
			agentNeighbors_[i] = agentNeighbors_[i - 1];
			--i;
		}

		agentNeighbors_[i] = std::make_pair(distSq, agent);
	}

	void ORCAAgent::insertObstacleNeighbor(eXl::Segmentf const& iSeg, float distSq)
	{
		obstacleNeighbors_.push_back(std::make_pair(distSq, iSeg));

		size_t i = obstacleNeighbors_.size() - 1;

		while (i != 0 && distSq < obstacleNeighbors_[i - 1].first) 
    {
			obstacleNeighbors_[i] = obstacleNeighbors_[i - 1];
			--i;
		}

		obstacleNeighbors_[i] = std::make_pair(distSq, iSeg);
	}

	bool linearProgram1(const eXl::Vector<Line> &lines, size_t lineNo, float radius, const eXl::Vector2f &optVelocity, bool directionOpt, eXl::Vector2f &result)
	{
		const float dotProduct = lines[lineNo].point.Dot(lines[lineNo].direction);
		const float discriminant = sqr(dotProduct) + sqr(radius) - absSq(lines[lineNo].point);

		if (discriminant < 0.0f) 
    {
			/* Max speed circle fully invalidates line lineNo. */
			return false;
		}

		const float sqrtDiscriminant = std::sqrt(discriminant);
		float tLeft = -dotProduct - sqrtDiscriminant;
		float tRight = -dotProduct + sqrtDiscriminant;

		for (size_t i = 0; i < lineNo; ++i) 
    {
			const float denominator = det(lines[lineNo].direction, lines[i].direction);
			const float numerator = det(lines[i].direction, lines[lineNo].point - lines[i].point);

			if (std::fabs(denominator) <= RVO_EPSILON) 
      {
				/* Lines lineNo and i are (almost) parallel. */
				if (numerator < 0.0f) 
        {
					return false;
				}
				else 
        {
					continue;
				}
			}

			const float t = numerator / denominator;

			if (denominator >= 0.0f) 
      {
				/* Line i bounds line lineNo on the right. */
				tRight = std::min(tRight, t);
			}
			else 
      {
				/* Line i bounds line lineNo on the left. */
				tLeft = std::max(tLeft, t);
			}

			if (tLeft > tRight) 
      {
				return false;
			}
		}

		if (directionOpt) 
    {
			/* Optimize direction. */
			if (optVelocity.Dot(lines[lineNo].direction) > 0.0f) 
      {
				/* Take right extreme. */
				result = lines[lineNo].point + tRight * lines[lineNo].direction;
			}
			else 
      {
				/* Take left extreme. */
				result = lines[lineNo].point + tLeft * lines[lineNo].direction;
			}
		}
		else 
    {
			/* Optimize closest point. */
			const float t = lines[lineNo].direction.Dot(optVelocity - lines[lineNo].point);

			if (t < tLeft) 
      {
				result = lines[lineNo].point + tLeft * lines[lineNo].direction;
			}
			else if (t > tRight) 
      {
				result = lines[lineNo].point + tRight * lines[lineNo].direction;
			}
			else 
      {
				result = lines[lineNo].point + t * lines[lineNo].direction;
			}
		}

		return true;
	}

	size_t linearProgram2(const eXl::Vector<Line> &lines, float radius, const eXl::Vector2f &optVelocity, bool directionOpt, eXl::Vector2f &result)
	{
		if (directionOpt) 
    {
			/*
			 * Optimize direction. Note that the optimization velocity is of unit
			 * length in this case.
			 */
			result = optVelocity * radius;
		}
		else if (absSq(optVelocity) > sqr(radius)) 
    {
			/* Optimize closest point and outside circle. */
			result = normalize(optVelocity) * radius;
		}
		else 
    {
			/* Optimize closest point and inside circle. */
			result = optVelocity;
		}

		for (size_t i = 0; i < lines.size(); ++i) 
    {
			if (det(lines[i].direction, lines[i].point - result) > 0.0f) 
      {
				/* Result does not satisfy constraint i. Compute new optimal result. */
				const eXl::Vector2f tempResult = result;

				if (!linearProgram1(lines, i, radius, optVelocity, directionOpt, result)) 
        {
					result = tempResult;
					return i;
				}
			}
		}

		return lines.size();
	}

	void linearProgram3(const eXl::Vector<Line> &lines, size_t numObstLines, size_t beginLine, float radius, eXl::Vector2f &result)
	{
		float distance = 0.0f;

		for (size_t i = beginLine; i < lines.size(); ++i) 
    {
			if (det(lines[i].direction, lines[i].point - result) > distance) 
      {
				/* Result does not satisfy constraint of line i. */
				eXl::Vector<Line> projLines(lines.begin(), lines.begin() + static_cast<ptrdiff_t>(numObstLines));

				for (size_t j = numObstLines; j < i; ++j) 
        {
					Line line;

					float determinant = det(lines[i].direction, lines[j].direction);

					if (std::fabs(determinant) <= RVO_EPSILON) 
          {
						/* Line i and line j are parallel. */
						if (lines[i].direction.Dot(lines[j].direction) > 0.0f) 
            {
							/* Line i and line j point in the same direction. */
							continue;
						}
						else 
            {
							/* Line i and line j point in opposite direction. */
							line.point = 0.5f * (lines[i].point + lines[j].point);
						}
					}
					else 
          {
						line.point = lines[i].point + (det(lines[j].direction, lines[i].point - lines[j].point) / determinant) * lines[i].direction;
					}

					line.direction = normalize(lines[j].direction - lines[i].direction);
					projLines.push_back(line);
				}

				const eXl::Vector2f tempResult = result;

				if (linearProgram2(projLines, radius, eXl::Vector2f(-lines[i].direction.Y(), lines[i].direction.X()), true, result) < projLines.size()) 
        {
					/* This should in principle not happen.  The result is by definition
					 * already in the feasible region of this linear program. If it fails,
					 * it is due to small floating point error, and the current result is
					 * kept.
					 */
					result = tempResult;
				}

				distance = det(lines[i].direction, lines[i].point - result);
			}
		}
	}
}
