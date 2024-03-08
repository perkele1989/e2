
#include "e2/rhi/vk/vkrendercontext.hpp"

#include "e2/rhi/vk/vkwindow.hpp"
#include "e2/rhi/vk/vkthreadcontext.hpp"
#include "e2/rhi/vk/vkfence.hpp"
#include "e2/rhi/vk/vksemaphore.hpp"
#include "e2/rhi/vk/vkdatabuffer.hpp"
#include "e2/rhi/vk/vktexture.hpp"
#include "e2/rhi/vk/vkshader.hpp"
#include "e2/rhi/vk/vkpipeline.hpp"
#include "e2/rhi/vk/vkvertexlayout.hpp"
#include "e2/rhi/vk/vkdescriptorsetlayout.hpp"
#include "e2/rhi/vk/vkrendertarget.hpp"

#include "e2/log.hpp"


namespace
{
	static bool initialized = false;
	static uint32_t contextCount = 0;
	
	void glfwError(int32_t error, const char* description)
	{
		LogError("glfw {}: {}", error, description);
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL vulkanValidation(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
		VkDebugUtilsMessageTypeFlagsEXT messageType, 
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
		void* pUserData)
	{
		LogNotice("vulkan validation: {}", pCallbackData->pMessage);
		return VK_FALSE;
	}
}

e2::IRenderContext_Vk::IRenderContext_Vk(e2::Context* engineContext, e2::Name applicationName)
	: e2::IRenderContext(engineContext, applicationName)
{
	m_threadLocals.resize(e2::maxPersistentThreads);

	VkResult result{};
	if (!initialized)
	{
		result = volkInitialize();
		if (result != VK_SUCCESS)
		{
			LogError("volkInitialize: {}", int32_t(result));
		}

		initialized = true;
	}

	if (::contextCount == 0)
	{
		glfwSetErrorCallback(::glfwError);

		glfwInit();
	}

	createInstance(applicationName);
#if defined(E2_DEVELOPMENT)
	createValidation();
#endif
	createPhysicalDevice();
	createDevice();

	// null initialize the sampler cache
	m_samplerCache.resize(8);
	for (uint8_t i = 0; i < 8; i++)
	{
		m_samplerCache[i] = nullptr;
	}

	::contextCount++;
}

e2::IRenderContext_Vk::~IRenderContext_Vk()
{
	vkDeviceWaitIdle(m_vkDevice);

#if defined(E2_DEVELOPMENT)
	printLeaks();
#endif

	for (VkSampler sampler : m_samplerCache)
	{
		if (sampler)
		{
			vkDestroySampler(m_vkDevice, sampler, nullptr);
		}
	}

	for (VkThreadLocals& locals : m_threadLocals)
	{
		if (locals.initialized)
		{
			vkFreeCommandBuffers(m_vkDevice, locals.vkTransientPool, 1, &locals.vkTransientBuffer);
			vkDestroyFence(m_vkDevice, locals.vkTransientFence, nullptr);
			vkDestroyCommandPool(m_vkDevice, locals.vkTransientPool, nullptr);
		}
	}

	destroyDevice();
	destroyPhysicalDevice();
#if defined(E2_DEVELOPMENT)
	destroyValidation();
#endif
	destroyInstance();

	if (::contextCount == 1)
	{
		glfwTerminate();
	}

	::contextCount--;
}

e2::VkSwapchainCapabilities e2::IRenderContext_Vk::querySwapchainCapabilities(VkSurfaceKHR surface)
{
	VkResult result{};
	e2::VkSwapchainCapabilities returner{};

	// Get surface capabilities 
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vkPhysicalDevice, surface, &returner.surfaceCapabilities);

	// Get surface formats 
	uint32_t surfaceFormatCount{ e2::maxVkSurfaceFormats };
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, surface, &surfaceFormatCount, returner.surfaceFormats.data());
	if (result == VK_SUCCESS)
	{
		returner.surfaceFormats.resize(surfaceFormatCount);
	}
	else if (result == VK_INCOMPLETE)
	{
		returner.surfaceFormats.resize(e2::maxVkSurfaceFormats);
		LogWarning("vkGetPhysicalDeviceSurfaceFormatsKHR incomplete, increase e2::maxVkSurfaceFormats to at least {}", surfaceFormatCount + 1);
	}

	// Get present modes 
	uint32_t presentModeCount{ e2::maxVkPresentModes };
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_vkPhysicalDevice, surface, &presentModeCount, returner.presentModes.data());
	if (result == VK_SUCCESS)
	{
		returner.presentModes.resize(presentModeCount);
	}
	else if (result == VK_INCOMPLETE)
	{
		returner.presentModes.resize(e2::maxVkPresentModes);
		LogWarning("vkGetPhysicalDeviceSurfacePresentModesKHR incomplete, increase e2::maxVkPresentModes to at least {}", presentModeCount);
	}

	return returner;
}

void e2::IRenderContext_Vk::tick(double seconds)
{
	glfwPollEvents();

}

e2::RenderCapabilities const& e2::IRenderContext_Vk::getCapabilities()
{
	return m_capabilities;
}

e2::IFence* e2::IRenderContext_Vk::createFence(e2::FenceCreateInfo const& createInfo)
{
	return e2::create<e2::IFence_Vk>(this, createInfo);
}

e2::ISemaphore* e2::IRenderContext_Vk::createSemaphore(e2::SemaphoreCreateInfo const& createInfo)
{
	return e2::create<e2::ISemaphore_Vk>(this, createInfo);
}

e2::IDataBuffer* e2::IRenderContext_Vk::createDataBuffer(e2::DataBufferCreateInfo const& createInfo)
{
	return e2::create<e2::IDataBuffer_Vk>(this, createInfo);
}

e2::ITexture* e2::IRenderContext_Vk::createTexture(e2::TextureCreateInfo const& createInfo)
{
	return e2::create<e2::ITexture_Vk>(this, createInfo);
}

e2::ISampler* e2::IRenderContext_Vk::createSampler(e2::SamplerCreateInfo const& createInfo)
{
	return e2::create<e2::ISampler_Vk>(this, createInfo);
}

e2::IPipeline* e2::IRenderContext_Vk::createPipeline(e2::PipelineCreateInfo const& createInfo)
{
	return e2::create<e2::IPipeline_Vk>(this, createInfo);
}

e2::IShader* e2::IRenderContext_Vk::createShader(e2::ShaderCreateInfo const& createInfo)
{
	return e2::create<e2::IShader_Vk>(this, createInfo);
}

e2::IThreadContext* e2::IRenderContext_Vk::createThreadContext(ThreadContextCreateInfo const& createInfo)
{
	return e2::create<e2::IThreadContext_Vk>(this, createInfo);
}

e2::IDescriptorSetLayout* e2::IRenderContext_Vk::createDescriptorSetLayout(e2::DescriptorSetLayoutCreateInfo const& createInfo)
{
	return e2::create<e2::IDescriptorSetLayout_Vk>(this, createInfo);
}

e2::IPipelineLayout* e2::IRenderContext_Vk::createPipelineLayout(e2::PipelineLayoutCreateInfo const& createInfo)
{
	return e2::create<e2::IPipelineLayout_Vk>(this, createInfo);
}

e2::IVertexLayout* e2::IRenderContext_Vk::createVertexLayout(e2::VertexLayoutCreateInfo const& createInfo)
{
	return e2::create<e2::IVertexLayout_Vk>(this, createInfo);
}

e2::IRenderTarget* e2::IRenderContext_Vk::createRenderTarget(e2::RenderTargetCreateInfo const& createInfo)
{
	return e2::create<e2::IRenderTarget_Vk>(this, createInfo);
}

void e2::IRenderContext_Vk::submitBuffers(e2::RenderSubmitInfo* submitInfos, uint32_t numSubmitInfos, e2::IFence* fence)
{
	m_submitCache.resize(numSubmitInfos);
	for (uint32_t i = 0; i < numSubmitInfos; i++)
	{
		e2::ICommandBuffer_Vk* vkCmdBuffer = static_cast<e2::ICommandBuffer_Vk*>(submitInfos[i].buffer);
		e2::ISemaphore_Vk* vkWaitSemaphore = static_cast<e2::ISemaphore_Vk*>(submitInfos[i].waitSemaphore);
		e2::ISemaphore_Vk* vkSignalSemaphore = static_cast<e2::ISemaphore_Vk*>(submitInfos[i].signalSemaphore);

		// @todo initialize these two in constructor
		m_submitCache[i].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		m_submitCache[i].pNext = nullptr;
		
		// 
		m_submitCache[i].commandBufferCount = 1;
		m_submitCache[i].pCommandBuffers = &vkCmdBuffer->m_vkHandle;
		m_submitCache[i].waitSemaphoreCount = vkWaitSemaphore ? 1 : 0;
		m_submitCache[i].pWaitSemaphores = &vkWaitSemaphore->m_vkHandle;
		m_submitCache[i].signalSemaphoreCount = vkSignalSemaphore ? 1 : 0;
		m_submitCache[i].pSignalSemaphores = &vkSignalSemaphore->m_vkHandle;
	}

	e2::IFence_Vk* vkFence = static_cast<e2::IFence_Vk*>(fence);

	std::scoped_lock lock(m_queueMutex);
	VkResult result = vkQueueSubmit(m_vkQueue, (uint32_t)m_submitCache.size(), m_submitCache.data(), vkFence ? vkFence->m_vkHandle : nullptr);
	if (result != VK_SUCCESS)
	{
		LogError("vkQueueSubmit failed: {}", int32_t(result));
	}
}

void e2::IRenderContext_Vk::waitIdle()
{
	std::scoped_lock lock(m_queueMutex);
	vkQueueWaitIdle(m_vkQueue);
}

e2::VkThreadLocals & e2::IRenderContext_Vk::getThreadLocals()
{
	e2::ThreadInfo const &info = e2::threadInfo();
	
	if (info.id >= e2::maxPersistentThreads)
	{
		LogError("We got thread outside maxPersistentThreads, expect crash or major bugs.");
	}

	if (!m_threadLocals[info.id].initialized)
	{
		VkResult result{};

		VkCommandPoolCreateInfo createInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
		createInfo.queueFamilyIndex = m_queueFamily;
		result = vkCreateCommandPool(m_vkDevice, &createInfo, nullptr, &m_threadLocals[info.id].vkTransientPool);
		if (result != VK_SUCCESS)
		{
			LogError("vkCreateCommandPool failed: {}", int32_t(result));
		}

		VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		//fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		result = vkCreateFence(m_vkDevice, &fenceCreateInfo, nullptr, &m_threadLocals[info.id].vkTransientFence);
		if (result != VK_SUCCESS)
		{
			LogError("VkCreateFence failed: {}", int32_t(result));
		}

		VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.commandPool = m_threadLocals[info.id].vkTransientPool;
		allocateInfo.commandBufferCount = 1;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		result = vkAllocateCommandBuffers(m_vkDevice, &allocateInfo, &m_threadLocals[info.id].vkTransientBuffer);
		if (result != VK_SUCCESS)
		{
			LogError("vkAllocateCommandBuffers failed: {}", int32_t(result));
		}

		m_threadLocals[info.id].initialized = true;
	}

	return m_threadLocals[info.id];
}

VkSampler e2::IRenderContext_Vk::getOrCreateSampler(e2::SamplerFilter filter, e2::SamplerWrap wrap)
{
	// generate internal cache index
	uint16_t index = uint16_t(filter) | (uint16_t(wrap) << 2);

	// create sampler if it doesnt yet exist
	if (!m_samplerCache[index])
	{
		VkSamplerCreateInfo createInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		createInfo.anisotropyEnable = filter == e2::SamplerFilter::Anisotropic ? VK_TRUE : VK_FALSE;
		createInfo.maxAnisotropy = filter == e2::SamplerFilter::Anisotropic ? 16.0f : 0.0f;
		createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		createInfo.compareEnable = VK_FALSE;
		createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		createInfo.minLod = 0;
		createInfo.maxLod = VK_LOD_CLAMP_NONE;
		createInfo.minFilter = filter == SamplerFilter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
		createInfo.magFilter = filter == SamplerFilter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
		createInfo.mipmapMode = (filter == SamplerFilter::Nearest || filter == SamplerFilter::Bilinear) ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
		createInfo.mipLodBias = 0.0f;
		createInfo.unnormalizedCoordinates = VK_FALSE;

		if (wrap == SamplerWrap::Clamp)
		{
			createInfo.addressModeU = createInfo.addressModeV = createInfo.addressModeW =  VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		}
		else if (wrap == SamplerWrap::Repeat)
		{
			createInfo.addressModeU = createInfo.addressModeV = createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}
		else if (wrap == SamplerWrap::Equirect)
		{
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeV = createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		}

		VkResult result = vkCreateSampler(m_vkDevice, &createInfo, nullptr, &m_samplerCache[index]);
		if (result != VK_SUCCESS)
		{
			LogError("vkCreateSampler failed: {}", int32_t(result));
		}
	}

	return m_samplerCache[index];
}

void e2::IRenderContext_Vk::vkCmdTransitionImage(VkCommandBuffer buffer, VkImage image, uint32_t mips, uint32_t mipOffset, uint32_t layers, VkImageAspectFlags aspectFlags, VkImageLayout from, VkImageLayout to, VkPipelineStageFlags sourceStage /*= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT*/, VkPipelineStageFlags destinationStage /*= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT*/, VkAccessFlags sourceAccess /*= 0*/, VkAccessFlags destinationAccess /*= 0*/)
{
	VkImageMemoryBarrier memoryBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	memoryBarrier.oldLayout = from;
	memoryBarrier.newLayout = to;
	memoryBarrier.srcAccessMask = sourceAccess;
	memoryBarrier.dstAccessMask = destinationAccess;
	memoryBarrier.image = image;
	memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memoryBarrier.subresourceRange.aspectMask = aspectFlags;
	memoryBarrier.subresourceRange.baseArrayLayer = 0;
	memoryBarrier.subresourceRange.baseMipLevel = mipOffset;
	memoryBarrier.subresourceRange.layerCount = layers;
	memoryBarrier.subresourceRange.levelCount = mips;

	vkCmdPipelineBarrier(buffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &memoryBarrier);
}

void e2::IRenderContext_Vk::vkCmdTransitionTexture(VkCommandBuffer buffer, e2::ITexture_Vk* texture, VkImageLayout from, VkImageLayout to, VkPipelineStageFlags sourceStage /*= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT*/, VkPipelineStageFlags destinationStage /*= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT*/, VkAccessFlags sourceAccess /*= 0*/, VkAccessFlags destinationAccess /*= 0*/)
{
	VkImageMemoryBarrier memoryBarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	memoryBarrier.oldLayout = from;
	memoryBarrier.newLayout = to;
	memoryBarrier.srcAccessMask = sourceAccess;
	memoryBarrier.dstAccessMask = destinationAccess;
	memoryBarrier.image = texture->m_vkImage;
	memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memoryBarrier.subresourceRange.aspectMask = texture->m_vkAspectFlags;
	memoryBarrier.subresourceRange.baseArrayLayer = 0;
	memoryBarrier.subresourceRange.baseMipLevel = 0;
	memoryBarrier.subresourceRange.layerCount = texture->m_layerCount;
	memoryBarrier.subresourceRange.levelCount = texture->m_mipCount;

	vkCmdPipelineBarrier(buffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &memoryBarrier);
}


void e2::IRenderContext_Vk::transientPrepare(bool blocking, e2::VkThreadLocals &threadLocals)
{

	if (threadLocals.vkTransientShouldWait)
	{
		// wait for the previously issued buffer
		vkWaitForFences(m_vkDevice, 1, &threadLocals.vkTransientFence, VK_TRUE, UINT64_MAX);
		vkResetFences(m_vkDevice, 1, &threadLocals.vkTransientFence);
	}

	vkResetCommandPool(m_vkDevice, threadLocals.vkTransientPool, 0);

	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkResult result = vkBeginCommandBuffer(threadLocals.vkTransientBuffer, &beginInfo);
	if (result != VK_SUCCESS)
	{
		LogError("vkBeginCommandBuffer failed: {}", int32_t(result));
	}
}

void e2::IRenderContext_Vk::transientFinalize(bool blocking, e2::VkThreadLocals &threadLocals)
{
	VkResult result = vkEndCommandBuffer(threadLocals.vkTransientBuffer);
	if (result != VK_SUCCESS)
	{
		LogError("vkEndCommandBuffer failed: {}", int32_t(result));
	}

	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.pCommandBuffers = &threadLocals.vkTransientBuffer;
	submitInfo.commandBufferCount = 1;

	{
		std::scoped_lock lock(m_queueMutex);
		result = vkQueueSubmit(m_vkQueue, 1, &submitInfo, threadLocals.vkTransientFence);
	}

	if (result != VK_SUCCESS)
	{
		LogError("vkQueueSubmit failed: {}", int32_t(result));
	}

	if (blocking)
	{
		// wait for the issued buffer
		vkWaitForFences(m_vkDevice, 1, &threadLocals.vkTransientFence, VK_TRUE, UINT64_MAX);
		vkResetFences(m_vkDevice, 1, &threadLocals.vkTransientFence);
		threadLocals.vkTransientShouldWait = false;
	}
	else
	{
		threadLocals.vkTransientShouldWait = true;
	}

}

void e2::IRenderContext_Vk::createInstance(e2::Name appName)
{
	VkResult result{};

	uint32_t glfwRequiredExtensionsCount{};
	char const** glfwRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionsCount);


	char const** enabledExtensions = new char const* [glfwRequiredExtensionsCount + 1];
	enabledExtensions[0] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
	for (uint32_t i = 1; i < glfwRequiredExtensionsCount + 1; i++)
	{
		enabledExtensions[i] = glfwRequiredExtensions[i - 1];
	}


#if defined(E2_DEVELOPMENT)
	char const* enabledLayers[] = { "VK_LAYER_KHRONOS_validation" };
#endif

	VkApplicationInfo vkAppInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	vkAppInfo.apiVersion = VK_MAKE_VERSION(1, 3, 0);
	vkAppInfo.pApplicationName = appName.cstring();
	vkAppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	vkAppInfo.pEngineName = "e2";
	vkAppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

	VkInstanceCreateInfo instanceCreateInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	instanceCreateInfo.pApplicationInfo = &vkAppInfo;
	instanceCreateInfo.enabledExtensionCount = glfwRequiredExtensionsCount + 1;
	instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions;
#if defined(E2_DEVELOPMENT)
	instanceCreateInfo.enabledLayerCount = 1;
	instanceCreateInfo.ppEnabledLayerNames = enabledLayers;
#else 
	instanceCreateInfo.enabledLayerCount = 0;
	instanceCreateInfo.ppEnabledLayerNames = nullptr;
#endif



	result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_vkInstance);
	if (result != VK_SUCCESS)
	{
		LogError("vkCreateInstance failed: {}", int32_t(result));
	}

	// Load the rest of Vulkan API
	volkLoadInstance(m_vkInstance);
}

void e2::IRenderContext_Vk::destroyInstance()
{
	vkDestroyInstance(m_vkInstance, nullptr);
}

void e2::IRenderContext_Vk::createValidation()
{
	if (!vkCreateDebugUtilsMessengerEXT)
		return;

	VkResult result{};

	VkDebugUtilsMessengerCreateInfoEXT validationCreateInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	validationCreateInfo.pfnUserCallback = ::vulkanValidation;
	validationCreateInfo.pUserData = this;
	validationCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	validationCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;

	result = vkCreateDebugUtilsMessengerEXT(m_vkInstance, &validationCreateInfo, nullptr, &m_vkValidation);
	if (result != VK_SUCCESS)
	{
		LogError("vkCreateDebugUtilsMessengerEXT failed: {}", int32_t(result));
	}
}

void e2::IRenderContext_Vk::destroyValidation()
{
	if (!vkCreateDebugUtilsMessengerEXT)
		return;

	vkDestroyDebugUtilsMessengerEXT(m_vkInstance, m_vkValidation, nullptr);
}

void e2::IRenderContext_Vk::createPhysicalDevice()
{
	// Select physical device
	std::vector<VkPhysicalDevice> devices;
	uint32_t numDevices{};

	vkEnumeratePhysicalDevices(m_vkInstance, &numDevices, nullptr);
	if (numDevices == 0)
	{
		LogError("No graphics card detected");
		return;

	}

	devices.resize(numDevices, {});

	vkEnumeratePhysicalDevices(m_vkInstance, &numDevices, devices.data());

	VkPhysicalDevice& fallbackDevice = devices[0];

	for (VkPhysicalDevice& physicalDevice : devices)
	{
		VkPhysicalDeviceProperties deviceProps;
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProps);

		if (deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			LogNotice("selected gpu: {}", deviceProps.deviceName);
			m_vkPhysicalDevice = physicalDevice;
			break;
		}
	}

	if (!m_vkPhysicalDevice)
	{
		m_vkPhysicalDevice = fallbackDevice;
		LogError("no discrete gpu found, using crap card");
	}

	// Find best queue family (presentable + graphics + most queues)

	std::vector<VkQueueFamilyProperties> familyProps;
	uint32_t numFamilyProps{};

	vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &numFamilyProps, nullptr);
	familyProps.resize(numFamilyProps);

	vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &numFamilyProps, familyProps.data());

	uint32_t mostQueues = 0;
	uint32_t bestQueueYet = 0;
	bool foundDecent = false;

	uint32_t currQueueFamily = 0;
	for (VkQueueFamilyProperties& currProps : familyProps)
	{
		bool presentable = glfwGetPhysicalDevicePresentationSupport(m_vkInstance, m_vkPhysicalDevice, currQueueFamily);
		bool graphicsable = (currProps.queueFlags & VK_QUEUE_GRAPHICS_BIT);

		if (presentable && graphicsable)
		{
			foundDecent = true;

			if (currProps.queueCount > mostQueues)
			{
				bestQueueYet = currQueueFamily;
				mostQueues = currProps.queueCount;
			}
		}

		currQueueFamily++;
	}

	m_queueFamily = bestQueueYet;

	if (!foundDecent)
	{
		LogError("no decent queue family found");
	}
	
	VkPhysicalDeviceProperties deviceProps{};
	vkGetPhysicalDeviceProperties(m_vkPhysicalDevice, &deviceProps);
	m_capabilities.minUniformBufferOffsetAlignment = (uint32_t)deviceProps.limits.minUniformBufferOffsetAlignment;

	LogNotice("best compatible queue family is {} and has {} queues", m_queueFamily, mostQueues);
	
}

void e2::IRenderContext_Vk::destroyPhysicalDevice()
{

}

void e2::IRenderContext_Vk::createDevice()
{
	VkResult result{};

	char const* deviceExtensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_MULTIVIEW_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
		VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME
	};

	VkPhysicalDeviceFeatures physicalFeatures{};
	vkGetPhysicalDeviceFeatures(m_vkPhysicalDevice, &physicalFeatures);

	static float queuePriority = 1.0f;
	VkDeviceQueueCreateInfo queueCreateInfo { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;
	queueCreateInfo.queueFamilyIndex = m_queueFamily;

	VkDeviceCreateInfo deviceCreateInfo { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.enabledExtensionCount = 4;
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
	deviceCreateInfo.pEnabledFeatures = &physicalFeatures;

	VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRendering{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR };
	dynamicRendering.dynamicRendering = VK_TRUE;
	deviceCreateInfo.pNext = &dynamicRendering;

	VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT vertexInput{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT };
	vertexInput.vertexInputDynamicState = VK_TRUE;
	dynamicRendering.pNext = &vertexInput;

	VkPhysicalDeviceVulkan12Features vk12Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	vk12Features.runtimeDescriptorArray = VK_TRUE;
	vk12Features.descriptorIndexing = VK_TRUE;
	vk12Features.descriptorBindingPartiallyBound = VK_TRUE;
	vk12Features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
	vertexInput.pNext = &vk12Features;

	result = vkCreateDevice(m_vkPhysicalDevice, &deviceCreateInfo, nullptr, &m_vkDevice);
	if (result != VK_SUCCESS)
	{
		LogError("vkCreateDevice failed: {}", int32_t(result));
	}

	// retrieve the queue we created 
	vkGetDeviceQueue(m_vkDevice, m_queueFamily, 0, &m_vkQueue);

	// create vma allocator 
	VmaVulkanFunctions vmaFunctions{};
	vmaFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vmaFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo allocatorCreateInfo{};
	allocatorCreateInfo.physicalDevice = m_vkPhysicalDevice;
	allocatorCreateInfo.device = m_vkDevice;
	allocatorCreateInfo.instance = m_vkInstance;
	allocatorCreateInfo.pVulkanFunctions = &vmaFunctions;

	result = vmaCreateAllocator(&allocatorCreateInfo, &m_vmaAllocator);
	if (result != VK_SUCCESS)
	{
		LogError("vmaCreateAllocator failed: {}", int32_t(result));
	}
}

void e2::IRenderContext_Vk::destroyDevice()
{
	vmaDestroyAllocator(m_vmaAllocator);
	vkDestroyDevice(m_vkDevice, nullptr);
}
