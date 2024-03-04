
#pragma once 

#include <e2/context.hpp>

#include <e2/utils.hpp>

namespace e2
{
	struct E2_API RenderView
	{
		glm::quat orientation{ glm::identity<glm::quat>() };
		glm::vec3 origin;
		glm::vec2 clipPlane{0.01f, 100.0f};
		float fov{ 60.0f };

		glm::mat4 calculateViewMatrix() const;
		glm::mat4 calculateProjectionMatrix(glm::vec2 const& resolution) const;
		glm::vec3 findWorldspaceViewRayFromNdc(glm::vec2 const& resolution, glm::vec2 const& xyPlane) const;
		glm::vec2 unprojectWorldPlane(glm::vec2 const& resolution, glm::vec2 const& xyCoords) const;
	};

}