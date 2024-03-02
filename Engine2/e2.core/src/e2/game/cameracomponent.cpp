
#include "e2/game/cameracomponent.hpp"
#include "e2/game/gamesession.hpp"

e2::CameraComponent::CameraComponent()
	: e2::SceneComponent()
{

}

e2::CameraComponent::CameraComponent(e2::Entity* entity, e2::Name name)
	: e2::SceneComponent(entity, name)
{

}

e2::CameraComponent::~CameraComponent()
{

}

void e2::CameraComponent::onSpawned()
{

}

void e2::CameraComponent::onDestroyed()
{

}

void e2::CameraComponent::tick(double seconds)
{
	e2::Session* session = worldSession();
	e2::GameSession* asGameSession = dynamic_cast<e2::GameSession*>(session);
	if (!asGameSession)
		return;

	e2::RenderView newView{};
	newView.fov = m_fov;
	newView.clipPlane = m_nearClip;
	newView.origin = getTransform()->getTranslation(e2::TransformSpace::World);
	newView.orientation = getTransform()->getRotation(e2::TransformSpace::World);

	asGameSession->renderer()->setView(newView);
}