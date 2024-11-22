#include "game/shared.hpp"


#include <glm/gtx/intersect.hpp>

e2::SweepResult e2::circleSweepTest(
    glm::vec2 const& startLocation,
    glm::vec2 const& endLocation,
    float sweepRadius,
    glm::vec2 const& obstacleLocation,
    float obstacleRadius)
{
    e2::SweepResult result;
    result.obstacleLocation = obstacleLocation;
    result.obstacleRadius = obstacleRadius;

    if (startLocation == endLocation)
    {
        return result;
    }

    glm::vec2 pa, pb, na, nb;
    result.hit = glm::intersectLineSphere(startLocation, endLocation, obstacleLocation, sweepRadius + obstacleRadius, pa, na, pb, nb);

    if (glm::length(pa - startLocation) < glm::length(pb - startLocation))
    {
        result.hitLocation = pa;
        result.hitNormal = na;
    }
    else
    {
        result.hitLocation = pb;
        result.hitNormal = nb;
    }

    if (glm::dot(glm::normalize(endLocation - startLocation), glm::normalize(result.hitLocation - startLocation)) <= 0.01f)
    {
        result.hit = false;
        return result;
    }


    if (glm::dot(glm::normalize(endLocation - startLocation), result.hitNormal) > 0.0f)
    {
        result.hit = false;
        return result;
    }


    if (glm::length(startLocation - result.hitLocation) > glm::length(startLocation - endLocation))
    {
        result.hit = false;
        return result;
    }

    if (!result.hit)
    {
        return result;
    }

    glm::vec2 obstacleToHit = glm::normalize(result.hitLocation - obstacleLocation);
    result.stopLocation = obstacleLocation + obstacleToHit * (obstacleRadius + sweepRadius);
    result.moveDistance = glm::length(result.stopLocation - startLocation) ;
    
    return result;
}

e2::OverlapResult e2::circleOverlapTest(glm::vec2 const& location1, float radius1, glm::vec2 const& location2, float radius2)
{
    e2::OverlapResult result;
    glm::vec2 atob = location2 - location1;
    float distance = glm::length(atob);

    result.overlaps = distance < radius1 + radius2;
    result.overlapDirection = glm::normalize(atob);
    result.overlapDepth = -(distance - radius1 - radius2);

    return result;
}
