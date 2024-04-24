
#include "e2/managers/rendermanager.hpp"
#include "e2/application.hpp"
#include "e2/renderer/shadermodels/lightweight.hpp"
#include "e2/renderer/renderer.hpp"

#include "e2/rhi/rendercontext.hpp"
#include "e2/rhi/fence.hpp"
#include "e2/rhi/vertexlayout.hpp"
#include "e2/timer.hpp"

#include "e2/managers/gamemanager.hpp"

#include "e2/rhi/vk/vkrendercontext.hpp"

#include "e2/e2.hpp"
#include "e2/log.hpp"


uint32_t e2::RenderManager::paddedBufferSize(uint32_t originalSize)
{
	uint32_t minAlignment = m_renderContext->getCapabilities().minUniformBufferOffsetAlignment;
	if (minAlignment > 0)
		return (originalSize + minAlignment - 1) & ~(minAlignment - 1);

	return originalSize;
}


e2::IVertexLayout* e2::RenderManager::getOrCreateVertexLayout(e2::VertexAttributeFlags flags)
{
	std::scoped_lock lock(m_vertexLayoutCacheMutex);
	uint8_t index = uint8_t(flags);
	if (m_vertexLayoutCache[index])
		return m_vertexLayoutCache[index];

	uint32_t currentIndex = 0;
	e2::VertexLayoutCreateInfo createInfo{};

	auto addVec4 = [&currentIndex, &createInfo]() {
		e2::VertexLayoutAttribute newAttribute;
		newAttribute.bindingIndex = currentIndex++;
		newAttribute.format = VertexFormat::Vec4;
		newAttribute.offset = 0;
		createInfo.attributes.push(newAttribute);

		e2::VertexLayoutBinding newBinding;
		newBinding.rate = VertexRate::PerVertex;
		newBinding.stride = sizeof(glm::vec4);
		createInfo.bindings.push(newBinding);
	};

	auto addVec4u = [&currentIndex, &createInfo]() {
		e2::VertexLayoutAttribute newAttribute;
		newAttribute.bindingIndex = currentIndex++;
		newAttribute.format = VertexFormat::Vec4u;
		newAttribute.offset = 0;
		createInfo.attributes.push(newAttribute);

		e2::VertexLayoutBinding newBinding;
		newBinding.rate = VertexRate::PerVertex;
		newBinding.stride = sizeof(glm::uvec4);
		createInfo.bindings.push(newBinding);
	};

	// add position 
	addVec4();

	// add normal and tangent
	if (uint8_t(flags) & uint8_t(e2::VertexAttributeFlags::Normal))
	{
		addVec4();
		addVec4();
	}

	// add texcoords01 
	if (uint8_t(flags) & uint8_t(e2::VertexAttributeFlags::TexCoords01))
	{
		addVec4();
	}

	// add texcoords23 
	if (uint8_t(flags) & uint8_t(e2::VertexAttributeFlags::TexCoords23))
	{
		addVec4();
	}
	
	// add color 
	if (uint8_t(flags) & uint8_t(e2::VertexAttributeFlags::Color))
	{
		addVec4();
	}

	// add bone weights and ids
	if (uint8_t(flags) & uint8_t(e2::VertexAttributeFlags::Bones))
	{
		addVec4();
		addVec4u();
	}

	e2::IVertexLayout* newLayout = renderContext()->createVertexLayout(createInfo);
	m_vertexLayoutCache[index] = newLayout;
	return newLayout;
}

e2::ShaderModel* e2::RenderManager::getShaderModel(e2::Name modelName)
{
	for (uint32_t i = 0; i < m_shaderModels.size(); i++)
		if (m_shaderModels[i]->type()->fqn == modelName)
			return m_shaderModels[i];

	return nullptr;
}

e2::ISampler* e2::RenderManager::equirectSampler()
{
	return m_equirectSampler;
}

e2::ISampler* e2::RenderManager::clampSampler()
{
	return m_clampSampler;
}

e2::ISampler* e2::RenderManager::repeatSampler()
{
	return m_repeatSampler;
}

e2::Texture2DPtr e2::RenderManager::integratedBrdf()
{
	return m_integratedBrdf;
}

e2::Texture2DPtr e2::RenderManager::defaultTexture()
{
	return m_defaultTexture;
}

e2::FontPtr e2::RenderManager::defaultFont(e2::FontFace face)
{
	return m_defaultFont[size_t(face)];
}

e2::MaterialPtr e2::RenderManager::defaultMaterial()
{
	return m_defaultMaterial;
}

e2::IShader* e2::RenderManager::fullscreenTriangleShader()
{
	return m_fullscreenTriangleShader;
}

void e2::RenderManager::invalidatePipelines()
{
	// Invalidate all shader models, this effectively discards all current pipelines
	for (e2::ShaderModel* model : m_shaderModels)
	{
		model->invalidatePipelines();
	}

	// invalidate all mesh proxies across all sessions, this makes them fetch new pipelines from their respective shadermodel 
	for (e2::Session* session : gameManager()->allSessions())
	{
		session->invalidateAllPipelines();
	}
}

e2::RenderManager::RenderManager(Engine* owner)
	: e2::Manager(owner)
{
	m_vertexLayoutCache.resize(e2::maxNumAttributePermutations);
	for (uint32_t i = 0; i < e2::maxNumAttributePermutations; i++)
	{
		m_vertexLayoutCache[i] = nullptr;
	}

}

e2::RenderManager::~RenderManager()
{
	e2::destroy(m_mainThreadContext);
	e2::destroy(m_renderContext);
}

void e2::RenderManager::initialize()
{
	m_renderContext = new e2::IRenderContext_Vk(this, "appName");

	e2::ThreadContextCreateInfo threadCreateInfo{};
	m_mainThreadContext = m_renderContext->createThreadContext(threadCreateInfo);

	e2::FenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.createSignaled = true;
	m_fences[0] = m_renderContext->createFence(fenceCreateInfo);
	m_fences[1] = m_renderContext->createFence(fenceCreateInfo);

	e2::CommandPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.transient = true;
	m_framePools[0] = m_mainThreadContext->createCommandPool(poolCreateInfo);
	m_framePools[1] = m_mainThreadContext->createCommandPool(poolCreateInfo);

	e2::SamplerCreateInfo samplerInfo{};
	samplerInfo.filter = SamplerFilter::Anisotropic;
	samplerInfo.wrap = SamplerWrap::Clamp;
	m_clampSampler = m_renderContext->createSampler(samplerInfo);

	samplerInfo.filter = SamplerFilter::Anisotropic;
	samplerInfo.wrap = SamplerWrap::Repeat;
	m_repeatSampler = m_renderContext->createSampler(samplerInfo);

	samplerInfo.filter = SamplerFilter::Anisotropic;
	samplerInfo.wrap = SamplerWrap::Equirect;
	m_equirectSampler = m_renderContext->createSampler(samplerInfo);

	m_shadowSampler = m_renderContext->createShadowSampler();

	// Create model pool and sets
	e2::DescriptorPoolCreateInfo modelPoolCreateInfo{};
	modelPoolCreateInfo.maxSets = 2 * e2::maxNumSessions;
	modelPoolCreateInfo.numDynamicBuffers = 2 * e2::maxNumSessions;
	m_modelPool = mainThreadContext()->createDescriptorPool(modelPoolCreateInfo);

	e2::DescriptorSetLayoutCreateInfo modelSetLayoutInfo{};
	modelSetLayoutInfo.bindings.push({ e2::DescriptorBindingType::DynamicBuffer, 1 });
	modelSetLayoutInfo.bindings.push({ e2::DescriptorBindingType::DynamicBuffer, 1 });
	m_modelSetLayout = renderContext()->createDescriptorSetLayout(modelSetLayoutInfo);

	e2::DescriptorPoolCreateInfo rendererPoolCreateInfo{};
	rendererPoolCreateInfo.maxSets = 2 * e2::maxNumSessions * e2::maxNumRenderers;
	rendererPoolCreateInfo.numUniformBuffers = 2 * e2::maxNumSessions * e2::maxNumRenderers;
	rendererPoolCreateInfo.numSamplers = 2 * e2::maxNumSessions * e2::maxNumRenderers * 5;
	rendererPoolCreateInfo.numTextures = 2 * e2::maxNumSessions * e2::maxNumRenderers * 8;
	m_rendererPool = mainThreadContext()->createDescriptorPool(rendererPoolCreateInfo);

	e2::DescriptorSetLayoutCreateInfo rendererSetLayoutInfo{};
	rendererSetLayoutInfo.bindings.push({ e2::DescriptorBindingType::UniformBuffer, 1 });
	rendererSetLayoutInfo.bindings.push({ e2::DescriptorBindingType::Sampler, 1 }); // Clamp sampler
	rendererSetLayoutInfo.bindings.push({ e2::DescriptorBindingType::Sampler, 1 }); // Repeat sampler
	rendererSetLayoutInfo.bindings.push({ e2::DescriptorBindingType::Sampler, 1 }); // Equirect sampler
	rendererSetLayoutInfo.bindings.push({ e2::DescriptorBindingType::Texture, 1 }); // integrated brdf
	rendererSetLayoutInfo.bindings.push({ e2::DescriptorBindingType::Texture, 1 }); // irradiance 
	rendererSetLayoutInfo.bindings.push({ e2::DescriptorBindingType::Texture, 1 }); // radiance
	rendererSetLayoutInfo.bindings.push({ e2::DescriptorBindingType::Texture, 1 }); // frontbuffer color 
	rendererSetLayoutInfo.bindings.push({ e2::DescriptorBindingType::Texture, 1 }); // frontbuffer position
	rendererSetLayoutInfo.bindings.push({ e2::DescriptorBindingType::Texture, 1 }); // frontbuffer depth
	rendererSetLayoutInfo.bindings.push({ e2::DescriptorBindingType::Texture, 1 }); // outline texture
	rendererSetLayoutInfo.bindings.push({ e2::DescriptorBindingType::Texture, 1 }); // shadow map
	rendererSetLayoutInfo.bindings.push({ e2::DescriptorBindingType::Sampler, 1 }); // shadow sampler
	m_rendererSetLayout = renderContext()->createDescriptorSetLayout(rendererSetLayoutInfo);
	
	e2::PipelineLayoutCreateInfo lineCreateInfo{};
	lineCreateInfo.sets.push(m_rendererSetLayout);
	lineCreateInfo.pushConstantSize = sizeof(glm::vec4) * 3;
	m_linePipelineLayout = renderContext()->createPipelineLayout(lineCreateInfo);

	//e2::VertexLayoutCreateInfo nullInfo{};
	//m_nullVertexLayout = renderContext()->createVertexLayout(nullInfo);

	e2::Type* shaderModelType = e2::Type::fromName("e2::ShaderModel");
	std::set<e2::Type*> shaderModelTypes = shaderModelType->findChildTypes();
	for (e2::Type* currModel : shaderModelTypes)
	{
		if (currModel->isAbstract)
		{

			LogNotice("Skipped shadermodel as it's abstract/not fully implemented: {}", currModel->fqn);
			continue;
		}
			

		e2::Object* rawModel = currModel->create();
		e2::ShaderModel* asModel = rawModel->cast<e2::ShaderModel>();
		if (!asModel)
		{
			LogError("Reflection broken for shadermodel {}", currModel->name);
			e2::destroy(rawModel);
			continue;
		}

		asModel->postConstruct(this);

		m_shaderModels.push(asModel);

		LogNotice("Registered shadermodel: {}", currModel->fqn);
	}


	bool aljSuccess = true;
	e2::ALJDescription alj;
	aljSuccess &= assetManager()->prescribeALJ(alj, "engine/F_DefaultFont_Sans.e2a");
	aljSuccess &= assetManager()->prescribeALJ(alj, "engine/F_DefaultFont_Serif.e2a");
	aljSuccess &= assetManager()->prescribeALJ(alj, "engine/F_DefaultFont_Monospace.e2a");
	aljSuccess &= assetManager()->prescribeALJ(alj, "engine/T_DefaultTexture.e2a");
	aljSuccess &= assetManager()->prescribeALJ(alj, "engine/ibl_brdf_lut_linear.e2a");
	aljSuccess &= assetManager()->queueWaitALJ(alj);
	if (!aljSuccess)
	{
		LogError("Failed to load default assets, broken distribution.");
		return;
	}

	m_defaultFont[size_t(e2::FontFace::Sans)] = assetManager()->get("engine/F_DefaultFont_Sans.e2a").cast<e2::Font>();
	m_defaultFont[size_t(e2::FontFace::Serif)] = assetManager()->get("engine/F_DefaultFont_Serif.e2a").cast<e2::Font>();
	m_defaultFont[size_t(e2::FontFace::Monospace)] = assetManager()->get("engine/F_DefaultFont_Monospace.e2a").cast<e2::Font>();
	m_defaultTexture = assetManager()->get("engine/T_DefaultTexture.e2a").cast<e2::Texture2D>();
	m_integratedBrdf = assetManager()->get("engine/ibl_brdf_lut_linear.e2a").cast<e2::Texture2D>();
	/*
	e2::Moment start = e2::timeNow();
	std::u32string foo = U"ABCDEFGHIJKLMNOPQRSTUVWXYZÅÄÖabcdefghijklmnopqrstuvwxyzåäö0123456789!?_-.,;:*'^\"§½<>@#£¤$%€&/{([)]=}´`µ";
	for (uint32_t i : foo)
	{
		m_defaultFont->getSDFGlyph(i, e2::FontStyle::Regular);
		m_defaultFont->getSDFGlyph(i, e2::FontStyle::Bold);
		m_defaultFont->getSDFGlyph(i, e2::FontStyle::Italic);
		m_defaultFont->getSDFGlyph(i, e2::FontStyle::BoldItalic);
	}
	LogNotice("Seeding the default font took {}ms", start.durationSince().milliseconds());*/

	m_defaultMaterial = e2::create<e2::Material>();
	m_defaultMaterial->postConstruct(this, e2::UUID());
	m_defaultMaterial->overrideModel(getShaderModel("e2::LightweightModel"));
	
	std::string srcData;
	e2::readFileWithIncludes("shaders/fullscreen_triangle.vertex.glsl", srcData);
	e2::ShaderCreateInfo shaderInfo{};
	shaderInfo.stage = e2::ShaderStage::Vertex;
	shaderInfo.source = srcData.c_str();
	m_fullscreenTriangleShader = m_renderContext->createShader(shaderInfo);

}

void e2::RenderManager::shutdown()
{
	m_renderContext->waitIdle();

	e2::destroy(m_fullscreenTriangleShader);
	e2::destroy(m_defaultMaterial);

	e2::destroy(m_shadowSampler);
	e2::destroy(m_equirectSampler);
	e2::destroy(m_repeatSampler);
	e2::destroy(m_clampSampler);
	m_integratedBrdf = nullptr;
	m_defaultTexture = nullptr;
	m_defaultFont[0] = nullptr;
	m_defaultFont[1] = nullptr;
	m_defaultFont[2] = nullptr;

	for(e2::ShaderModel* model : m_shaderModels)
		e2::destroy(model);

	for (uint32_t i = 0; i < m_vertexLayoutCache.size(); i++)
	{
		if(m_vertexLayoutCache[i])
			e2::destroy(m_vertexLayoutCache[i]);

		m_vertexLayoutCache[i] = nullptr;
	}

	e2::destroy(m_modelSetLayout);
	e2::destroy(m_modelPool);

	e2::destroy(m_linePipelineLayout);
	//e2::destroy(m_nullVertexLayout);

	e2::destroy(m_rendererSetLayout);
	e2::destroy(m_rendererPool);

	e2::destroy(m_framePools[0]);
	e2::destroy(m_framePools[1]);

	e2::destroy(m_fences[0]);
	e2::destroy(m_fences[1]);


}

void e2::RenderManager::preUpdate(double deltaTime)
{

}

void e2::RenderManager::update(double deltaTime)
{
	m_renderContext->tick(deltaTime);

#if defined(E2_PROFILER)
	e2::Moment preWait;
#endif 

	// wait for this frames fence, and then reset it
	m_fences[m_frameIndex]->wait();

#if defined(E2_PROFILER)
	engine()->metrics().gpuWaitTimeUs[engine()->metrics().cursor] = (float)preWait.durationSince().microseconds();
#endif

	m_fences[m_frameIndex]->reset();

	m_framePools[m_frameIndex]->reset();

	// From here we are safe to record command buffers (including updating data and descriptor sets)
	m_queueOpen[m_frameIndex] = true;

	for (e2::RenderCallbacks* callbacks : m_callbacks)
	{
		callbacks->onNewFrame(m_frameIndex);
	}
}


e2::IRenderContext* e2::RenderManager::renderContext()
{
	return m_renderContext;
}

e2::IThreadContext* e2::RenderManager::mainThreadContext()
{
	return m_mainThreadContext;
}

void e2::RenderManager::queue(e2::ICommandBuffer* buffer, e2::ISemaphore* waitSemaphore, e2::ISemaphore* signalSemaphore)
{
#if defined(E2_DEVELOPMENT)
	if (!m_queueOpen[m_frameIndex])
	{
		LogError("queue not open for buffers, make sure to only record between e2::RenderManager::update() and e2::RenderManager::dispatch(), and verify your frame indices.");
		return;
	}
#endif

	if (m_queue.size() == maxNumQueuedBuffers)
	{
		LogError("maxNumQueuedBuffers reached, refuse to queue command buffer.");
		return;
	}

	e2::RenderSubmitInfo newInfo;
	newInfo.buffer = buffer;
	newInfo.signalSemaphore = signalSemaphore;
	newInfo.waitSemaphore = waitSemaphore;
	m_queue.push(newInfo);
}

void e2::RenderManager::dispatch()
{

	for (e2::RenderCallbacks* callbacks : m_callbacks)
	{
		callbacks->preDispatch(m_frameIndex);
	}

	m_queueOpen[m_frameIndex] = false;

	// Submit with this frames fence, then clear queue and swap frame
	m_renderContext->submitBuffers(m_queue.data(), (uint32_t)m_queue.size(), m_fences[m_frameIndex]);
	m_queue.clear();


	for (e2::RenderCallbacks* callbacks : m_callbacks)
	{
		callbacks->postDispatch(m_frameIndex);
	}

	m_frameIndex = m_frameIndex == 0 ? 1 : 0;
}

e2::ICommandPool* e2::RenderManager::framePool(uint8_t frameIndex)
{
	return m_framePools[frameIndex];
}

void e2::RenderManager::registerCallbacks(e2::RenderCallbacks* callbacks)
{
	if (!m_callbacks.addUnique(callbacks))
	{
		LogError("maxNumRenderCallbacks reached, callbacks won't be registered");
	}
}

void e2::RenderManager::unregisterCallbacks(e2::RenderCallbacks* callbacks)
{
	m_callbacks.removeFirstByValueFast(callbacks);
}

