
#pragma once 

#include <e2/game/component.hpp>
#include <e2/transform.hpp>

#include <glm/glm.hpp>

namespace e2
{
	/** @tags(arena, arenaSize=1024, dynamic) */
	class E2_API SceneComponent : public e2::Component
	{
		ObjectDeclaration()
	public:
		SceneComponent();
		SceneComponent(e2::Entity* entity, e2::Name name);
		virtual ~SceneComponent();

		inline e2::Transform* getTransform() const
		{
			return m_transform;
		}

	protected:
		e2::Transform *m_transform{};
	};
}

#include "scenecomponent.generated.hpp"