
#pragma once 

#include <e2/buildcfg.hpp>

#include <e2/rhi/vk/vkresource.hpp>
#include <e2/rhi/window.hpp>

#include <Volk/volk.h>
#include <GLFW/glfw3.h>

#include <unordered_set>

namespace e2
{


	/** @tags(arena, arenaSize=e2::maxVkWindows) */
	class E2_API IWindow_Vk : public e2::IWindow, public e2::ThreadHolder_Vk
	{
		ObjectDeclaration()
	public:
		IWindow_Vk(IThreadContext* context, e2::WindowCreateInfo const& createInfo);
		virtual ~IWindow_Vk();

		virtual void title(std::string const& newTitle) override;
		virtual void source(ITexture *newSource) override;

		virtual void wantsClose(bool newValue) override;
		virtual bool wantsClose() const override;

		virtual void present() override;

		virtual glm::uvec2 size() override;
		virtual void size(glm::uvec2 const& newSize) override;

		virtual void cursor(e2::CursorShape newCursor) override;

		void onResized(glm::uvec2 newSize);

		virtual glm::vec2 position() override;
		virtual void position(glm::vec2 const& newPosition) override;

		virtual void maximize() override;
		virtual void restore() override;
		virtual void minimize() override;
		virtual bool isMaximized() override;
		virtual bool isFocused() override;

		virtual void mouseLock(bool newMouseLock) override;

		void registerInputHandler(e2::IWindowInputHandler* handler) override;
		void unregisterInputHandler(e2::IWindowInputHandler* handler) override;

		GLFWwindow* m_glfwHandle{};
		VkSurfaceKHR m_vkSurface{};

		VkSwapchainKHR m_vkSwapchain{ nullptr  };

		e2::Pair<VkImage> m_vkSwapchainImages { nullptr };
		e2::Pair<VkImageView> m_vkSwapchainImageViews { nullptr };

		e2::Pair<VkFence> m_vkFences { nullptr };

		e2::Pair<VkSemaphore> m_vkSemaphoresAvailable { nullptr };
		e2::Pair<VkSemaphore> m_vkSemaphoresEndFrame { nullptr };

		e2::Pair<VkCommandBuffer> m_vkCommandBuffers { nullptr };

		/** Internal state, so needs to be mutable */
		mutable uint32_t m_currentFrame{};

		e2::ITexture* m_source{};

		glm::uvec2 m_size{};

		uint32_t m_skipPresent{};

		std::unordered_set<e2::IWindowInputHandler*> m_inputHandlers;

		bool m_wantsResize{ false };
		glm::vec2 m_newSize{};

		bool m_swapchainDirty{};


	protected:

		void createGlfwWindow(e2::WindowCreateInfo const& createInfo);
		void destroyGlfwWindow();
		
		void createSurface();
		void destroySurface();

		void createSwapchain();
		void destroySwapchain();

		void createCommandBuffers();
		void destroyCommandBuffers();

		void recordCommandBuffer();

		void createTemp();
		void destroyTemp();
		void recreateTemp();

		
	};
}

#include "vkwindow.generated.hpp"