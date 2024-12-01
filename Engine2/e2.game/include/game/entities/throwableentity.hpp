
#pragma once 

#include "game/entity.hpp"

namespace e2
{
	class ThrowableEntity : public e2::Entity
	{
		ObjectDeclaration();
	public:
		virtual ~ThrowableEntity() {}
		virtual void setVelocity(glm::vec3 newVelocity) {}
	};
}

#include "throwableentity.generated.hpp"