
#include "e2/game/scenecomponent.hpp"

e2::SceneComponent::SceneComponent(e2::Entity* entity, e2::Name name)
	: e2::Component(entity, name)
{
	m_transform = e2::create<e2::Transform>();
}

e2::SceneComponent::SceneComponent()
	: e2::Component(nullptr, 0)
{

}

e2::SceneComponent::~SceneComponent()
{
	e2::destroy(m_transform);
}
