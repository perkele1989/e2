
#include "e2/renderer/meshproxy.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/game/meshcomponent.hpp"
#include "e2/renderer/shadermodel.hpp"
#include "e2/assets/mesh.hpp"
#include "e2/assets/material.hpp"
#include "e2/game/session.hpp"


e2::MaterialProxy::MaterialProxy(e2::Session* inSession, e2::MaterialPtr materialAsset)
{
	session = inSession;
	asset = materialAsset;
}

e2::MaterialProxy::~MaterialProxy()
{
	asset = nullptr;
}

e2::Engine* e2::MaterialProxy::engine()
{
	return asset->engine();
}

e2::MeshProxy::MeshProxy(e2::Session* inSession, e2::MeshProxyConfiguration const& config)
	: session{ inSession }
	, asset(config.mesh)
{
	if (config.materials.size() > 0)
	{
		materialProxies = config.materials;
	}
	else
	{
		for (uint8_t i = 0; i < config.mesh->submeshCount(); i++)
		{
			e2::MaterialProxy* newMaterialProxy = session->getOrCreateDefaultMaterialProxy(config.mesh->material(i));
			materialProxies.push(newMaterialProxy);
		}
	}

	modelMatrix = glm::mat4(1.0f);


	invalidatePipeline();

}

e2::MeshProxy::~MeshProxy()
{
	if (id != UINT32_MAX)
	{
		session->unregisterMeshProxy(this);
	}
}

void e2::MeshProxy::invalidatePipeline()
{
	if (id != UINT32_MAX)
	{
		session->unregisterMeshProxy(this);
		id = UINT32_MAX;
	}

	pipelineLayouts.resize(asset->submeshCount());
	pipelines.resize(asset->submeshCount());
	for (uint8_t submeshIndex = 0; submeshIndex < asset->submeshCount(); submeshIndex++)
	{
		e2::ShaderModel* model = materialProxies[submeshIndex]->asset->model();
		pipelineLayouts[submeshIndex] = model->getOrCreatePipelineLayout(this, submeshIndex);

		pipelines[submeshIndex] = model->getOrCreatePipeline(this, submeshIndex, skinProxy ? RendererFlags::Skin : e2::RendererFlags::None);
	}

	id = session->registerMeshProxy(this);
}

e2::Engine* e2::MeshProxy::engine()
{
	return session->engine();
}

e2::SkinProxy::SkinProxy(e2::Session* inSession, e2::SkinProxyConfiguration const& config)
	: session(inSession)
	, asset(config.skeleton)
{
	id = session->registerSkinProxy(this);
}

e2::SkinProxy::~SkinProxy()
{
	if (id != UINT32_MAX)
	{
		session->unregisterSkinProxy(this);
	}
}

e2::Engine* e2::SkinProxy::engine()
{
	return session->engine();
}

void e2::SkinProxy::applyPose(e2::Pose* pose)
{
	uint32_t i = 0;
	for (glm::mat4 const& boneTransform : pose->skin())
	{
		skin[i++] = boneTransform;
	}

	skinDirty = true;
}
