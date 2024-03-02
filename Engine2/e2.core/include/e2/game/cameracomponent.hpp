
#pragma once 

#include <e2/game/scenecomponent.hpp>

#include <e2/renderer/camera.hpp>

namespace e2
{

	/** @tags(arena, arenaSize=64, dynamic) */
	class E2_API CameraComponent : public e2::SceneComponent
	{
		ObjectDeclaration()
	public:
		CameraComponent();
		CameraComponent(e2::Entity* entity, e2::Name name);
		virtual ~CameraComponent();

		virtual void onSpawned() override;
		virtual void onDestroyed() override;

		virtual void tick(double seconds) override;

	protected:
		glm::vec2 m_nearClip{ 0.01f, 100.0f };
		float m_fov{ 60.0f };

	};
}

#include <cameracomponent.generated.hpp>