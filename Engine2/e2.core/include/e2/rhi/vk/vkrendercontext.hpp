
#pragma once 

#include <e2/buildcfg.hpp>
#include <e2/rhi/rendercontext.hpp>


#define VMA_STATIC_VULKAN_FUNCTIONS VK_TRUE
#define VMA_DYNAMIC_VULKAN_FUNCTIONS VK_TRUE

#include <Volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include <GLFW/glfw3.h>

namespace e2
{
	class ITexture_Vk;

	/** Presentmodes are 4 with core Vulkan, 6 with extensions. Not a build parameter. */
	static constexpr uint32_t maxVkPresentModes = 6;

	struct E2_API VkSwapchainCapabilities
	{
		e2::StackVector<VkSurfaceFormatKHR, maxVkSurfaceFormats> surfaceFormats;
		e2::StackVector<VkPresentModeKHR, maxVkPresentModes> presentModes;

		VkSurfaceCapabilitiesKHR surfaceCapabilities;
	};

	struct E2_API VkThreadLocals
	{
		bool initialized{};
		//e2::IThreadContext* threadContext{}; would be cool to cache this but no use for it yet
		// the IThreadContext is userspace utility across RHI's, VkThreadLocals is specific to Vulkan RHI implementation and opaque to user. 
		// Additionally, RHI implementation shouldn't mess with userspace functionality (including utilizing RHI interface objects)

		/** Transient pool, fence and buffer. This is for transient one-off commands, that have to wait for eaech other to finish. */
		VkCommandPool vkTransientPool{};
		VkFence vkTransientFence{};
		VkCommandBuffer vkTransientBuffer{};
		bool vkTransientShouldWait{};
	};



	class E2_API IRenderContext_Vk : public e2::IRenderContext
	{
	public:
		IRenderContext_Vk(e2::Context* engineContext, e2::Name applicationName);
		virtual ~IRenderContext_Vk();

		VkSwapchainCapabilities querySwapchainCapabilities(VkSurfaceKHR surface);

		virtual void tick(double seconds) override;

		virtual e2::RenderCapabilities const& getCapabilities() override;

		virtual e2::IFence* createFence(e2::FenceCreateInfo const& createInfo) override;

		virtual e2::ISemaphore* createSemaphore(e2::SemaphoreCreateInfo const& createInfo) override;


		virtual e2::IDataBuffer* createDataBuffer(e2::DataBufferCreateInfo const& createInfo) override;

		virtual e2::ITexture* createTexture(e2::TextureCreateInfo const& createInfo) override;

		virtual e2::ISampler* createSampler(e2::SamplerCreateInfo const& createInfo) override;

		virtual e2::IPipeline* createPipeline(e2::PipelineCreateInfo const& createInfo) override;

		virtual e2::IShader* createShader(e2::ShaderCreateInfo const& createInfo) override;

		virtual e2::IThreadContext* createThreadContext(e2::ThreadContextCreateInfo const& createInfo) override;

		virtual e2::IDescriptorSetLayout* createDescriptorSetLayout(e2::DescriptorSetLayoutCreateInfo const& createInfo) override;

		virtual e2::IPipelineLayout* createPipelineLayout(e2::PipelineLayoutCreateInfo const& createInfo) override;

		virtual e2::IVertexLayout* createVertexLayout(e2::VertexLayoutCreateInfo const& createInfo) override;
		virtual e2::IRenderTarget* createRenderTarget(e2::RenderTargetCreateInfo const& createInfo) override;

		virtual void submitBuffers(e2::RenderSubmitInfo* submitInfos, uint32_t numSubmitInfos, e2::IFence* fence) override;

		virtual void waitIdle() override;

		VkInstance m_vkInstance{};
		VkPhysicalDevice m_vkPhysicalDevice{};
		
		/** Queue family capable of graphics and GLFW present, that has the most queues */
		uint32_t m_queueFamily{};

		VkDevice m_vkDevice{};
		VkQueue m_vkQueue{};
		VmaAllocator m_vmaAllocator{};

		std::mutex m_queueMutex;

		// Validation
		VkDebugUtilsMessengerEXT m_vkValidation{};

		// 
		e2::StackVector<VkSubmitInfo, maxVkSubmitInfos> m_submitCache;

		/** 
		 * Records and submits to the transient command buffer via a predicate.
		 * If blocking is true, will block the current thread until GPU has executed this command buffer before returning.
		 * This is a slow way to issue commands and should only be used for things like copying data from a staging buffer,
		 * or setting up initial barriers for textures.
		 * Warning: Only call from persistent threads!
		 */

		template<typename Predicate>
		void submitTransient(bool blocking, Predicate pred)
		{
			e2::VkThreadLocals &threadLocals = getThreadLocals();

			transientPrepare(blocking, threadLocals);

			pred(threadLocals.vkTransientBuffer);

			transientFinalize(blocking, threadLocals);
		}

		/** Warning: Only call from persistent threads! */
		VkThreadLocals & getThreadLocals();

		

		VkSampler getOrCreateSampler(e2::SamplerFilter filter, e2::SamplerWrap wrap);

		void vkCmdTransitionImage(VkCommandBuffer buffer, VkImage image, uint32_t mips, uint32_t mipOffset, uint32_t layers, VkImageAspectFlags aspectFlags, VkImageLayout from, VkImageLayout to,
			VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VkAccessFlags sourceAccess = 0, VkAccessFlags destinationAccess = 0);
		
		void vkCmdTransitionTexture(VkCommandBuffer buffer, e2::ITexture_Vk* texture, VkImageLayout from, VkImageLayout to, 
			VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VkAccessFlags sourceAccess = 0, VkAccessFlags destinationAccess = 0);

		void vkCmdMipGenPrepareSrc(VkCommandBuffer buffer, e2::ITexture_Vk* texture);
		void vkCmdMipGenPrepareDst(VkCommandBuffer buffer, e2::ITexture_Vk* texture, uint32_t numMips);

		void vkCmdMipGenFinalizeSrc(VkCommandBuffer buffer, e2::ITexture_Vk* texture);
		void vkCmdMipGenFinalizeDst(VkCommandBuffer buffer, e2::ITexture_Vk* texture, uint32_t numMips);

	private:

		e2::RenderCapabilities m_capabilities;

		e2::StackVector<VkThreadLocals, e2::maxPersistentThreads> m_threadLocals;
		e2::StackVector<VkSampler, 16> m_samplerCache;

		void transientPrepare(bool blocking, e2::VkThreadLocals &threadLocals);
		void transientFinalize(bool blocking, e2::VkThreadLocals &threadLocals);

		void createInstance(e2::Name appName);
		void destroyInstance();
		void createValidation();
		void destroyValidation();
		void createPhysicalDevice();
		void destroyPhysicalDevice();
		void createDevice();
		void destroyDevice();
	};

}