
#include "e2/renderer/meshproxy.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/renderer/shadermodel.hpp"
#include "e2/assets/mesh.hpp"
#include "e2/assets/material.hpp"
#include "e2/game/session.hpp"
#include "e2/transform.hpp"

#include <glm/gtx/matrix_decompose.hpp>

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
{
	session->m_meshProxies.insert(this);

	for (MeshLodConfiguration const& lod : config.lods)
	{
		MeshProxyLOD newLod;
		newLod.asset = lod.mesh;
		newLod.maxDistance = lod.maxDistance;
		if (lod.materials.size() > 0)
		{
			newLod.materialProxies = lod.materials;
		}
		else
		{
			for (uint8_t i = 0; i < lod.mesh->submeshCount(); i++)
			{
				e2::MaterialProxy* newMaterialProxy = session->getOrCreateDefaultMaterialProxy(lod.mesh->material(i));
				newLod.materialProxies.push(newMaterialProxy);
			}
		}
		lods.push(newLod);
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

	uint8_t lodIndex = 0;
	for (auto& lod : lods)
	{
		
		lod.pipelineLayouts.resize(lod.asset->submeshCount());
		lod.shadowPipelineLayouts.resize(lod.asset->submeshCount());
		lod.pipelines.resize(lod.asset->submeshCount());
		lod.shadowPipelines.resize(lod.asset->submeshCount());

		for (uint8_t submeshIndex = 0; submeshIndex < lod.asset->submeshCount(); submeshIndex++)
		{
			e2::ShaderModel* model = lod.materialProxies[submeshIndex]->asset->model();
			lod.pipelineLayouts[submeshIndex] = model->getOrCreatePipelineLayout(this, lodIndex, submeshIndex, false);
			lod.pipelines[submeshIndex] = model->getOrCreatePipeline(this, lodIndex, submeshIndex, skinProxy ? RendererFlags::Skin : e2::RendererFlags::None);

			if (model->supportsShadows())
			{
				lod.shadowPipelineLayouts[submeshIndex] = model->getOrCreatePipelineLayout(this, lodIndex, submeshIndex, true);
				lod.shadowPipelines[submeshIndex] = model->getOrCreatePipeline(this, lodIndex, submeshIndex, skinProxy ? RendererFlags::Skin | RendererFlags::Shadow : e2::RendererFlags::Shadow);
			}
			else
			{
				lod.shadowPipelineLayouts[submeshIndex] = nullptr;
				lod.shadowPipelines[submeshIndex] = nullptr;
			}

		}

		lodIndex++;
	}

	if (wasEnabled)
		enable();
}

e2::Engine* e2::MeshProxy::engine()
{
	return session->engine();
}

void e2::MeshProxy::setScale(float newScale)
{
	glm::vec3 translation, scale, skew;
	glm::vec4 perspective;
	glm::quat rotation;
	glm::decompose(modelMatrix, scale, rotation, translation, skew, perspective);
	scale = glm::vec3(newScale);
	modelMatrix = e2::recompose(translation, scale, skew, perspective, rotation);
	modelMatrixDirty = true;
}

void e2::MeshProxy::setPosition(glm::vec3 const& newPosition)
{
	glm::vec3 translation, scale, skew;
	glm::vec4 perspective;
	glm::quat rotation;
	glm::decompose(modelMatrix, scale, rotation, translation, skew, perspective);
	translation = newPosition;
	modelMatrix = e2::recompose(translation, scale, skew, perspective, rotation);
	modelMatrixDirty = true;
}

void e2::MeshProxy::setRotation(float newRotation)
{
	glm::vec3 translation, scale, skew;
	glm::vec4 perspective;
	glm::quat rotation;
	glm::decompose(modelMatrix, scale, rotation, translation, skew, perspective);
	rotation = glm::angleAxis(newRotation, e2::worldUpf());

	modelMatrix = e2::recompose(translation, scale, skew, perspective, rotation);
	modelMatrixDirty = true;
}

bool e2::MeshProxy::lodTest(uint8_t lod, float distance)
{
	for (uint8_t i = 0; i < lods.size(); i++)
	{
		if (distance <= lods[i].maxDistance || lods[i].maxDistance <= 0.0001f)
			return lod == i;
	}

	return false;
}

e2::MeshProxyLOD* e2::MeshProxy::lodByDistance(float distance)
{
	for (uint32_t i = 0; i < lods.size(); i++)
	{
		if (distance <= lods[i].maxDistance || lods[i].maxDistance <= 0.0001f)
			return &lods[i];
	}

	return nullptr;
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
