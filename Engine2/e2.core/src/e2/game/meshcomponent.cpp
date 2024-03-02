
#include "e2/game/meshcomponent.hpp"

#include "e2/game/world.hpp"
#include "e2/managers/gamemanager.hpp"
#include "e2/renderer/meshproxy.hpp"
#include "e2/assets/material.hpp"
#include "e2/assets/mesh.hpp"
#include "e2/game/gamesession.hpp"

e2::MeshComponent::MeshComponent(e2::Entity* entity, e2::Name name)
	: e2::SceneComponent(entity, name)
{
	LogNotice("Creating mesh component {}", name.cstring());
}

e2::MeshComponent::MeshComponent()
	: e2::SceneComponent()
{
	LogNotice("Creating empty mesh component");
}

e2::MeshComponent::~MeshComponent()
{
	LogNotice("Destroying mesh component {}", m_name.cstring());
	destroyProxy();
}

bool e2::MeshComponent::assertProxy()
{
	if (m_proxy)
	{
		return true;
	}

	if (!m_mesh || m_materialProxies.size() != m_mesh->submeshCount())
	{
		return false;
	}

	if (!m_mesh->isDone())
	{
		return false;
	}

	for (e2::MaterialProxy *material : m_materialProxies)
	{
		if (!material)
			return false;
	}

	e2::MeshProxyConfiguration meshConfig;
	meshConfig.mesh = m_mesh;
	meshConfig.materials = m_materialProxies;
	m_proxy = e2::create<MeshProxy>(worldSession(), meshConfig);

	return true;
}

bool e2::MeshComponent::recreateProxy()
{
	destroyProxy();
	return assertProxy();
}


void e2::MeshComponent::destroyProxy()
{
	if (!m_proxy)
	{
		return;
	}

	e2::destroy(m_proxy);
	m_proxy = nullptr;
	m_materialProxies.clear();
}


void e2::MeshComponent::onSpawned()
{
	e2::SceneComponent::onSpawned();
}

void e2::MeshComponent::tick(double seconds)
{
	e2::SceneComponent::tick(seconds);

	if (!assertProxy())
		return;

	// @todo optimize with external dirty parameter in e2::Transform 
	// will save us a matrix comparison for every component, which can be a lot
	glm::mat4 currMatrix = getTransform()->getTransformMatrix(TransformSpace::World);
	if (currMatrix != m_proxy->modelMatrix)
	{
		m_proxy->modelMatrix = currMatrix;
		m_proxy->modelMatrixDirty = { true };
	}
}

e2::MeshPtr e2::MeshComponent::mesh() const
{
	return m_mesh;
}

void e2::MeshComponent::mesh(e2::MeshPtr newMesh)
{
	destroyProxy();
	m_mesh = newMesh;

	if (!m_mesh)
		return;

	for(uint8_t i = 0; i < m_mesh->submeshCount(); i++)
		m_materialProxies.push(worldSession()->getOrCreateDefaultMaterialProxy(m_mesh->material(i)));

	assertProxy();
}

e2::StackVector<e2::MaterialProxy*, e2::maxNumSubmeshes>const& e2::MeshComponent::materialProxies() const
{
	return m_materialProxies;
}

void e2::MeshComponent::onDestroyed()
{
	e2::SceneComponent::onDestroyed();
}

void e2::MeshComponent::setMaterial(uint8_t submeshIndex, e2::MaterialProxy *newMaterial)
{
	// Make sure to destroy our render proxy here, it will be recreated when needed the next time 
	destroyProxy();
	m_materialProxies[submeshIndex] = newMaterial;
}

