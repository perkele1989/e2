
#include "game/components/staticmeshcomponent.hpp"
#include "game/entity.hpp"
#include "game/game.hpp"

#include <e2/game/gamesession.hpp>

e2::StaticMeshComponent::StaticMeshComponent(e2::StaticMeshSpecification* specification, e2::Entity* entity)
	: m_specification(specification)
	, m_entity(entity)
{
	e2::Session* session = m_entity->gameSession();

	if (m_specification->meshAsset)
	{
		e2::MeshProxyConfiguration proxyConf{};
		e2::MeshLodConfiguration lod;

		lod.mesh = m_specification->meshAsset;
		proxyConf.lods.push(lod);

		m_meshProxy = e2::create<e2::MeshProxy>(session, proxyConf);
	}

}

e2::StaticMeshComponent::~StaticMeshComponent()
{
	if (m_meshProxy)
		e2::destroy(m_meshProxy);
}

glm::mat4 e2::StaticMeshComponent::getScaleTransform()
{
	return glm::scale(glm::identity<glm::mat4>(), m_specification->scale);
}

void e2::StaticMeshComponent::updateVisibility()
{
	if (!m_meshProxy)
		return;

	bool proxyEnabled = m_meshProxy->enabled();
	bool inView = m_entity->isInView();

	if (inView && !proxyEnabled)
	{
		m_meshProxy->enable();
		applyTransform();
	}
	else if (!inView && proxyEnabled)
	{
		m_meshProxy->disable();
	}
}

void e2::StaticMeshComponent::applyTransform()
{
	if (m_meshProxy)
	{
		glm::mat4 heightOffset = glm::translate(glm::identity<glm::mat4>(), { 0.0f, m_heightOffset, 0.0f });
		m_meshProxy->modelMatrix = heightOffset * m_entity->getTransform()->getTransformMatrix(e2::TransformSpace::World) * getScaleTransform();
		m_meshProxy->modelMatrixDirty = true;
	}
}
void e2::StaticMeshComponent::applyCustomTransform(glm::mat4 const& transform)
{
	if (m_meshProxy)
	{
		m_meshProxy->modelMatrix = transform;
		m_meshProxy->modelMatrixDirty = true;
	}
}
void e2::StaticMeshSpecification::populate(nlohmann::json& obj, std::unordered_set<e2::Name>& deps)
{
	if (obj.contains("mesh"))
	{
		meshAssetName = obj.at("mesh").template get<std::string>();
		deps.insert(meshAssetName);
	}


	if (obj.contains("meshScale"))
	{
		scale.x = obj.at("meshScale")[0].template get<float>();
		scale.y = obj.at("meshScale")[1].template get<float>();
		scale.z = obj.at("meshScale")[2].template get<float>();
	}
}

void e2::StaticMeshSpecification::finalize(e2::GameContext* ctx)
{

	e2::AssetManager* am = ctx->game()->assetManager();

	if(meshAssetName.index() > 0)
		meshAsset = am->get(meshAssetName).cast<e2::Mesh>();
}
