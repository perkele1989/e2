
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
	session->m_meshProxies.insert(this);

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
	enable();

}

e2::MeshProxy::~MeshProxy()
{
	disable();
	session->m_meshProxies.erase(this);
}

bool e2::MeshProxy::enabled()
{
	return id != UINT32_MAX;
}

void e2::MeshProxy::enable()
{
	if (enabled())
		return;

	id = session->registerMeshProxy(this);
}

void e2::MeshProxy::disable()
{
	if (!enabled())
		return;

	session->unregisterMeshProxy(this);
	id = UINT32_MAX;
}

// this is never called for disabled proxies
void e2::MeshProxy::invalidatePipeline()
{
	bool wasEnabled = enabled();


	if (wasEnabled)
		disable();

	pipelineLayouts.resize(asset->submeshCount());
	shadowPipelineLayouts.resize(asset->submeshCount());
	pipelines.resize(asset->submeshCount());
	shadowPipelines.resize(asset->submeshCount());


	for (uint8_t submeshIndex = 0; submeshIndex < asset->submeshCount(); submeshIndex++)
	{
		e2::ShaderModel* model = materialProxies[submeshIndex]->asset->model();
		pipelineLayouts[submeshIndex] = model->getOrCreatePipelineLayout(this, submeshIndex, false);
		pipelines[submeshIndex] = model->getOrCreatePipeline(this, submeshIndex, skinProxy ? RendererFlags::Skin : e2::RendererFlags::None);

		if (model->supportsShadows())
		{
			shadowPipelineLayouts[submeshIndex] = model->getOrCreatePipelineLayout(this, submeshIndex, true);
			shadowPipelines[submeshIndex] = model->getOrCreatePipeline(this, submeshIndex, skinProxy ? RendererFlags::Skin | RendererFlags::Shadow: e2::RendererFlags::Shadow);
		}
		else
		{
			shadowPipelineLayouts[submeshIndex] = nullptr;
			shadowPipelines[submeshIndex] = nullptr;
		}

	}

	if (wasEnabled)
		enable();
}

e2::Engine* e2::MeshProxy::engine()
{
	return session->engine();
}

e2::SkinProxy::SkinProxy(e2::Session* inSession, e2::SkinProxyConfiguration const& config)
	: session(inSession)
	, skeletonAsset(config.skeleton)
	, meshAsset(config.mesh)
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
		uint32_t skeletonId = i++;
		e2::Bone* bone = skeletonAsset->boneById(skeletonId);
		int32_t meshId = meshAsset->boneIndexByName(bone->name);

		if(meshId >= 0)
			skin[meshId] = boneTransform;
	}

	skinDirty = true;
}
