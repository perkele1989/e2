
#include "e2/rhi/vk/vkwindow.hpp"
#include "e2/rhi/vk/vkrendercontext.hpp"
#include "e2/rhi/vk/vkthreadcontext.hpp"
#include "e2/rhi/vk/vktexture.hpp"

#include <Volk/volk.h>

#include <GLFW/glfw3.h>


#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

e2::IWindow_Vk::IWindow_Vk(IThreadContext* context, e2::WindowCreateInfo const& createInfo)
	: e2::IWindow(context, createInfo)
	, e2::ThreadHolder_Vk(context)
{
	VkResult result{};

	m_size = createInfo.size;



	createGlfwWindow(createInfo);

	createSurface();
	createSwapchain();

	createCommandBuffers();



	VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (uint8_t i = 0; i < 2; i++)
	{
		result = vkCreateFence(m_renderContextVk->m_vkDevice, &fenceCreateInfo, nullptr, &m_vkFences[i]);
		if (result != VK_SUCCESS)
		{
			LogError("vkCreateFence failed: {}", int32_t(result));
		}
	}

	VkSemaphoreCreateInfo semaCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	for (uint8_t i = 0; i < 2; i++)
	{
		result = vkCreateSemaphore(m_renderContextVk->m_vkDevice, &semaCreateInfo, nullptr, &m_vkSemaphoresAvailable[i]);
		if (result != VK_SUCCESS)
		{
			LogError("vkCreateSemaphore failed: {}", int32_t(result));
		}

		result = vkCreateSemaphore(m_renderContextVk->m_vkDevice, &semaCreateInfo, nullptr, &m_vkSemaphoresEndFrame[i]);
		if (result != VK_SUCCESS)
		{
			LogError("vkCreateSemaphore failed: {}", int32_t(result));
		}
	}

}

e2::IWindow_Vk::~IWindow_Vk()
{
	destroyCommandBuffers();
	// destroy the old handle completely
	if (m_vkSwapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(m_renderContextVk->m_vkDevice, m_vkSwapchain, nullptr);
		m_vkSwapchain = nullptr;
	}
	destroySwapchain();
	destroySurface();
	destroyGlfwWindow();

	vkDestroySemaphore(m_renderContextVk->m_vkDevice, m_vkSemaphoresEndFrame[0], nullptr);
	vkDestroySemaphore(m_renderContextVk->m_vkDevice, m_vkSemaphoresEndFrame[1], nullptr);

	vkDestroySemaphore(m_renderContextVk->m_vkDevice, m_vkSemaphoresAvailable[0], nullptr);
	vkDestroySemaphore(m_renderContextVk->m_vkDevice, m_vkSemaphoresAvailable[1], nullptr);

	vkDestroyFence(m_renderContextVk->m_vkDevice, m_vkFences[0], nullptr);
	vkDestroyFence(m_renderContextVk->m_vkDevice, m_vkFences[1], nullptr);

}

void e2::IWindow_Vk::title(std::string const& newTitle)
{
	glfwSetWindowTitle(m_glfwHandle, newTitle.c_str());
}

bool e2::IWindow_Vk::wantsClose() const
{
	return glfwWindowShouldClose(m_glfwHandle);
}

void e2::IWindow_Vk::setFullscreen(bool newFullscreen)
{
	if (newFullscreen)
	{
		GLFWvidmode const* vidMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		glfwSetWindowMonitor(m_glfwHandle,  glfwGetPrimaryMonitor() , 0, 0, vidMode->width, vidMode->height, vidMode->refreshRate);
		
	}
	else
	{

		glfwSetWindowMonitor(m_glfwHandle, nullptr, 64, 64, 1280, 720, GLFW_DONT_CARE);
	}
}

bool e2::IWindow_Vk::isFullscreen()
{
	return glfwGetWindowMonitor(m_glfwHandle) != nullptr;
}

void e2::IWindow_Vk::wantsClose(bool newValue)
{
	glfwSetWindowShouldClose(m_glfwHandle, newValue);
}

void e2::IWindow_Vk::present() 
{


	if (!m_source)
		return;

	if (m_size.x == 0 || m_size.y == 0)
		return;

	if (m_skipPresent> 0)
	{
		m_skipPresent--;
		return;
	}

	vkWaitForFences(m_renderContextVk->m_vkDevice, 1, &m_vkFences[m_currentFrame], VK_TRUE, UINT64_MAX);
	vkResetFences(m_renderContextVk->m_vkDevice, 1, &m_vkFences[m_currentFrame]);


	if (m_swapchainDirty)
	{
		recreateTemp();
		m_swapchainDirty = false;
	}

	uint32_t imageIndex{};
	VkResult result = vkAcquireNextImageKHR(m_renderContextVk->m_vkDevice, m_vkSwapchain, UINT64_MAX, m_vkSemaphoresAvailable[m_currentFrame], nullptr, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		LogNotice("Out of date, dirtying.");
		//vkDeviceWaitIdle(m_renderContextVk->m_vkDevice);
		//recreateTemp();
		m_swapchainDirty = true;
		return;
	}
	else if (result != VK_SUCCESS)
	{
		LogError("vkAcquireNextImageKHR failed, dirtying: {}", int32_t(result));
		m_swapchainDirty = true;
	}

	recordCommandBuffer();

	// Submit blit buffers
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_vkSemaphoresAvailable[m_currentFrame];
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_vkCommandBuffers[m_currentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_vkSemaphoresEndFrame[m_currentFrame];
	
	{
		std::scoped_lock lock(m_renderContextVk->m_queueMutex);
		result = vkQueueSubmit(m_renderContextVk->m_vkQueue, 1, &submitInfo, m_vkFences[m_currentFrame]);
		if (result != VK_SUCCESS)
		{
			LogError("vkQueueSubmit failed: {}", int32_t(result));
		}

		// Submit present
		VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &m_vkSemaphoresEndFrame[m_currentFrame];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_vkSwapchain;
		presentInfo.pImageIndices = &imageIndex;
		result = vkQueuePresentKHR(m_renderContextVk->m_vkQueue, &presentInfo);
		if (result != VK_SUCCESS)
		{
			LogError("vkQueuePresentKHR failed, dirtying: {}", int32_t(result));
			m_swapchainDirty = true;
		}
	}
	// Flip buffers (this will never not be nostalgic)
	m_currentFrame = m_currentFrame == 0 ? 1 : 0;

	// resize here if wanted 
	if (m_wantsResize)
	{
		glfwSetWindowSize(m_glfwHandle, (int32_t)m_newSize.x, (int32_t)m_newSize.y);
		m_wantsResize = false;
	}

}

glm::uvec2 e2::IWindow_Vk::size()
{
	return m_size;
}

namespace
{
	e2::StackVector<GLFWcursor*, size_t(e2::CursorShape::Count)> glfwCursors;
	e2::StackVector<uint32_t, size_t(e2::CursorShape::Count)> glfwMap;
}

void e2::IWindow_Vk::cursor(e2::CursorShape newCursor)
{
	

	if (::glfwCursors.size() == 0)
	{
		::glfwCursors.resize(size_t(e2::CursorShape::Count));
		::glfwMap.resize(size_t(e2::CursorShape::Count));
		::glfwMap[size_t(e2::CursorShape::Default)] = GLFW_ARROW_CURSOR;
		::glfwMap[size_t(e2::CursorShape::Text)] = GLFW_IBEAM_CURSOR;
		::glfwMap[size_t(e2::CursorShape::Crosshair)] = GLFW_CROSSHAIR_CURSOR;
		::glfwMap[size_t(e2::CursorShape::Link)] = GLFW_POINTING_HAND_CURSOR;
		::glfwMap[size_t(e2::CursorShape::ResizeNESW)] = GLFW_RESIZE_NESW_CURSOR;
		::glfwMap[size_t(e2::CursorShape::ResizeNS)] = GLFW_RESIZE_NS_CURSOR;
		::glfwMap[size_t(e2::CursorShape::ResizeNWSE)] = GLFW_RESIZE_NWSE_CURSOR;
		::glfwMap[size_t(e2::CursorShape::ResizeWE)] = GLFW_RESIZE_EW_CURSOR;
		
		for (uint8_t i = uint8_t(e2::CursorShape::Default); i < uint8_t(e2::CursorShape::Count); i++)
		{
			::glfwCursors[i] = glfwCreateStandardCursor(::glfwMap[i]);
		}
		
	}

	glfwSetCursor(m_glfwHandle, ::glfwCursors[size_t(newCursor)]);
}

void e2::IWindow_Vk::size(glm::uvec2 const& newSize)
{
	if (newSize.x == 0 || newSize.y == 0)
		return;

	//LogNotice("VkWindow requesting resize {}", newSize);
	m_wantsResize = true;
	m_newSize = newSize;

	//glfwSetWindowSize(m_glfwHandle, newSize.x, newSize.y);
	//m_skipPresent = 1;
}

void e2::IWindow_Vk::onResized(glm::uvec2 newSize)
{
	//if (newSize.x != 0 && newSize.y != 0)
	{
		m_size = newSize;
		recreateTemp();
	}
}

glm::vec2 e2::IWindow_Vk::position()
{
	int32_t x, y;
	glfwGetWindowPos(m_glfwHandle, &x, &y);
	return glm::vec2(x, y);
}

void e2::IWindow_Vk::maximize()
{
	glfwMaximizeWindow(m_glfwHandle);
}

void e2::IWindow_Vk::restore()
{
	glfwRestoreWindow(m_glfwHandle);
}

void e2::IWindow_Vk::minimize()
{
	glfwIconifyWindow(m_glfwHandle);
}

bool e2::IWindow_Vk::isMaximized()
{
	return glfwGetWindowAttrib(m_glfwHandle, GLFW_MAXIMIZED);
}

bool e2::IWindow_Vk::isFocused()
{
	return glfwGetWindowAttrib(m_glfwHandle, GLFW_FOCUSED);
}

void e2::IWindow_Vk::mouseLock(bool newMouseLock)
{
	if (newMouseLock)
	{
		glfwSetInputMode(m_glfwHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		if (glfwRawMouseMotionSupported() == GLFW_TRUE)
		{
			glfwSetInputMode(m_glfwHandle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		}
	}
	else
	{
		glfwSetInputMode(m_glfwHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		if (glfwRawMouseMotionSupported() == GLFW_TRUE)
		{
			glfwSetInputMode(m_glfwHandle, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
		}
	}
}

void e2::IWindow_Vk::position(glm::vec2 const& newPosition)
{
	glfwSetWindowPos(m_glfwHandle, int32_t(newPosition.x), int32_t(newPosition.y));
}

void e2::IWindow_Vk::registerInputHandler(e2::IWindowInputHandler* handler)
{
	m_inputHandlers.insert(handler);
}

void e2::IWindow_Vk::unregisterInputHandler(e2::IWindowInputHandler* handler)
{
	m_inputHandlers.erase(handler);
}

namespace
{
	void glfwWindowResized(GLFWwindow* window, int width, int height)
	{
		e2::IWindow_Vk* vkWin = static_cast<e2::IWindow_Vk*>(glfwGetWindowUserPointer(window));
		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		vkWin->onResized(glm::uvec2(w, h));
	}

	void glfwMouseMove(GLFWwindow* window, double xpos, double ypos)
	{
		e2::IWindow_Vk* vkWin = static_cast<e2::IWindow_Vk*>(glfwGetWindowUserPointer(window));
		if (vkWin->m_inputHandlers.size() == 0)
			return;

		int winX, winY;
		glfwGetWindowPos(window, &winX, &winY);

		for (e2::IWindowInputHandler* handler : vkWin->m_inputHandlers)
		{
			handler->onMouseMove(glm::vec2( double(winX) + xpos, double(winY)+ ypos ));
		}
	}

	void glfwMouseButton(GLFWwindow* window, int button, int action, int mods)
	{
		e2::IWindow_Vk* vkWin = static_cast<e2::IWindow_Vk*>(glfwGetWindowUserPointer(window));
		if (vkWin->m_inputHandlers.size() == 0)
			return;

		e2::MouseButton btn;
		switch (button)
		{
		default:
		case GLFW_MOUSE_BUTTON_LEFT:
			btn = e2::MouseButton::Left;
			break;
		case GLFW_MOUSE_BUTTON_MIDDLE:
			btn = e2::MouseButton::Middle;
			break;
		case GLFW_MOUSE_BUTTON_RIGHT:
			btn = e2::MouseButton::Right;
			break;
		case GLFW_MOUSE_BUTTON_4:
			btn = e2::MouseButton::Extra1;
			break;
		case GLFW_MOUSE_BUTTON_5:
			btn = e2::MouseButton::Extra2;
			break;
		}

		for (e2::IWindowInputHandler* handler : vkWin->m_inputHandlers)
		{
			if (action == GLFW_PRESS)
				handler->onMouseDown(btn);
			else if (action == GLFW_RELEASE)
				handler->onMouseUp(btn);
		}
	}

	void glfwKey(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		e2::IWindow_Vk* vkWin = static_cast<e2::IWindow_Vk*>(glfwGetWindowUserPointer(window));
		if (vkWin->m_inputHandlers.size() == 0)
			return;

		e2::Key k = (e2::Key)key;

		for (e2::IWindowInputHandler* handler : vkWin->m_inputHandlers)
		{
			if (action == GLFW_PRESS)
				handler->onKeyDown(k);
			else if (action == GLFW_RELEASE)
				handler->onKeyUp(k);
		}

	}

	void glfwScroll(GLFWwindow* window, double xoffset, double yoffset)
	{
		e2::IWindow_Vk* vkWin = static_cast<e2::IWindow_Vk*>(glfwGetWindowUserPointer(window));
		if (vkWin->m_inputHandlers.size() == 0)
			return;
		for (e2::IWindowInputHandler* handler : vkWin->m_inputHandlers)
		{
			handler->onMouseScroll((float)yoffset);
		}

	}

	void glfwDrop(GLFWwindow* window, int32_t pathCount, char const* paths[])
	{
		e2::IWindow_Vk* vkWin = static_cast<e2::IWindow_Vk*>(glfwGetWindowUserPointer(window));
		if (vkWin->m_inputHandlers.size() == 0)
			return;
		for (e2::IWindowInputHandler* handler : vkWin->m_inputHandlers)
		{
			handler->onDrop(pathCount, paths);
		}
	}
}

void e2::IWindow_Vk::createGlfwWindow(e2::WindowCreateInfo const& createInfo)
{
	GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
	GLFWvidmode const* primaryMode = glfwGetVideoMode(primaryMonitor);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, createInfo.resizable ? GLFW_TRUE : GLFW_FALSE);
	glfwWindowHint(GLFW_DECORATED, createInfo.customDecorations ? GLFW_FALSE : createInfo.mode == WindowMode::Windowed ? GLFW_TRUE : GLFW_FALSE);
	glfwWindowHint(GLFW_MAXIMIZED, createInfo.mode == WindowMode::Borderless ? GLFW_TRUE : GLFW_FALSE);
	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, createInfo.customDecorations ? GLFW_TRUE : GLFW_FALSE);
	glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_TRUE);
	glfwWindowHint(GLFW_FLOATING, createInfo.topMost ? GLFW_TRUE : GLFW_FALSE);

	if (createInfo.tool)
	{
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	}

	GLFWmonitor* monitor = nullptr;
	glm::uvec2 resolution = createInfo.size;


	if (createInfo.mode == WindowMode::Fullscreen)
	{
		int32_t numMonitors = 0;
		GLFWmonitor** monitors = glfwGetMonitors(&numMonitors);

		if (numMonitors < 0)
		{
			LogWarning("glfw broken, reported {} monitors, attempting to fallback to primary monitor", numMonitors);
			monitor = primaryMonitor;
		}
		else if (createInfo.monitorIndex >= uint32_t(numMonitors))
		{
			LogWarning("monitor index out of range ({}/{}), defaulting to primary monitor", createInfo.monitorIndex, numMonitors);
			monitor = primaryMonitor;
		}
		else
		{
			monitor = monitors[createInfo.monitorIndex];
		}
	}
	else if (createInfo.mode == WindowMode::Borderless)
	{
		resolution = glm::uvec2(primaryMode->width, primaryMode->height);
	}

	m_glfwHandle = glfwCreateWindow(resolution.x, resolution.y, createInfo.title.c_str(), monitor, nullptr);
	if (createInfo.tool)
	{
		HWND hwnd = glfwGetWin32Window(m_glfwHandle);
		SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_TOOLWINDOW);
	}

	glfwShowWindow(m_glfwHandle);

	glfwSetWindowUserPointer(m_glfwHandle, this);
	glfwSetWindowSizeCallback(m_glfwHandle, &::glfwWindowResized);
	glfwSetMouseButtonCallback(m_glfwHandle, &::glfwMouseButton);
	glfwSetCursorPosCallback(m_glfwHandle, &::glfwMouseMove);
	glfwSetScrollCallback(m_glfwHandle, &::glfwScroll);
	glfwSetDropCallback(m_glfwHandle, &::glfwDrop);
	glfwSetKeyCallback(m_glfwHandle, &::glfwKey);


}


void e2::IWindow_Vk::destroyGlfwWindow()
{
	glfwDestroyWindow(m_glfwHandle);
}


void e2::IWindow_Vk::createSurface()
{
	// create surface
	VkResult vkError = glfwCreateWindowSurface(m_renderContextVk->m_vkInstance, m_glfwHandle, nullptr, &m_vkSurface);
	if (vkError)
	{
		LogError("vulkan surface creation failed {}", int32_t(vkError));
	}
}

void e2::IWindow_Vk::destroySurface()
{
	vkDestroySurfaceKHR(m_renderContextVk->m_vkInstance, m_vkSurface, nullptr);
}

void e2::IWindow_Vk::createSwapchain()
{
	if (m_size.x == 0 || m_size.y == 0)
		return;

	// get swapchain capabilities for this surface
	VkSwapchainCapabilities swapchainCapabilities = m_renderContextVk->querySwapchainCapabilities(m_vkSurface);

	// select best format 
	VkSurfaceFormatKHR selectedFormat{};
	if (swapchainCapabilities.surfaceFormats.size() == 1 && swapchainCapabilities.surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		selectedFormat = swapchainCapabilities.surfaceFormats[0];
		//LogNotice("surface format: undefined");
	}
	else
	{
		bool found = false;
		for (VkSurfaceFormatKHR& currFormat : swapchainCapabilities.surfaceFormats)
		{
			if (currFormat.format == VK_FORMAT_B8G8R8A8_SRGB && currFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				selectedFormat = currFormat;
				found = true;
				//LogNotice("surface format: G8G8R8A8_SRGB VK_COLOR_SPACE_SRGB_NONLINEAR_KHR (preferred)");
				break;
			}
		}

		// Fallback to first available 
		if (!found)
		{
			selectedFormat = swapchainCapabilities.surfaceFormats[0];
			//LogNotice("surface format: defined but unpreferred");
		}
	}

	VkPresentModeKHR selectedPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	bool foundPresentMode = false;
	for (VkPresentModeKHR& presentMode : swapchainCapabilities.presentModes)
	{
		if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			selectedPresentMode = presentMode;
			foundPresentMode = true;
			//LogNotice("present mode: immediate");
			break;
		}

		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			selectedPresentMode = presentMode;
			foundPresentMode = true;
			//LogNotice("present mode: mailbox");
		}
	}

	if (!foundPresentMode)
	{
		//LogNotice("present mode: FIFO");
	}

	VkSwapchainCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	createInfo.surface = m_vkSurface;
	
	createInfo.imageFormat = selectedFormat.format;
	createInfo.imageColorSpace = selectedFormat.colorSpace;

	createInfo.presentMode = selectedPresentMode;

	createInfo.minImageCount = 2;
	createInfo.imageExtent.width = m_size.x;
	createInfo.imageExtent.height = m_size.y;
	createInfo.imageArrayLayers = 1;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	createInfo.oldSwapchain = m_vkSwapchain;
	//createInfo.oldSwapchain = nullptr;
	createInfo.clipped = VK_TRUE;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.preTransform = swapchainCapabilities.surfaceCapabilities.currentTransform;
	
	// create it to a separate variable since we pass the old handle to createinfo to be disposed
	VkSwapchainKHR newSwapchain{ VK_NULL_HANDLE };
	VkResult result = vkCreateSwapchainKHR(m_renderContextVk->m_vkDevice, &createInfo, nullptr, &newSwapchain);
	if (result != VK_SUCCESS)
	{
		LogError("vkCreateSwapchainKHR failed: {}", int32_t(result));
	}
	
	// destroy the old handle completely
	if (m_vkSwapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(m_renderContextVk->m_vkDevice, m_vkSwapchain, nullptr);
		m_vkSwapchain = nullptr;
	}

	// assign the new swapchain
	m_vkSwapchain = newSwapchain;


	// get the new swapchain images 
	uint32_t numImages = 2;
	result = vkGetSwapchainImagesKHR(m_renderContextVk->m_vkDevice, m_vkSwapchain, &numImages, m_vkSwapchainImages.data());
	if (result != VK_SUCCESS)
	{
		if (result == VK_INCOMPLETE)
			LogError("vkGetSwapchainImagesKHR returned more than two images, fix this!");
		else 
			LogError("vkGetSwapchainImagesKHR failed: {}", int32_t(result));
	}

	// create image views for the new swapchain images 
	if(m_vkSwapchainImageViews[0] != VK_NULL_HANDLE)
		vkDestroyImageView(m_renderContextVk->m_vkDevice, m_vkSwapchainImageViews[0], nullptr);

	if (m_vkSwapchainImageViews[1] != VK_NULL_HANDLE)
		vkDestroyImageView(m_renderContextVk->m_vkDevice, m_vkSwapchainImageViews[1], nullptr);

	VkImageViewCreateInfo viewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.format = selectedFormat.format;
	viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	viewCreateInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
	
	viewCreateInfo.image = m_vkSwapchainImages[0];
	result = vkCreateImageView(m_renderContextVk->m_vkDevice, &viewCreateInfo, nullptr, &m_vkSwapchainImageViews[0]);
	if (result != VK_SUCCESS)
	{
		LogError("vkCreateImageView failed: {}", int32_t(result));
	}

	viewCreateInfo.image = m_vkSwapchainImages[1];
	result = vkCreateImageView(m_renderContextVk->m_vkDevice, &viewCreateInfo, nullptr, &m_vkSwapchainImageViews[1]);
	if (result != VK_SUCCESS)
	{
		LogError("vkCreateImageView failed: {}", int32_t(result));
	}

	m_renderContextVk->submitTransient(true, [this](VkCommandBuffer buffer) {
		m_renderContextVk->vkCmdTransitionImage(buffer, m_vkSwapchainImages[0], 1, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		m_renderContextVk->vkCmdTransitionImage(buffer, m_vkSwapchainImages[1], 1, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	});	


}

void e2::IWindow_Vk::destroySwapchain()
{

	if(m_vkSwapchainImageViews[0])
		vkDestroyImageView(m_renderContextVk->m_vkDevice, m_vkSwapchainImageViews[0], nullptr);

	if (m_vkSwapchainImageViews[1])
		vkDestroyImageView(m_renderContextVk->m_vkDevice, m_vkSwapchainImageViews[1], nullptr);

	m_vkSwapchainImageViews[0] = nullptr;
	m_vkSwapchainImageViews[1] = nullptr;

	//vkDestroySwapchainKHR(m_renderContextVk->m_vkDevice, m_vkSwapchain, nullptr);
	//m_vkSwapchain = nullptr;
}

void e2::IWindow_Vk::createCommandBuffers()
{
	VkCommandBufferAllocateInfo vkAllocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	vkAllocateInfo.commandBufferCount = 2;
	vkAllocateInfo.commandPool = m_threadContextVk->m_persistentPool->m_vkHandle;
	vkAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	VkResult result = vkAllocateCommandBuffers(m_renderContextVk->m_vkDevice, &vkAllocateInfo, m_vkCommandBuffers.data());
	if (result != VK_SUCCESS)
	{
		LogError("vkAllocateCommandBuffers failed: {}", int32_t(result));
	}
}

void e2::IWindow_Vk::destroyCommandBuffers()
{
	vkFreeCommandBuffers(m_renderContextVk->m_vkDevice, m_threadContextVk->m_persistentPool->m_vkHandle, 2, m_vkCommandBuffers.data());
	m_vkCommandBuffers[0] = { nullptr };
	m_vkCommandBuffers[1] = { nullptr };
}

void e2::IWindow_Vk::recordCommandBuffer()
{

	ITexture_Vk* vkSource = static_cast<ITexture_Vk*>(m_source);

	VkImageBlit imageBlit = {};
	imageBlit.srcOffsets[0] = { 0,0,0 };
	imageBlit.srcOffsets[1] = { (int32_t)vkSource->m_size.x, (int32_t)vkSource->m_size.y, 1 };
	imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageBlit.srcSubresource.baseArrayLayer = 0;
	imageBlit.srcSubresource.layerCount = 1;
	imageBlit.srcSubresource.mipLevel = 0;
	imageBlit.dstOffsets[0] = { 0,0,0 };
	imageBlit.dstOffsets[1] = { (int32_t)m_size.x, (int32_t)m_size.y, 1 };
	imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageBlit.dstSubresource.baseArrayLayer = 0;
	imageBlit.dstSubresource.layerCount = 1;
	imageBlit.dstSubresource.mipLevel = 0;

	VkImageMemoryBarrier barrierSourceA{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrierSourceA.image = vkSource->m_vkImage;
	barrierSourceA.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrierSourceA.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrierSourceA.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	barrierSourceA.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrierSourceA.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrierSourceA.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrierSourceA.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrierSourceA.subresourceRange.baseMipLevel = 0;
	barrierSourceA.subresourceRange.levelCount = 1;
	barrierSourceA.subresourceRange.baseArrayLayer = 0;
	barrierSourceA.subresourceRange.layerCount = 1;

	VkImageMemoryBarrier barrierSourceB{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrierSourceB.image = vkSource->m_vkImage;
	barrierSourceB.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrierSourceB.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrierSourceB.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrierSourceB.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	barrierSourceB.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrierSourceB.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrierSourceB.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrierSourceB.subresourceRange.baseMipLevel = 0;
	barrierSourceB.subresourceRange.levelCount = 1;
	barrierSourceB.subresourceRange.baseArrayLayer = 0;
	barrierSourceB.subresourceRange.layerCount = 1;

	VkImageMemoryBarrier barrierSwapchainA{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrierSwapchainA.image = m_vkSwapchainImages[m_currentFrame];
	barrierSwapchainA.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrierSwapchainA.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrierSwapchainA.srcAccessMask = 0;
	barrierSwapchainA.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrierSwapchainA.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrierSwapchainA.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrierSwapchainA.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrierSwapchainA.subresourceRange.baseMipLevel = 0;
	barrierSwapchainA.subresourceRange.levelCount = 1;
	barrierSwapchainA.subresourceRange.baseArrayLayer = 0;
	barrierSwapchainA.subresourceRange.layerCount = 1;

	VkImageMemoryBarrier barrierSwapchainB{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrierSwapchainB.image = m_vkSwapchainImages[m_currentFrame];
	barrierSwapchainB.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrierSwapchainB.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrierSwapchainB.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrierSwapchainB.dstAccessMask = 0;
	barrierSwapchainB.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrierSwapchainB.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrierSwapchainB.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrierSwapchainB.subresourceRange.baseMipLevel = 0;
	barrierSwapchainB.subresourceRange.levelCount = 1;
	barrierSwapchainB.subresourceRange.baseArrayLayer = 0;
	barrierSwapchainB.subresourceRange.layerCount = 1;


	VkCommandBuffer commandBuffer = m_vkCommandBuffers[m_currentFrame];
	vkResetCommandBuffer(commandBuffer, 0);

	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierSourceA);
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierSwapchainA);
	vkCmdBlitImage(commandBuffer, vkSource->m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_vkSwapchainImages[m_currentFrame], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierSwapchainB);
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierSourceB);
	vkEndCommandBuffer(commandBuffer);
}

void e2::IWindow_Vk::createTemp()
{
	createSwapchain();
}

void e2::IWindow_Vk::destroyTemp()
{
	destroySwapchain();
}

void e2::IWindow_Vk::recreateTemp()
{
	//m_renderContextVk->waitIdle();
	destroyTemp();



	createTemp();
}

void e2::IWindow_Vk::source(ITexture *newSource)
{
	m_source = newSource;
}
