
#include "e2/game/session.hpp"

#include "e2/assets/mesh.hpp"
#include "e2/assets/material.hpp"
#include "e2/assets/texture2d.hpp"

#include "e2/game/world.hpp"
#include "e2/game/entity.hpp"
#include "e2/game/meshcomponent.hpp"
#include "e2/renderer/shadermodel.hpp"
#include "e2/renderer/shadermodels/lightweight.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/managers/assetmanager.hpp"
#include "e2/managers/gamemanager.hpp"

#include "e2/renderer/meshproxy.hpp"

e2::Session::Session(e2::Context* ctx)
	: m_engine(ctx->engine())
{
	m_persistentWorld = spawnWorld();

	m_meshProxies.reserve(1024);
	//m_submeshIndex.reserve(2048);

	e2::DataBufferCreateInfo bufferCreateInfo;
	bufferCreateInfo.type = BufferType::UniformBuffer;
	bufferCreateInfo.dynamic = true;
	bufferCreateInfo.size = renderManager()->paddedBufferSize(sizeof(glm::mat4) * e2::maxNumMeshProxies);
	m_modelBuffers[0] = renderContext()->createDataBuffer(bufferCreateInfo);
	m_modelBuffers[1] = renderContext()->createDataBuffer(bufferCreateInfo);

	// @todo maybe default initialize the model buffers if neccessary (make sure we aren't rendering single frames with invalid data, as this can have severe performance implications)


	uint32_t dynamicSize = renderManager()->paddedBufferSize(sizeof(glm::mat4));
	//uint32_t dynamicSize = bufferCreateInfo.size;


	m_modelSets[0] = renderManager()->modelPool()->createDescriptorSet(renderManager()->modelSetLayout());
	m_modelSets[1] = renderManager()->modelPool()->createDescriptorSet(renderManager()->modelSetLayout());
	m_modelSets[0]->writeDynamicBuffer(0, m_modelBuffers[0], dynamicSize, 0);
	m_modelSets[1]->writeDynamicBuffer(0, m_modelBuffers[1], dynamicSize, 0);

	gameManager()->registerSession(this);
}

e2::Session::~Session()
{
	gameManager()->unregisterSession(this);
	std::unordered_set<e2::World*> tmpWorlds = m_worlds;
	for (e2::World* world : tmpWorlds)
	{
		destroyWorld(world);
	}

	for (std::pair<e2::Material* const, e2::MaterialProxy*> & p : m_defaultMaterialProxies)
	{
		//p.second->keepAround();
		e2::destroy(p.second);
	}

	e2::destroy(m_modelBuffers[0]);
	e2::destroy(m_modelBuffers[1]);

	e2::destroy(m_modelSets[0]);
	e2::destroy(m_modelSets[1]);
}

void e2::Session::tick(double seconds)
{

	for(e2::World* world : m_worlds)
		world->tick(seconds);

	// Now we actually update any mesh proxies and material proxies that need updating 
	// for the uninitiated, we do this here instead of renderer or world, because you can have many renderers, and you can have many worlds.
	uint8_t frameIndex = renderManager()->frameIndex();

	for (e2::MeshProxy* proxy : m_meshProxies)
	{
		if (proxy->modelMatrixDirty[frameIndex])
		{
			proxy->modelMatrixDirty[frameIndex] = false;

			// @todo if we ever change model buffers from being dynamic, we need to optimize these uploads
			// right now it's fine as theyre just mapped memcpys
			uint32_t proxyOffset = renderManager()->paddedBufferSize(sizeof(glm::mat4)) * proxy->id;
			m_modelBuffers[frameIndex]->upload(reinterpret_cast<uint8_t const*>(&proxy->modelMatrix), sizeof(glm::mat4), 0, proxyOffset);
		}

	}

	for (e2::MaterialProxy* proxy : m_materialProxies)
	{
		proxy->invalidate(frameIndex);

	}
}

e2::Engine* e2::Session::engine()
{
	return m_engine;
}

e2::World* e2::Session::spawnWorld()
{
	e2::World* newWorld = e2::create<e2::World>(this);
	m_worlds.insert(newWorld);
	return newWorld;
}

void e2::Session::destroyWorld(e2::World* world)
{
	m_worlds.erase(world);
	e2::destroy(world);
}


uint32_t e2::Session::registerMeshProxy(e2::MeshProxy* proxy)
{
	if (m_meshProxies.size() >= e2::maxNumMeshProxies)
	{
		LogError("maxNumMeshProxies reached");
		return UINT32_MAX;
	}

	m_meshProxies.insert(proxy);

	for (uint8_t i = 0; i < proxy->asset->submeshCount(); i++)
	{
		e2::ShaderModel* shaderModel = proxy->materialProxies[i]->asset->model();
		e2::RenderLayer layer = shaderModel->renderLayer();

		m_submeshIndex[layer].insert({ proxy, i });
	}

	return m_modelIds.create();
}

void e2::Session::unregisterMeshProxy(uint32_t id, e2::MeshProxy* proxy)
{
	m_modelIds.destroy(proxy->id);

	for (uint8_t i = 0; i < proxy->asset->submeshCount(); i++)
	{
		e2::ShaderModel* shaderModel = proxy->materialProxies[i]->asset->model();
		e2::RenderLayer layer = shaderModel->renderLayer();


		m_submeshIndex[layer].erase({ proxy, i });
	}

	m_meshProxies.erase(proxy);
}


void e2::Session::registerMaterialProxy(e2::MaterialProxy* proxy)
{
	m_materialProxies.insert(proxy);
}

void e2::Session::unregisterMaterialProxy(e2::MaterialProxy* proxy)
{
	m_materialProxies.erase(proxy);
}

std::map<e2::RenderLayer, std::unordered_set<e2::MeshProxySubmesh>> const& e2::Session::submeshIndex() const
{
	return m_submeshIndex;
}


e2::IDescriptorSet* e2::Session::getModelSet(uint8_t frameIndex)
{
	return m_modelSets[frameIndex];
}


e2::MaterialProxy* e2::Session::getOrCreateDefaultMaterialProxy(e2::MaterialPtr material)
{
	auto finder = m_defaultMaterialProxies.find(material.get());
	if (finder == m_defaultMaterialProxies.end())
	{
		e2::MaterialProxy* newProxy = material->model()->createMaterialProxy(this, material);
		m_defaultMaterialProxies[material.get()] = newProxy;
		return newProxy;
	}

	return finder->second;
}

void e2::Session::invalidateAllPipelines()
{

	std::unordered_set<e2::MeshProxy*> copy = m_meshProxies;
	for (e2::MeshProxy* proxy : copy)
	{
		proxy->invalidatePipeline();
	}
}

bool e2::MeshProxySubmesh::operator==(const MeshProxySubmesh& rhs) const
{
	return std::tie(proxy, submesh) == std::tie(rhs.proxy, rhs.submesh);
}

bool e2::MeshProxySubmesh::operator<(const MeshProxySubmesh& rhs) const
{
	return std::tie(proxy, submesh) < std::tie(rhs.proxy, rhs.submesh);
}
