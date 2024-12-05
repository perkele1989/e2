
#pragma once 

#include <e2/context.hpp>

#include <e2/utils.hpp>

namespace e2
{
	struct E2_API RenderView
	{
		glm::dquat orientation{ glm::identity<glm::dquat>() };
		glm::dvec3 origin;
		glm::dvec2 clipPlane{0.1, 100.0};
		double fov{ 60.0 };

		glm::dmat4 calculateViewMatrix() const;
		glm::dmat4 calculateProjectionMatrix(glm::dvec2 const& resolution) const;
		glm::dvec3 findWorldspaceViewRayFromNdc(glm::dvec2 const& resolution, glm::dvec2 const& xyPlane) const;
		glm::dvec2 unprojectWorldPlane(glm::dvec2 const& resolution, glm::dvec2 const& xyCoords, double limitDistance) const;
		glm::dvec2 unprojectWorld(glm::dvec2 const& resolution, glm::dvec3 const& worldPosition) const;
	};

}