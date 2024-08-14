
#include "e2/renderer/shadermodels/terrain.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/game/gamesession.hpp"
#include "e2/assets/mesh.hpp"
#include "e2/assets/material.hpp"
#include "e2/renderer/meshproxy.hpp"

#include <e2/rhi/threadcontext.hpp>

#include "e2/utils.hpp"


e2::TerrainModel::TerrainModel()
	: e2::ShaderModel()
{
	m_pipelineCache.resize(uint16_t(e2::TerrainFlags::Count));
	for (uint16_t i = 0; i < uint16_t(e2::TerrainFlags::Count); i++)
	{
		m_pipelineCache[i] = {};
	}
}

e2::TerrainModel::~TerrainModel()
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

void e2::TerrainModel::postConstruct(e2::Context* ctx)
{
	e2::ShaderModel::postConstruct(ctx);
	m_specification.requiredAttributes = e2::VertexAttributeFlags::Normal | e2::VertexAttributeFlags::TexCoords01;

	e2::DescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
	setLayoutCreateInfo.bindings = {
		{ e2::DescriptorBindingType::UniformBuffer , 1}, // ubo params
		{ e2::DescriptorBindingType::Texture, 1}, // mountainAlbedo
		{ e2::DescriptorBindingType::Texture, 1}, // mountainNormal
		{ e2::DescriptorBindingType::Texture, 1}, // sandAlbedo
		{ e2::DescriptorBindingType::Texture, 1}, // sandNormal
		{ e2::DescriptorBindingType::Texture, 1}, // greenAlbedo
		{ e2::DescriptorBindingType::Texture, 1}, // greenNormal
		
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
	poolCreateInfo.maxSets = e2::maxNumTerrainProxies * 2;
	poolCreateInfo.numTextures = e2::maxNumTerrainProxies * 2 * 6;
	poolCreateInfo.numUniformBuffers = e2::maxNumTerrainProxies * 2 * 1;
	m_descriptorPool = mainThreadContext()->createDescriptorPool(poolCreateInfo);


	e2::DataBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.dynamic = true;
	bufferCreateInfo.size = renderManager()->paddedBufferSize(e2::maxNumTerrainProxies * sizeof(e2::TerrainData));
	bufferCreateInfo.type = BufferType::UniformBuffer;
	m_proxyUniformBuffers[0] = renderContext()->createDataBuffer(bufferCreateInfo);
	m_proxyUniformBuffers[1] = renderContext()->createDataBuffer(bufferCreateInfo);

	//std::string cubemapName = "assets/lakeside_4k.e2a";
	//std::string cubemapName = "assets/the_sky_is_on_fire_4k.e2a";
	//std::string cubemapName = "assets/studio_small_03_4k.e2a";
	
	std::string mountainAlbedoPath = "assets/Landscape_albedo_srgb.e2a";
	std::string mountainNormalPath = "assets/Landscape_normal_linear.e2a";

	std::string sandAlbedoPath = "assets/T_SandDesert_Albedo.e2a";
	std::string sandNormalPath = "assets/T_SandDesert_Normal.e2a";

	//std::string greenAlbedoPath = "assets/T_Greenlands_Albedo.e2a";
	std::string greenAlbedoPath = "assets/environment/T_Greenlands_Albedo.e2a";
	std::string greenNormalPath = "assets/T_Greenlands_Normal.e2a";

	e2::ALJDescription aljDesc;
	assetManager()->prescribeALJ(aljDesc, mountainAlbedoPath);
	assetManager()->prescribeALJ(aljDesc, mountainNormalPath);
	assetManager()->prescribeALJ(aljDesc, sandAlbedoPath);
	assetManager()->prescribeALJ(aljDesc, sandNormalPath);
	assetManager()->prescribeALJ(aljDesc, greenAlbedoPath);
	assetManager()->prescribeALJ(aljDesc, greenNormalPath);
	assetManager()->queueWaitALJ(aljDesc);

	m_mountainAlbedo = assetManager()->get(mountainAlbedoPath).cast<e2::Texture2D>();
	m_mountainNormal = assetManager()->get(mountainNormalPath).cast<e2::Texture2D>();

	m_sandAlbedo = assetManager()->get(sandAlbedoPath).cast<e2::Texture2D>();
	m_sandNormal = assetManager()->get(sandNormalPath).cast<e2::Texture2D>();

	m_greenAlbedo = assetManager()->get(greenAlbedoPath).cast<e2::Texture2D>();
	m_greenNormal = assetManager()->get(greenNormalPath).cast<e2::Texture2D>();
}

e2::MaterialProxy* e2::TerrainModel::createMaterialProxy(e2::Session* session, e2::MaterialPtr material)
{
	e2::TerrainProxy* newProxy = e2::create<e2::TerrainProxy>(session, material);

	for (uint8_t i = 0; i < 2; i++)
	{
		newProxy->sets[i] = m_descriptorPool->createDescriptorSet(m_descriptorSetLayout);
		newProxy->sets[i]->writeUniformBuffer(0, m_proxyUniformBuffers[i], sizeof(e2::TerrainData), renderManager()->paddedBufferSize(sizeof(e2::TerrainData)) * newProxy->id);

		newProxy->sets[i]->writeTexture(1, m_mountainAlbedo->handle());
		newProxy->sets[i]->writeTexture(2, m_mountainNormal->handle());

		newProxy->sets[i]->writeTexture(3, m_sandAlbedo->handle());
		newProxy->sets[i]->writeTexture(4, m_sandNormal->handle());
		
		newProxy->sets[i]->writeTexture(5, m_greenAlbedo->handle());
		newProxy->sets[i]->writeTexture(6, m_greenNormal->handle());

	}

	e2::TerrainData newData;
	newData.albedo = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	newProxy->uniformData.set(newData);

	return newProxy;
}

e2::IPipelineLayout* e2::TerrainModel::getOrCreatePipelineLayout(e2::MeshProxy* proxy, uint8_t lodIndex, uint8_t submeshIndex, bool shadows)
{
	if(shadows)
		return m_pipelineLayoutShadows;
	else 
		return m_pipelineLayout;
}

e2::IPipeline* e2::TerrainModel::getOrCreatePipeline(e2::MeshProxy* proxy, uint8_t lodIndex, uint8_t submeshIndex, e2::RendererFlags rendererFlags)
{
	if (!m_shadersReadFromDisk)
	{
		m_shadersOnDiskOK = true;

		if (!e2::readFileWithIncludes("shaders/terrain/terrain.vertex.glsl", m_vertexSource))
		{
			m_shadersOnDiskOK = false;
			LogError("failed to read vertex source from disk");
		}

		if (!e2::readFileWithIncludes("shaders/terrain/terrain.fragment.glsl", m_fragmentSource))
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
	e2::TerrainProxy* lwProxy = static_cast<e2::TerrainProxy*>(proxy->lods[lodIndex].materialProxies[submeshIndex]);

	uint16_t geometryFlags = (uint16_t)spec.attributeFlags;

	uint16_t materialFlags = 0;

	uint16_t lwFlagsInt = (geometryFlags << (uint16_t)e2::TerrainFlags::VertexFlagsOffset)
		| (uint16_t(rendererFlags) << (uint16_t)e2::TerrainFlags::RendererFlagsOffset)
		| (materialFlags);

	e2::TerrainFlags lwFlags = (e2::TerrainFlags)lwFlagsInt;

	if (m_pipelineCache[uint16_t(lwFlags)].vertexShader)
	{
		return m_pipelineCache[uint16_t(lwFlags)].pipeline;
	}

	e2::TerrainCacheEntry newEntry;

	e2::ShaderCreateInfo shaderInfo; 

	e2::applyVertexAttributeDefines(spec.attributeFlags, shaderInfo);

	bool hasShadows = (lwFlags & e2::TerrainFlags::Shadow) == e2::TerrainFlags::Shadow;
	if (hasShadows)
		shaderInfo.defines.push({ "Renderer_Shadow", "1" });

	if ((lwFlags & e2::TerrainFlags::Skin) == e2::TerrainFlags::Skin)
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
		
		if(hasShadows)
			pipelineInfo.layout = m_pipelineLayoutShadows;
		else 
			pipelineInfo.layout = m_pipelineLayout;

		pipelineInfo.shaders = { newEntry.vertexShader, newEntry.fragmentShader };

		if(hasShadows)
			pipelineInfo.colorFormats = { };
		else 
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

void e2::TerrainModel::invalidatePipelines()
{
	m_shadersReadFromDisk = false;
	m_shadersOnDiskOK = false;
	m_vertexSource.clear();
	m_fragmentSource.clear();

	for (uint16_t i = 0; i < uint16_t(e2::TerrainFlags::Count);i++)
	{
		e2::TerrainCacheEntry &entry = m_pipelineCache[i];

		if(entry.vertexShader)
			e2::discard(entry.vertexShader);
		entry.vertexShader = nullptr;

		if(entry.fragmentShader)
			e2::discard(entry.fragmentShader);
		entry.fragmentShader = nullptr;

		if(entry.pipeline)
			e2::discard(entry.pipeline);
		entry.pipeline = nullptr;
	}
}

bool e2::TerrainModel::supportsShadows()
{
	return true;
}

e2::TerrainProxy::TerrainProxy(e2::Session* inSession, e2::MaterialPtr materialAsset)
	: e2::MaterialProxy(inSession, materialAsset)
{
	// @todo dont use gamesession here, it uses the default game session and thus this breaks in PIE etc.
	// instead take a sesson as parametr when creating proxies or similar
	session->registerMaterialProxy(this);
	model = static_cast<e2::TerrainModel*>(materialAsset->model());
	id = model->proxyIds.create();
}

e2::TerrainProxy::~TerrainProxy()
{
	session->unregisterMaterialProxy(this);
	model->proxyIds.destroy(id);

	sets[0]->keepAround();
	e2::destroy(sets[0]);

	sets[1]->keepAround();
	e2::destroy(sets[1]);
}

void e2::TerrainProxy::bind(e2::ICommandBuffer* buffer, uint8_t frameIndex, bool shadows)
{
	if(!shadows)
		buffer->bindDescriptorSet(model->m_pipelineLayout, 2, sets[frameIndex]);
}

void e2::TerrainProxy::invalidate(uint8_t frameIndex)
{
	if (uniformData.invalidate(frameIndex))
	{
		uint32_t proxyOffset = renderManager()->paddedBufferSize(sizeof(TerrainData)) * id;
		model->m_proxyUniformBuffers[frameIndex]->upload(reinterpret_cast<uint8_t const*>(&uniformData.data()), sizeof(TerrainData), 0, proxyOffset);
	}
}
