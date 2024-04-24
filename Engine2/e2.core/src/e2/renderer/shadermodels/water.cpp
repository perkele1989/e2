
#include "e2/renderer/shadermodels/water.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/game/gamesession.hpp"
#include "e2/assets/mesh.hpp"
#include "e2/assets/material.hpp"
#include "e2/renderer/meshproxy.hpp"

#include <e2/rhi/threadcontext.hpp>

#include "e2/utils.hpp"


e2::WaterModel::WaterModel()
	: e2::ShaderModel()
{
	m_pipelineCache.resize(uint16_t(e2::WaterFlags::Count));
	for (uint16_t i = 0; i < uint16_t(e2::WaterFlags::Count); i++)
	{
		m_pipelineCache[i] = {};
	}
}

e2::WaterModel::~WaterModel()
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

void e2::WaterModel::postConstruct(e2::Context* ctx)
{
	e2::ShaderModel::postConstruct(ctx);
	m_specification.requiredAttributes = e2::VertexAttributeFlags::Normal | e2::VertexAttributeFlags::TexCoords01;

	e2::DescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
	setLayoutCreateInfo.bindings = {
		{ e2::DescriptorBindingType::UniformBuffer , 1}, // ubo params
		{ e2::DescriptorBindingType::Texture, 1}, // reflection cubemap
		{ e2::DescriptorBindingType::Texture, 2}, // visibility
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
	bufferCreateInfo.size = renderManager()->paddedBufferSize(e2::maxNumWaterProxies * sizeof(e2::WaterData));
	bufferCreateInfo.type = BufferType::UniformBuffer;
	m_proxyUniformBuffers[0] = renderContext()->createDataBuffer(bufferCreateInfo);
	m_proxyUniformBuffers[1] = renderContext()->createDataBuffer(bufferCreateInfo);

	//std::string cubemapName = "assets/lakeside_4k.e2a";
	//std::string cubemapName = "assets/the_sky_is_on_fire_4k.e2a";
	//std::string cubemapName = "assets/studio_small_03_4k.e2a";
	std::string cubemapName = "assets/kloofensky_rad.e2a";

	e2::ALJDescription aljDesc;
	assetManager()->prescribeALJ(aljDesc, cubemapName);
	assetManager()->queueWaitALJ(aljDesc);

	m_cubemap = assetManager()->get(cubemapName).cast<e2::Texture2D>();
}

e2::MaterialProxy* e2::WaterModel::createMaterialProxy(e2::Session* session, e2::MaterialPtr material)
{
	e2::WaterProxy* newProxy = e2::create<e2::WaterProxy>(session, material);

	for (uint8_t i = 0; i < 2; i++)
	{
		newProxy->sets[i] = m_descriptorPool->createDescriptorSet(m_descriptorSetLayout);
		newProxy->sets[i]->writeUniformBuffer(0, m_proxyUniformBuffers[i], sizeof(e2::WaterData), renderManager()->paddedBufferSize(sizeof(e2::WaterData)) * newProxy->id);
		newProxy->sets[i]->writeTexture(1, m_cubemap->handle());
	}

	e2::WaterData newData;
	newData.albedo = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	newProxy->uniformData.set(newData);

	return newProxy;
}

e2::IPipelineLayout* e2::WaterModel::getOrCreatePipelineLayout(e2::MeshProxy* proxy, uint8_t submeshIndex, bool shadows)
{
	if (shadows)
		return nullptr;
	else 
		return m_pipelineLayout;
}

e2::IPipeline* e2::WaterModel::getOrCreatePipeline(e2::MeshProxy* proxy, uint8_t submeshIndex, e2::RendererFlags rendererFlags)
{
	if (!m_shadersReadFromDisk)
	{
		m_shadersOnDiskOK = true;

		if (!e2::readFileWithIncludes("shaders/water/water.vertex.glsl", m_vertexSource))
		{
			m_shadersOnDiskOK = false;
			LogError("failed to read vertex source from disk");
		}

		if (!e2::readFileWithIncludes("shaders/water/water.fragment.glsl", m_fragmentSource))
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

	e2::SubmeshSpecification const& spec = proxy->asset->specification(submeshIndex);
	e2::WaterProxy* lwProxy = static_cast<e2::WaterProxy*>(proxy->materialProxies[submeshIndex]);

	uint16_t geometryFlags = (uint16_t)spec.attributeFlags;

	uint16_t materialFlags = 0;

	uint16_t lwFlagsInt = (geometryFlags << (uint16_t)e2::WaterFlags::VertexFlagsOffset)
		| (uint16_t(rendererFlags) << (uint16_t)e2::WaterFlags::RendererFlagsOffset)
		| (materialFlags);

	e2::WaterFlags lwFlags = (e2::WaterFlags)lwFlagsInt;

	if (m_pipelineCache[uint16_t(lwFlags)].vertexShader)
	{
		return m_pipelineCache[uint16_t(lwFlags)].pipeline;
	}

	e2::WaterCacheEntry newEntry;

	e2::ShaderCreateInfo shaderInfo; 

	e2::applyVertexAttributeDefines(spec.attributeFlags, shaderInfo);

	if ((lwFlags & e2::WaterFlags::Shadow) == e2::WaterFlags::Shadow)
		shaderInfo.defines.push({ "Renderer_Shadow", "1" });

	if ((lwFlags & e2::WaterFlags::Skin) == e2::WaterFlags::Skin)
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
		pipelineInfo.colorFormats = { e2::TextureFormat::RGBA8, e2::TextureFormat::RGBA32 };
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

void e2::WaterModel::invalidatePipelines()
{
	m_shadersReadFromDisk = false;
	m_shadersOnDiskOK = false;
	m_vertexSource.clear();
	m_fragmentSource.clear();


	for (uint16_t i = 0; i < uint16_t(e2::WaterFlags::Count); i++)
	{
		e2::WaterCacheEntry& entry = m_pipelineCache[i];

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

e2::RenderLayer e2::WaterModel::renderLayer()
{
	return RenderLayer::Water;
}

e2::WaterProxy::WaterProxy(e2::Session* inSession, e2::MaterialPtr materialAsset)
	: e2::MaterialProxy(inSession, materialAsset)
{
	// @todo dont use gamesession here, it uses the default game session and thus this breaks in PIE etc.
	// instead take a sesson as parametr when creating proxies or similar
	session->registerMaterialProxy(this);
	model = static_cast<e2::WaterModel*>(materialAsset->model());
	id = model->proxyIds.create();
}

e2::WaterProxy::~WaterProxy()
{
	session->unregisterMaterialProxy(this);
	model->proxyIds.destroy(id);

	sets[0]->keepAround();
	e2::destroy(sets[0]);

	sets[1]->keepAround();
	e2::destroy(sets[1]);
}

void e2::WaterProxy::bind(e2::ICommandBuffer* buffer, uint8_t frameIndex, bool shadows)
{
	if(!shadows)
		buffer->bindDescriptorSet(model->m_pipelineLayout, 2, sets[frameIndex]);
}

void e2::WaterProxy::invalidate(uint8_t frameIndex)
{
	if (uniformData.invalidate(frameIndex))
	{
		uint32_t proxyOffset = renderManager()->paddedBufferSize(sizeof(WaterData)) * id;
		model->m_proxyUniformBuffers[frameIndex]->upload(reinterpret_cast<uint8_t const*>(&uniformData.data()), sizeof(WaterData), 0, proxyOffset);
	}

	if (reflectionHdr.invalidate(frameIndex))
	{
		e2::ITexture* tex = reflectionHdr.data();
		if (tex)
			sets[frameIndex]->writeTexture(1, tex);
	}

	if (visibilityMasks[frameIndex].invalidate(frameIndex))
	{
		e2::ITexture* tex = visibilityMasks[frameIndex].data();
		if (tex)
			sets[frameIndex]->writeTexture(2, tex);
	}

}
