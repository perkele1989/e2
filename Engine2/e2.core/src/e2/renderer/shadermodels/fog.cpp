
#include "e2/renderer/shadermodels/fog.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/game/gamesession.hpp"
#include "e2/assets/mesh.hpp"
#include "e2/assets/material.hpp"
#include "e2/renderer/meshproxy.hpp"

#include <e2/rhi/threadcontext.hpp>

#include "e2/utils.hpp"


e2::FogModel::FogModel()
	: e2::ShaderModel()
{
	m_pipelineCache.resize(uint16_t(e2::FogFlags::Count));
	for (uint16_t i = 0; i < uint16_t(e2::FogFlags::Count); i++)
	{
		m_pipelineCache[i] = {};
	}
}

e2::FogModel::~FogModel()
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
	e2::destroy(m_descriptorSetLayout);
}

void e2::FogModel::postConstruct(e2::Context* ctx)
{
	e2::ShaderModel::postConstruct(ctx);

	e2::DescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
	setLayoutCreateInfo.bindings = {
		{ e2::DescriptorBindingType::UniformBuffer , 1}, // ubo params
		{ e2::DescriptorBindingType::Texture, 1}, // visibilityMask
		{ e2::DescriptorBindingType::Texture, 2}, // irradianceHdr
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

	//m_pipelineCache.reserve(128);


	e2::DescriptorPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.maxSets = e2::maxNumWaterProxies * 2;
	poolCreateInfo.numTextures = e2::maxNumWaterProxies * 2 * 1;
	poolCreateInfo.numUniformBuffers = e2::maxNumWaterProxies * 2 * 1;
	m_descriptorPool = mainThreadContext()->createDescriptorPool(poolCreateInfo);


	e2::DataBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.dynamic = true;
	bufferCreateInfo.size = renderManager()->paddedBufferSize(e2::maxNumWaterProxies * sizeof(e2::FogData));
	bufferCreateInfo.type = BufferType::UniformBuffer;
	m_proxyUniformBuffers[0] = renderContext()->createDataBuffer(bufferCreateInfo);
	m_proxyUniformBuffers[1] = renderContext()->createDataBuffer(bufferCreateInfo);

	e2::Name cubemapName = "T_Courtyard_Irradiance.e2a";

	e2::ALJDescription aljDesc;
	assetManager()->prescribeALJ(aljDesc, cubemapName);
	assetManager()->queueWaitALJ(aljDesc);

	m_cubemap = assetManager()->get(cubemapName).cast<e2::Texture2D>();

}

e2::MaterialProxy* e2::FogModel::createMaterialProxy(e2::Session* session, e2::MaterialPtr material)
{
	e2::FogProxy* newProxy = e2::create<e2::FogProxy>(session, material);

	for (uint8_t i = 0; i < 2; i++)
	{
		newProxy->sets[i] = m_descriptorPool->createDescriptorSet(m_descriptorSetLayout);
		newProxy->sets[i]->writeUniformBuffer(0, m_proxyUniformBuffers[i], sizeof(e2::FogData), renderManager()->paddedBufferSize(sizeof(e2::FogData)) * newProxy->id);
		//newProxy->sets[i]->writeTexture(1, m_visibilityMask);
		newProxy->sets[i]->writeTexture(2, m_cubemap->handle());
	}

	e2::FogData newData;
	newData.albedo = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
	newProxy->uniformData.set(newData);
	newProxy->irradianceHdr.set(m_cubemap->handle());

	return newProxy;
}

e2::IPipelineLayout* e2::FogModel::getOrCreatePipelineLayout(e2::MeshProxy* proxy, uint8_t lodIndex, uint8_t submeshIndex, bool shadows)
{
	return m_pipelineLayout;
}

e2::IPipeline* e2::FogModel::getOrCreatePipeline(e2::MeshProxy* proxy, uint8_t lodIndex, uint8_t submeshIndex, e2::RendererFlags rendererFlags)
{
	if (!m_shadersReadFromDisk)
	{
		m_shadersOnDiskOK = true;

		if (!e2::readFileWithIncludes("shaders/fog/fog.vertex.glsl", m_vertexSource))
		{
			m_shadersOnDiskOK = false;
			LogError("failed to read vertex source from disk");
		}

		if (!e2::readFileWithIncludes("shaders/fog/fog.fragment.glsl", m_fragmentSource))
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

	e2::SubmeshSpecification const& spec = proxy->lods[lodIndex].asset->specification(submeshIndex);
	e2::FogProxy* lwProxy = static_cast<e2::FogProxy*>(proxy->lods[lodIndex].materialProxies[submeshIndex]);

	uint16_t geometryFlags = (uint16_t)spec.attributeFlags;

	uint16_t materialFlags = 0;

	uint16_t lwFlagsInt = (geometryFlags << (uint16_t)e2::FogFlags::VertexFlagsOffset)
		| (uint16_t(rendererFlags) << (uint16_t)e2::FogFlags::RendererFlagsOffset)
		| (materialFlags);

	e2::FogFlags lwFlags = (e2::FogFlags)lwFlagsInt;

	if (m_pipelineCache[uint16_t(lwFlags)].vertexShader)
	{
		return m_pipelineCache[uint16_t(lwFlags)].pipeline;
	}

	e2::FogCacheEntry newEntry;

	e2::ShaderCreateInfo shaderInfo; 

	e2::applyVertexAttributeDefines(spec.attributeFlags, shaderInfo);

	if ((lwFlags & e2::FogFlags::Shadow) == e2::FogFlags::Shadow)
		shaderInfo.defines.push({ "Renderer_Shadow", "1" });

	if ((lwFlags & e2::FogFlags::Skin) == e2::FogFlags::Skin)
		shaderInfo.defines.push({ "Renderer_Skin", "1" });

	shaderInfo.stage = ShaderStage::Vertex;
	shaderInfo.source = m_vertexSource.c_str();
	newEntry.vertexShader = renderContext()->createShader(shaderInfo);

	shaderInfo.stage = ShaderStage::Fragment;
	shaderInfo.source = m_fragmentSource.c_str();
	newEntry.fragmentShader = renderContext()->createShader(shaderInfo);

	if (newEntry.vertexShader && newEntry.fragmentShader && newEntry.vertexShader->valid() && newEntry.fragmentShader->valid())
	{
		e2::PipelineCreateInfo pipelineInfo;
		pipelineInfo.layout = m_pipelineLayout;
		pipelineInfo.shaders = { newEntry.vertexShader, newEntry.fragmentShader };
		pipelineInfo.colorFormats = { e2::TextureFormat::RGBA32, e2::TextureFormat::RGBA32 };
		pipelineInfo.depthFormat = { e2::TextureFormat::D32 };
		pipelineInfo.alphaBlending = true;
		newEntry.pipeline = renderContext()->createPipeline(pipelineInfo);
	}
	else
	{
		LogError("shader compilation failed for the given bitflags: {:b}", lwFlagsInt);
	}

	m_pipelineCache[uint16_t(lwFlags)] = newEntry;
	return newEntry.pipeline;
}

void e2::FogModel::invalidatePipelines()
{
	m_shadersReadFromDisk = false;
	m_shadersOnDiskOK = false;
	m_vertexSource.clear();
	m_fragmentSource.clear();


	for (uint16_t i = 0; i < uint16_t(e2::FogFlags::Count); i++)
	{
		e2::FogCacheEntry& entry = m_pipelineCache[i];

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

e2::RenderLayer e2::FogModel::renderLayer()
{
	return RenderLayer::Fog;
}

e2::FogProxy::FogProxy(e2::Session* inSession, e2::MaterialPtr materialAsset)
	: e2::MaterialProxy(inSession, materialAsset)
{
	// @todo dont use gamesession here, it uses the default game session and thus this breaks in PIE etc.
	// instead take a sesson as parametr when creating proxies or similar
	session->registerMaterialProxy(this);
	model = static_cast<e2::FogModel*>(materialAsset->model());
	id = model->proxyIds.create();
}

e2::FogProxy::~FogProxy()
{
	session->unregisterMaterialProxy(this);
	model->proxyIds.destroy(id);

	e2::discard(sets[0]);
	e2::discard(sets[1]);
}

void e2::FogProxy::bind(e2::ICommandBuffer* buffer, uint8_t frameIndex, bool shadows)
{
	if(!shadows)
		buffer->bindDescriptorSet(model->m_pipelineLayout, 2, sets[frameIndex]);
}

void e2::FogProxy::invalidate(uint8_t frameIndex)
{
	if (uniformData.invalidate(frameIndex))
	{
		uint32_t proxyOffset = renderManager()->paddedBufferSize(sizeof(FogData)) * id;
		model->m_proxyUniformBuffers[frameIndex]->upload(reinterpret_cast<uint8_t const*>(&uniformData.data()), sizeof(FogData), 0, proxyOffset);
	}

	if (visibilityMasks[frameIndex].invalidate(frameIndex))
	{
		e2::ITexture* tex = visibilityMasks[frameIndex].data();
		if (tex)
			sets[frameIndex]->writeTexture(1, tex);
	}


	if (irradianceHdr.invalidate(frameIndex))
	{
		e2::ITexture* tex = irradianceHdr.data();
		if (tex)
			sets[frameIndex]->writeTexture(2, tex);
	}

}
