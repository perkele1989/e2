
#include "e2/renderer/shadermodels/custom.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/game/gamesession.hpp"
#include "e2/assets/mesh.hpp"
#include "e2/assets/material.hpp"
#include "e2/renderer/meshproxy.hpp"

#include <e2/rhi/threadcontext.hpp>

#include "e2/utils.hpp"
#include "e2/renderer/shadermodels/custom.hpp"


#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

e2::CustomModel::CustomModel()
	: e2::ShaderModel()
{
	m_pipelineCache.resize(uint16_t(e2::CustomFlags::Count));
	for (uint16_t i = 0; i < uint16_t(e2::CustomFlags::Count); i++)
	{
		m_pipelineCache[i] = {};
	}
}


e2::CustomModel::~CustomModel()
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

	if(m_proxyUniformBuffers[0])
		e2::destroy(m_proxyUniformBuffers[0]);

	if(m_proxyUniformBuffers[1])
		e2::destroy(m_proxyUniformBuffers[1]);

	if(m_descriptorPool)
		e2::destroy(m_descriptorPool);

	if(m_pipelineLayout)
		e2::destroy(m_pipelineLayout);

	if(m_pipelineLayoutShadows)
		e2::destroy(m_pipelineLayoutShadows);

	if(m_descriptorSetLayout)
		e2::destroy(m_descriptorSetLayout);
}

void e2::CustomModel::source(std::string const& sourceFile)
{
	m_sourceFile = sourceFile;
}

void e2::CustomModel::postConstruct(e2::Context* ctx)
{
	e2::ShaderModel::postConstruct(ctx);




	std::ifstream unitFile(m_sourceFile);
	json data;
	try
	{
		data = json::parse(unitFile);

		if (!data.contains("name") || !data.contains("vertexShader") || !data.contains("fragmentShader"))
		{
			LogError("custom shader models require fields 'name', 'vertexShader', 'fragmentShader': {}", m_sourceFile);
			return;
		}

		m_name = data.at("name").template get<std::string>();
		m_vertexSourcePath = data.at("vertexShader").template get<std::string>();
		m_fragmentSourcePath = data.at("fragmentShader").template get<std::string>();

		if (data.contains("renderLayer"))
		{
			m_renderLayer = data.at("renderLayer").template get<uint64_t>();
		}

		if (data.contains("textures"))
		{
			for (json& texture : data.at("textures"))
			{
				if (!texture.contains("name"))
					continue;

				std::string textureName = texture.at("name").template get<std::string>();
				m_textureSlots.push(textureName);
			}
		}

		if (data.contains("parameters"))
		{
			for (json& parameter : data.at("parameters"))
			{
				if (!parameter.contains("name"))
					continue;

				std::string parameterName = parameter.at("name").template get<std::string>();
				m_parameterSlots.push(parameterName);
				if (parameter.contains("defaultValue"))
				{
					glm::vec4 newDefault{};
					newDefault.x = parameter.at("defaultValue")[0].template get<float>();
					newDefault.y = parameter.at("defaultValue")[1].template get<float>();
					newDefault.z = parameter.at("defaultValue")[2].template get<float>();
					newDefault.w = parameter.at("defaultValue")[3].template get<float>();
					m_parameterDefaults.push(newDefault);
				}
				else
				{
					m_parameterDefaults.push({});
				}

			}
		}

	}
	catch (json::parse_error& e)
	{
		LogError("failed to load specifications, json parse error: {}", e.what());
		return;
	}



	bool requiresUniformBuffer = m_parameterSlots.size() > 0;
	bool requiresDescriptorSet = requiresUniformBuffer || m_textureSlots.size() > 0;

	m_customDescriptorsString = "";

	if (requiresDescriptorSet)
	{
		m_customDescriptorsString += "#if !defined(Renderer_Shadow)\n";
		if (requiresUniformBuffer)
		{
			m_customDescriptorsString += "layout(set = MaterialSetIndex, binding = 0) uniform MaterialData\n";
			m_customDescriptorsString += "{\n";
			for (uint32_t i = 0; i < m_parameterSlots.size(); i++)
			{
				m_customDescriptorsString += std::format("    vec4 {};\n", m_parameterSlots[i]);
			}
			
			m_customDescriptorsString += "} material;\n";
		}

		for (uint32_t i = 0; i < m_textureSlots.size(); i++)
		{
			m_customDescriptorsString += std::format("layout(set = MaterialSetIndex, binding = {}) uniform texture2D customTexture_{};\n", i + (requiresUniformBuffer ? 1 : 0), m_textureSlots[i]);
		}

		m_customDescriptorsString += "#endif\n";
	}



	if (requiresDescriptorSet)
	{
		e2::DescriptorSetLayoutCreateInfo setLayoutCreateInfo{};


		if (requiresUniformBuffer)
			setLayoutCreateInfo.bindings.push({ e2::DescriptorBindingType::UniformBuffer , 1 });

		for (uint32_t i = 0; i < m_textureSlots.size(); i++)
		{
			setLayoutCreateInfo.bindings.push({ e2::DescriptorBindingType::Texture, 1 });
		}

		m_descriptorSetLayout = renderContext()->createDescriptorSetLayout(setLayoutCreateInfo);
	}


	e2::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sets = {
		renderManager()->rendererSetLayout(),
		renderManager()->modelSetLayout()
	};

	if (requiresDescriptorSet)
		pipelineLayoutCreateInfo.sets.push(m_descriptorSetLayout);

	pipelineLayoutCreateInfo.pushConstantSize = sizeof(e2::PushConstantData);
	m_pipelineLayout = renderContext()->createPipelineLayout(pipelineLayoutCreateInfo);

	e2::PipelineLayoutCreateInfo shadowsLayoutCreateInfo{};
	shadowsLayoutCreateInfo.sets = {
		renderManager()->modelSetLayout()
	};
	shadowsLayoutCreateInfo.pushConstantSize = sizeof(e2::ShadowPushConstantData);

	m_pipelineLayoutShadows = renderContext()->createPipelineLayout(shadowsLayoutCreateInfo);

	if (requiresDescriptorSet)
	{
		e2::DescriptorPoolCreateInfo poolCreateInfo{};
		poolCreateInfo.maxSets = e2::maxNumCustomProxies * 2;
		poolCreateInfo.numTextures = e2::maxNumCustomProxies * 2 * m_textureSlots.size();
		if(requiresUniformBuffer)
			poolCreateInfo.numUniformBuffers = e2::maxNumCustomProxies * 2;

		m_descriptorPool = mainThreadContext()->createDescriptorPool(poolCreateInfo);


		if (requiresUniformBuffer)
		{
			e2::DataBufferCreateInfo bufferCreateInfo{};
			bufferCreateInfo.dynamic = true;
			bufferCreateInfo.size = renderManager()->paddedBufferSize(e2::maxNumCustomProxies * getUniformBufferSize());
			bufferCreateInfo.type = BufferType::UniformBuffer;
			m_proxyUniformBuffers[0] = renderContext()->createDataBuffer(bufferCreateInfo);
			m_proxyUniformBuffers[1] = renderContext()->createDataBuffer(bufferCreateInfo);
		}
	}

	m_valid = true;
}

e2::MaterialProxy* e2::CustomModel::createMaterialProxy(e2::Session* session, e2::MaterialPtr material)
{
	e2::CustomProxy* newProxy = e2::create<e2::CustomProxy>(session, material);



	bool requiresUniformBuffer = m_parameterSlots.size() > 0;
	bool requiresDescriptorSet = requiresUniformBuffer || m_textureSlots.size() > 0;

	if (requiresDescriptorSet)
	{
		newProxy->sets[0] = m_descriptorPool->createDescriptorSet(m_descriptorSetLayout);
		newProxy->sets[1] = m_descriptorPool->createDescriptorSet(m_descriptorSetLayout);


		if (requiresUniformBuffer)
		{
			uint64_t uniformBufferSize = getUniformBufferSize();

			newProxy->sets[0]->writeUniformBuffer(0, m_proxyUniformBuffers[0], uniformBufferSize, renderManager()->paddedBufferSize(uniformBufferSize) * newProxy->id);
			newProxy->sets[1]->writeUniformBuffer(0, m_proxyUniformBuffers[1], uniformBufferSize, renderManager()->paddedBufferSize(uniformBufferSize) * newProxy->id);
		}

		e2::Texture2DPtr defaultTexture = renderManager()->defaultTexture();

		// set default textures
		for (uint32_t i = 0; i < m_textureSlots.size(); i++)
		{
			newProxy->textures.push({});
			newProxy->textures[newProxy->textures.size() - 1].set(defaultTexture);
			newProxy->sets[0]->writeTexture(i + (requiresUniformBuffer ? 1 : 0), defaultTexture->handle());
			newProxy->sets[1]->writeTexture(i + (requiresUniformBuffer ? 1 : 0), defaultTexture->handle());
		}

		// set material textures
		for (auto [name, texture] : material->textures())
		{
			int32_t slotId = getTextureSlot(name);
			if (slotId == -1)
				continue;

			newProxy->textures[slotId].set(texture);
			newProxy->sets[0]->writeTexture(slotId + (requiresUniformBuffer ? 1 : 0), texture->handle());
			newProxy->sets[1]->writeTexture(slotId + (requiresUniformBuffer ? 1 : 0), texture->handle());
		}

		// set default parameters
		for (glm::vec4& val : m_parameterDefaults)
		{
			newProxy->parameters.push({});
			newProxy->parameters[newProxy->parameters.size() - 1].set(val);
		}

		// set material parameters
		for (auto [name, value] : material->vectors())
		{
			int32_t parameterSlot = getParameterSlot(name);
			if (parameterSlot == -1)
				continue;

			newProxy->parameters[parameterSlot].set(value);
		}
		

	}

	return newProxy;
}

e2::IPipelineLayout* e2::CustomModel::getOrCreatePipelineLayout(e2::MeshProxy* proxy, uint8_t lodIndex, uint8_t submeshIndex, bool shadows)
{
	if (shadows)
		return m_pipelineLayoutShadows;
	else
		return m_pipelineLayout;
}

e2::IPipeline* e2::CustomModel::getOrCreatePipeline(e2::MeshProxy* proxy, uint8_t lodIndex, uint8_t submeshIndex, e2::RendererFlags rendererFlags)
{


	e2::SubmeshSpecification const& spec = proxy->lods[lodIndex].asset->specification(submeshIndex);
	e2::CustomProxy* lwProxy = static_cast<e2::CustomProxy*>(proxy->lods[lodIndex].materialProxies[submeshIndex]);
	e2::MaterialPtr material = lwProxy->asset;



	if (!m_shadersReadFromDisk)
	{
		m_shadersOnDiskOK = true;

		if (!e2::readFileWithIncludes(m_vertexSourcePath, m_vertexSource))
		{
			m_shadersOnDiskOK = false;
			LogError("failed to read vertex source from disk");
		}

		if (!e2::readFileWithIncludes(m_fragmentSourcePath, m_fragmentSource))
		{
			m_shadersOnDiskOK = false;
			LogError("failed to read fragment source from disk");
		}

		m_fragmentSource = e2::replace("%CustomDescriptorSets%", m_customDescriptorsString, m_fragmentSource);
		m_vertexSource = e2::replace("%CustomDescriptorSets%", m_customDescriptorsString, m_vertexSource);

		m_shadersReadFromDisk = true;
	}

	if (!m_shadersOnDiskOK)
	{
		return nullptr;
	}



	uint16_t geometryFlags = (uint16_t)spec.attributeFlags;

	uint16_t materialFlags = 0;
	for (uint32_t i = 0; i < m_textureSlots.size(); i++)
	{
		e2::Name textureName = m_textureSlots[i];
		materialFlags |= 1 << ((uint16_t)e2::CustomFlags::TextureFlagsOffset + i);
	}

	uint16_t lwFlagsInt = (geometryFlags << (uint16_t)e2::CustomFlags::VertexFlagsOffset)
		| (uint16_t(rendererFlags) << (uint16_t)e2::CustomFlags::RendererFlagsOffset)
		| (materialFlags);

	e2::CustomFlags lwFlags = (e2::CustomFlags)lwFlagsInt;

	if (m_pipelineCache[uint16_t(lwFlags)].pipeline)
	{
		return m_pipelineCache[uint16_t(lwFlags)].pipeline;
	}

	e2::CustomCacheEntry newEntry;

	e2::ShaderCreateInfo shaderInfo;

	e2::applyVertexAttributeDefines(spec.attributeFlags, shaderInfo);

	bool shadows = (lwFlags & e2::CustomFlags::Shadow) == e2::CustomFlags::Shadow;
	if (shadows)
		shaderInfo.defines.push({ "Renderer_Shadow", "1" });

	if ((lwFlags & e2::CustomFlags::Skin) == e2::CustomFlags::Skin)
		shaderInfo.defines.push({ "Renderer_Skin", "1" });

	for (uint32_t i = 0; i < m_textureSlots.size(); i++)
	{
		e2::Name textureName = m_textureSlots[i];
		if (lwProxy->textures[i].data() != nullptr)
		{
			shaderInfo.defines.push({ std::format("HasCustomTexture_{}", textureName), "1" });
		}
	}

	shaderInfo.stage = ShaderStage::Vertex;
	shaderInfo.source = m_vertexSource.c_str();
	newEntry.vertexShader = renderContext()->createShader(shaderInfo);

	shaderInfo.stage = ShaderStage::Fragment;
	shaderInfo.source = m_fragmentSource.c_str();
	newEntry.fragmentShader = renderContext()->createShader(shaderInfo);

	if (newEntry.vertexShader && newEntry.fragmentShader && newEntry.vertexShader->valid() && newEntry.fragmentShader->valid())
	{
		e2::PipelineCreateInfo pipelineInfo;
		if (shadows)
			pipelineInfo.layout = m_pipelineLayoutShadows;
		else
			pipelineInfo.layout = m_pipelineLayout;

		pipelineInfo.shaders = { newEntry.vertexShader, newEntry.fragmentShader };

		if (shadows)
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

bool e2::CustomModel::supportsShadows()
{
	return true;
}

void e2::CustomModel::invalidatePipelines()
{

	m_shadersReadFromDisk = false;
	m_shadersOnDiskOK = false;
	m_vertexSource.clear();
	m_fragmentSource.clear();


	for (uint16_t i = 0; i < uint16_t(e2::CustomFlags::Count); i++)
	{
		e2::CustomCacheEntry& entry = m_pipelineCache[i];

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

e2::Name e2::CustomModel::name()
{
	return m_name;
}

e2::RenderLayer e2::CustomModel::renderLayer()
{
	return (e2::RenderLayer)m_renderLayer;
}

bool e2::CustomModel::valid()
{
	return m_valid;
}

uint64_t e2::CustomModel::getUniformBufferSize()
{

	return m_parameterSlots.size() * sizeof(glm::vec4);
}

int32_t e2::CustomModel::getTextureSlot(e2::Name name)
{
	for (int32_t i = 0; i < m_textureSlots.size(); i++)
	{
		if (name == m_textureSlots[i])
			return i;
	}

	return -1;
}

int32_t e2::CustomModel::getParameterSlot(e2::Name name)
{
	for (int32_t i = 0; i < m_parameterSlots.size(); i++)
	{
		if (name == m_parameterSlots[i])
			return i;
	}

	return -1;
}

uint32_t e2::CustomModel::numParameters()
{
	return m_parameterSlots.size();
}

uint32_t e2::CustomModel::numTextures()
{
	return m_textureSlots.size();
}

e2::CustomProxy::CustomProxy(e2::Session* inSession, e2::MaterialPtr materialAsset)
	: e2::MaterialProxy(inSession, materialAsset)
{
	session->registerMaterialProxy(this);
	model = static_cast<e2::CustomModel*>(materialAsset->model());

	id = model->proxyIds.create();
}

e2::CustomProxy::~CustomProxy()
{
	session->unregisterMaterialProxy(this);
	model->proxyIds.destroy(id);

	sets[0]->keepAround();
	e2::destroy(sets[0]);

	sets[1]->keepAround();
	e2::destroy(sets[1]);
}

void e2::CustomProxy::bind(e2::ICommandBuffer* buffer, uint8_t frameIndex, bool shadows)
{
	if (!shadows)
		buffer->bindDescriptorSet(model->m_pipelineLayout, 2, sets[frameIndex]);
}

void e2::CustomProxy::invalidate(uint8_t frameIndex)
{
	bool requiresUniformBuffer = model->numParameters() > 0;
	if (requiresUniformBuffer)
	{
		e2::StackVector<glm::vec4, e2::maxNumCustomParameters> newBuffer;
		newBuffer.resize(parameters.size());

		bool requireWrite = false;
		for (uint32_t i = 0; i < parameters.size(); i++)
		{
			newBuffer[i] = parameters[i].data();

			if (parameters[i].invalidate(frameIndex))
			{
				requireWrite = true;
			}
		}

		if (requireWrite)
		{
			uint64_t uniformBufferSize = model->getUniformBufferSize();
			uint32_t proxyOffset = renderManager()->paddedBufferSize(uniformBufferSize) * id;
			model->m_proxyUniformBuffers[frameIndex]->upload(reinterpret_cast<uint8_t const*>(newBuffer.data()), uniformBufferSize, 0, proxyOffset);
		}
	}

	for (uint32_t i = 0; i < textures.size(); i++)
	{
		if (textures[i].invalidate(frameIndex))
		{
			e2::Texture2DPtr texture = textures[i].data();
			if (texture)
				sets[frameIndex]->writeTexture(i + (requiresUniformBuffer ? 1 : 0), texture->handle());
		}
	}
}