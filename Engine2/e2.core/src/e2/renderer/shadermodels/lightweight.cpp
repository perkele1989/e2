
#include "e2/renderer/shadermodels/lightweight.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/game/gamesession.hpp"
#include "e2/assets/mesh.hpp"
#include "e2/assets/material.hpp"
#include "e2/renderer/meshproxy.hpp"

#include <e2/rhi/threadcontext.hpp>

#include "e2/utils.hpp"


e2::LightweightModel::LightweightModel()
	: e2::ShaderModel()
{
	m_pipelineCache.resize(uint16_t(e2::LightweightFlags::Count));
	for (uint16_t i = 0; i < uint16_t(e2::LightweightFlags::Count); i++)
	{
		m_pipelineCache[i] = {};
	}
}

e2::LightweightModel::~LightweightModel()
{
	for (uint16_t i = 0; i < m_pipelineCache.size(); i++)
	{
		if (m_pipelineCache[i].vertexShader)
			e2::destroy(m_pipelineCache[i].vertexShader);
		if (m_pipelineCache[i].fragmentShader)
			e2::destroy(m_pipelineCache[i].fragmentShader);
		if (m_pipelineCache[i].pipeline)
			e2::destroy(m_pipelineCache[i].pipeline);
	}

	e2::destroy(m_proxyUniformBuffers[0]);
	e2::destroy(m_proxyUniformBuffers[1]);
	e2::destroy(m_descriptorPool);
	e2::destroy(m_pipelineLayout);
	e2::destroy(m_pipelineLayoutShadows);
	e2::destroy(m_descriptorSetLayout);
}

void e2::LightweightModel::postConstruct(e2::Context* ctx)
{
	e2::ShaderModel::postConstruct(ctx);
	m_specification.requiredAttributes = e2::VertexAttributeFlags::Normal | e2::VertexAttributeFlags::TexCoords01;

	e2::DescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
	setLayoutCreateInfo.bindings = {
		{ e2::DescriptorBindingType::UniformBuffer , 1}, // ubo params
		{ e2::DescriptorBindingType::Texture, 1}, // albedoTexture
		{ e2::DescriptorBindingType::Texture, 1}, // normalTexture
		{ e2::DescriptorBindingType::Texture, 1}, // roughnessTexture
		{ e2::DescriptorBindingType::Texture, 1}, // metalnessTexture
	};
	m_descriptorSetLayout = renderContext()->createDescriptorSetLayout(setLayoutCreateInfo);


	e2::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sets = {
		renderManager()->rendererSetLayout(),
		renderManager()->modelSetLayout(),
		m_descriptorSetLayout
	};
	pipelineLayoutCreateInfo.pushConstantSize = sizeof(e2::PushConstantData);
	m_pipelineLayout = renderContext()->createPipelineLayout(pipelineLayoutCreateInfo);

	e2::PipelineLayoutCreateInfo shadowsLayoutCreateInfo{};
	shadowsLayoutCreateInfo.sets = {
		renderManager()->modelSetLayout()
	};
	shadowsLayoutCreateInfo.pushConstantSize = sizeof(e2::ShadowPushConstantData);

	m_pipelineLayoutShadows = renderContext()->createPipelineLayout(shadowsLayoutCreateInfo);

	//m_pipelineCache.reserve(128);


	e2::DescriptorPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.maxSets = e2::maxNumLightweightProxies * 2;
	poolCreateInfo.numTextures = e2::maxNumLightweightProxies * 2 * 4;
	poolCreateInfo.numUniformBuffers = e2::maxNumLightweightProxies * 2 * 1;
	m_descriptorPool = mainThreadContext()->createDescriptorPool(poolCreateInfo);


	e2::DataBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.dynamic = true;
	bufferCreateInfo.size = renderManager()->paddedBufferSize(e2::maxNumLightweightProxies * sizeof(e2::LightweightData));
	bufferCreateInfo.type = BufferType::UniformBuffer;
	m_proxyUniformBuffers[0] = renderContext()->createDataBuffer(bufferCreateInfo);
	m_proxyUniformBuffers[1] = renderContext()->createDataBuffer(bufferCreateInfo);

}

e2::MaterialProxy* e2::LightweightModel::createMaterialProxy(e2::Session* session, e2::MaterialPtr material)
{
	e2::LightweightProxy *newProxy = e2::create<e2::LightweightProxy>(session, material);

	newProxy->alphaClip = material->hasDefine("ALPHACLIP");
	newProxy->doubleSided = material->hasDefine("DOUBLESIDED");

	auto albedoAsset = material->getTexture("albedo", nullptr);
	if(albedoAsset)
		newProxy->albedoTexture.set(albedoAsset->handle());

	auto normalAsset = material->getTexture("normal", nullptr);
	if(normalAsset)
		newProxy->normalTexture.set(normalAsset->handle());

	auto roughnessAsset = material->getTexture("roughness", nullptr);
	if(roughnessAsset)
		newProxy->roughnessTexture.set(roughnessAsset->handle());
	
	auto metalnessAsset = material->getTexture("metalness", nullptr);
	if(metalnessAsset)
		newProxy->metalnessTexture.set(metalnessAsset->handle());

	for (uint8_t i = 0; i < 2; i++)
	{
		newProxy->sets[i] = m_descriptorPool->createDescriptorSet(m_descriptorSetLayout);
		newProxy->sets[i]->writeUniformBuffer(0, m_proxyUniformBuffers[i], sizeof(e2::LightweightData), renderManager()->paddedBufferSize(sizeof(e2::LightweightData)) * newProxy->id);

		if(newProxy->albedoTexture.data())
			newProxy->sets[i]->writeTexture(1, newProxy->albedoTexture.data());

		if (newProxy->normalTexture.data())
			newProxy->sets[i]->writeTexture(2, newProxy->normalTexture.data());

		if (newProxy->roughnessTexture.data())
			newProxy->sets[i]->writeTexture(3, newProxy->roughnessTexture.data());

		if (newProxy->metalnessTexture.data())
			newProxy->sets[i]->writeTexture(4, newProxy->metalnessTexture.data());
	}

	e2::LightweightData newData;
	newData.albedo = material->getVec4("albedo", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	newData.rmxx = material->getVec4("rmxx", glm::vec4(0.05f, 0.0f, 0.0f, 0.0f));
	newData.emissive = material->getVec4("emissive", glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
	newProxy->uniformData.set(newData);

	return newProxy;
}

e2::IPipelineLayout* e2::LightweightModel::getOrCreatePipelineLayout(e2::MeshProxy* proxy, uint8_t lodIndex, uint8_t submeshIndex, bool shadows)
{
	if (shadows)
		return m_pipelineLayoutShadows;
	else 
		return m_pipelineLayout;
}

e2::IPipeline* e2::LightweightModel::getOrCreatePipeline(e2::MeshProxy* proxy, uint8_t lodIndex, uint8_t submeshIndex, e2::RendererFlags rendererFlags)
{

	if (!m_shadersReadFromDisk)
	{
		m_shadersOnDiskOK = true;

		if (!e2::readFileWithIncludes("shaders/lightweight/lightweight.vertex.glsl", m_vertexSource))
		{
			m_shadersOnDiskOK = false;
			LogError("failed to read vertex source from disk");
		}

		if (!e2::readFileWithIncludes("shaders/lightweight/lightweight.fragment.glsl", m_fragmentSource))
		{
			m_shadersOnDiskOK = false;
			LogError("failed to read fragment source from disk");
		}

		m_shadersReadFromDisk = true;
	}

	if (!m_shadersOnDiskOK)
	{
		return nullptr;
	}






	e2::SubmeshSpecification const &spec = proxy->lods[lodIndex].asset->specification(submeshIndex);
	e2::LightweightProxy* lwProxy = static_cast<e2::LightweightProxy*>(proxy->lods[lodIndex].materialProxies[submeshIndex]);

	uint16_t geometryFlags = (uint16_t)spec.attributeFlags;
	
	uint16_t materialFlags = 0;
	if (lwProxy->albedoTexture.data())
		materialFlags |= uint16_t(e2::LightweightFlags::AlbedoTexture);

	if (lwProxy->normalTexture.data())
		materialFlags |= uint16_t(e2::LightweightFlags::NormalTexture);

	if (lwProxy->roughnessTexture.data())
		materialFlags |= uint16_t(e2::LightweightFlags::RoughnessTexture);

	if (lwProxy->metalnessTexture.data())
		materialFlags |= uint16_t(e2::LightweightFlags::MetalnessTexture);

	if (lwProxy->alphaClip)
		materialFlags |= uint16_t(e2::LightweightFlags::AlphaClip);

	if (lwProxy->doubleSided)
		materialFlags |= uint16_t(e2::LightweightFlags::DoubleSided);

	uint16_t lwFlagsInt = (geometryFlags << (uint16_t)e2::LightweightFlags::VertexFlagsOffset)
		| (uint16_t(rendererFlags) << (uint16_t)e2::LightweightFlags::RendererFlagsOffset)
		| (materialFlags);

	e2::LightweightFlags lwFlags = (e2::LightweightFlags)lwFlagsInt;

	if (m_pipelineCache[uint16_t(lwFlags)].pipeline)
	{
		return m_pipelineCache[uint16_t(lwFlags)].pipeline;
	}
	/*auto finder = m_pipelineCache.find(lwFlags);
	if (finder != m_pipelineCache.end())
	{
		return finder->second.pipeline;
	}*/

	e2::LightweightCacheEntry newEntry;

	e2::ShaderCreateInfo shaderInfo;

	e2::applyVertexAttributeDefines(spec.attributeFlags, shaderInfo);

	bool shadows = (lwFlags & e2::LightweightFlags::Shadow) == e2::LightweightFlags::Shadow;
	if (shadows)
		shaderInfo.defines.push({ "Renderer_Shadow", "1" });

	if ((lwFlags & e2::LightweightFlags::Skin) == e2::LightweightFlags::Skin)
		shaderInfo.defines.push({ "Renderer_Skin", "1" });

	if ((lwFlags & e2::LightweightFlags::AlbedoTexture) == e2::LightweightFlags::AlbedoTexture)
		shaderInfo.defines.push({"Material_AlbedoTexture", "1"});

	if ((lwFlags & e2::LightweightFlags::RoughnessTexture) == e2::LightweightFlags::RoughnessTexture)
		shaderInfo.defines.push({ "Material_RoughnessTexture", "1" });

	if ((lwFlags & e2::LightweightFlags::MetalnessTexture) == e2::LightweightFlags::MetalnessTexture)
		shaderInfo.defines.push({ "Material_MetalnessTexture", "1" });

	if ((lwFlags & e2::LightweightFlags::NormalTexture) == e2::LightweightFlags::NormalTexture)
		shaderInfo.defines.push({ "Material_NormalTexture", "1" });

	if ((lwFlags & e2::LightweightFlags::AlphaClip) == e2::LightweightFlags::AlphaClip)
		shaderInfo.defines.push({ "Material_AlphaClip", "1" });

	if ((lwFlags & e2::LightweightFlags::DoubleSided) == e2::LightweightFlags::DoubleSided)
		shaderInfo.defines.push({ "Material_DoubleSided", "1" });

	shaderInfo.stage = ShaderStage::Vertex;
	shaderInfo.source = m_vertexSource.c_str();
	newEntry.vertexShader = renderContext()->createShader(shaderInfo);

	shaderInfo.stage = ShaderStage::Fragment;
	shaderInfo.source = m_fragmentSource.c_str();
	newEntry.fragmentShader = renderContext()->createShader(shaderInfo);

	if (newEntry.vertexShader && newEntry.fragmentShader && newEntry.vertexShader->valid() && newEntry.fragmentShader->valid())
	{
		e2::PipelineCreateInfo pipelineInfo;
		if(shadows)
			pipelineInfo.layout = m_pipelineLayoutShadows;
		else 
			pipelineInfo.layout = m_pipelineLayout;

		pipelineInfo.shaders = { newEntry.vertexShader, newEntry.fragmentShader };

		if(shadows)
			pipelineInfo.colorFormats = { };
		else 
			pipelineInfo.colorFormats = { e2::TextureFormat::RGBA32, e2::TextureFormat::RGBA32 };

		pipelineInfo.depthFormat = { e2::TextureFormat::D32 };
		newEntry.pipeline = renderContext()->createPipeline(pipelineInfo);
	}
	else
	{
		LogError("shader compilation failed for the given bitflags: {:b}", lwFlagsInt);
	}


	m_pipelineCache[uint16_t(lwFlags)] = newEntry;
	return newEntry.pipeline;
}

bool e2::LightweightModel::supportsShadows()
{
	return true;
}

void e2::LightweightModel::invalidatePipelines()
{

	m_shadersReadFromDisk = false;
	m_shadersOnDiskOK = false;
	m_vertexSource.clear();
	m_fragmentSource.clear();


	for (uint16_t i = 0; i < uint16_t(e2::LightweightFlags::Count); i++)
	{
		e2::LightweightCacheEntry& entry = m_pipelineCache[i];

		if (entry.vertexShader)
			e2::discard(entry.vertexShader);
		entry.vertexShader = nullptr;

		if (entry.fragmentShader)
			e2::discard(entry.fragmentShader);
		entry.fragmentShader = nullptr;

		if (entry.pipeline)
			e2::discard(entry.pipeline);
		entry.pipeline = nullptr;
	}

}

e2::LightweightProxy::LightweightProxy(e2::Session* inSession, e2::MaterialPtr materialAsset)
	: e2::MaterialProxy(inSession, materialAsset)
{
	// @todo dont use gamesession here, it uses the default game session and thus this breaks in PIE etc.
	// instead take a sesson as parametr when creating proxies or similar
	session->registerMaterialProxy(this);
	model = static_cast<e2::LightweightModel*>(materialAsset->model());
	id = model->proxyIds.create();
}

e2::LightweightProxy::~LightweightProxy()
{
	session->unregisterMaterialProxy(this);
	model->proxyIds.destroy(id);

	sets[0]->keepAround();
	e2::destroy(sets[0]);

	sets[1]->keepAround();
	e2::destroy(sets[1]);
}

void e2::LightweightProxy::bind(e2::ICommandBuffer* buffer, uint8_t frameIndex, bool shadows)
{
	if(!shadows)
		buffer->bindDescriptorSet(model->m_pipelineLayout, 2, sets[frameIndex]);
}

void e2::LightweightProxy::invalidate(uint8_t frameIndex)
{
	if (uniformData.invalidate(frameIndex))
	{
		uint32_t proxyOffset = renderManager()->paddedBufferSize(sizeof(LightweightData)) * id;
		model->m_proxyUniformBuffers[frameIndex]->upload(reinterpret_cast<uint8_t const*>(&uniformData.data()), sizeof(LightweightData), 0, proxyOffset);
	}

	if (albedoTexture.invalidate(frameIndex))
	{
		e2::ITexture* tex = albedoTexture.data();
		if(tex)
			sets[frameIndex]->writeTexture(1, tex);
	}
	if (normalTexture.invalidate(frameIndex))
	{
		e2::ITexture* tex = normalTexture.data();
		if (tex)
			sets[frameIndex]->writeTexture(2, tex);
	}
	if (roughnessTexture.invalidate(frameIndex))
	{
		e2::ITexture* tex = roughnessTexture.data();
		if (tex)
			sets[frameIndex]->writeTexture(3, tex);
	}
	if (metalnessTexture.invalidate(frameIndex))
	{
		e2::ITexture* tex = metalnessTexture.data();
		if (tex)
			sets[frameIndex]->writeTexture(4, tex);
	}
}
