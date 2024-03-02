
#pragma once 

#include <e2/buildcfg.hpp>

#include <e2/rhi/vk/vkresource.hpp>
#include <e2/rhi/threadcontext.hpp>

#include <Volk/volk.h>
#include <GLFW/glfw3.h>

namespace e2
{
	class ICommandPool_Vk;
	class IRenderTarget_Vk;

	/** @tags(arena, arenaSize=128) @todo adjust arena parameters */
	class E2_API ICommandBuffer_Vk : public e2::ICommandBuffer, public e2::ThreadHolder_Vk
	{
		ObjectDeclaration()
	public:
		ICommandBuffer_Vk(IThreadContext* context, e2::ICommandPool_Vk* pool, e2::CommandBufferCreateInfo const& createInfo);
		virtual ~ICommandBuffer_Vk();

		virtual void reset() override;

		virtual void beginRecord(bool oneShot, e2::PipelineSettings const &defaultSettings ) override;
		virtual void endRecord() override;

		// dynamic states
		virtual void setDepthTest(bool newDepthTest) override;
		virtual void setDepthWrite(bool newDepthWrite) override;
		virtual void setStencilTest(bool newStencilTest) override;
		virtual void setFrontFace(e2::FrontFace newFrontFace) override;
		virtual void setCullMode(e2::CullMode newCullMode) override;

		virtual void setScissor(glm::uvec2 offset, glm::uvec2 size) override;
		virtual void beginRender(e2::IRenderTarget* renderTarget) override;
		virtual void endRender() override;


		virtual void clearColor(uint32_t attachmentIndex, glm::vec4 const& color) override;
		virtual void clearColor(uint32_t attachmentIndex, glm::uvec4 const& color) override;
		virtual void clearColor(uint32_t attachmentIndex, glm::ivec4 const& color) override;
		virtual void clearDepth(float depth) override;
		virtual void clearStencil(uint32_t stencil) override;


		virtual void bindPipeline(e2::IPipeline* pipeline) override;

		virtual void bindDescriptorSet(e2::IPipelineLayout* layout, uint32_t setIndex, e2::IDescriptorSet* descriptorSet, uint32_t numDynamicOffsets, uint32_t *dynamicOffsets) override;

		virtual void nullVertexLayout() override;
		virtual void bindVertexLayout(e2::IVertexLayout* vertexLayout) override;
		virtual void bindVertexBuffer(uint32_t binding, e2::IDataBuffer* dataBuffer) override;
		virtual void bindIndexBuffer(e2::IDataBuffer* dataBuffer) override;

		virtual void pushConstants(e2::IPipelineLayout* layout, uint32_t offset, uint32_t size, uint8_t const* data) override;

		virtual void draw(uint32_t indexCount, uint32_t instanceCount) override;
		virtual void drawNonIndexed(uint32_t vertexCount, uint32_t instanceCount) override;

		virtual void useAsDescriptor(e2::ITexture* texture) override;
		virtual void useAsDepthStencilAttachment(e2::ITexture* texture) override;
		virtual void useAsDepthAttachment(e2::ITexture* texture) override;
		virtual void useAsAttachment(e2::ITexture* texture) override;
		virtual void useAsDefault(e2::ITexture* texture) override;

		virtual void useAsTransferDst(e2::ITexture* texture) override;
		virtual void useAsTransferSrc(e2::ITexture* texture) override;

		virtual void blit(e2::ITexture* dst, e2::ITexture* src, uint8_t dstMip, uint8_t srcMip) override;

		e2::IRenderTarget_Vk* m_currentTarget{};

		VkCommandBuffer m_vkHandle{};
		ICommandPool_Vk* m_pool{};
	};


	/** @tags(arena, arenaSize=32) @todo adjust arena parameters */
	class E2_API ICommandPool_Vk : public e2::ICommandPool, public e2::ThreadHolder_Vk
	{
		ObjectDeclaration()
	public:
		ICommandPool_Vk(IThreadContext* context, e2::CommandPoolCreateInfo const& createInfo);
		virtual ~ICommandPool_Vk();

		virtual void reset() override;
		virtual ICommandBuffer* createBuffer(CommandBufferCreateInfo const& createInfo) override;

		VkCommandPool m_vkHandle{};

	};

	/** @tags(arena, arenaSize=32) @todo adjust arena parameters */
	class E2_API IDescriptorPool_Vk : public e2::IDescriptorPool, public e2::ThreadHolder_Vk
	{
		ObjectDeclaration()
	public:
		IDescriptorPool_Vk(IThreadContext* context, e2::DescriptorPoolCreateInfo const& createInfo);
		virtual ~IDescriptorPool_Vk();

		virtual e2::IDescriptorSet* createDescriptorSet(IDescriptorSetLayout* setLayout) override;

		VkDescriptorPool m_vkHandle{};

	};

	/** @tags(arena, arenaSize=4096) @todo adjust arena parameters */
	class E2_API IDescriptorSet_Vk : public e2::IDescriptorSet, public e2::ThreadHolder_Vk
	{
		ObjectDeclaration()
	public:
		IDescriptorSet_Vk(e2::IDescriptorPool* descriptorPool, IDescriptorSetLayout* setLayout);
		virtual ~IDescriptorSet_Vk();

		virtual void writeSampler(uint32_t binding, e2::ISampler* sampler) override;

		virtual void writeTexture(uint32_t binding, e2::ITexture* textures, uint32_t elementOffset = 0, uint32_t numTextures = 1) override;

		virtual void writeUniformBuffer(uint32_t binding, e2::IDataBuffer* uniformBuffer, uint32_t size, uint32_t offset) override;

		virtual void writeDynamicBuffer(uint32_t binding, e2::IDataBuffer* dynamicBuffer, uint32_t constantSize, uint32_t constantOffset) override;

		virtual void writeStorageBuffer(uint32_t binding, e2::IDataBuffer* storageBuffer, uint32_t size, uint32_t offset) override;

		VkDescriptorPool m_vkPool{};
		VkDescriptorSet m_vkHandle{};

		//e2::StackVector<VkImage, e2::maxNumDescriptorImages> m_textureCache;

	};

	/** @tags(arena, arenaSize=32) */
	class E2_API IThreadContext_Vk : public e2::IThreadContext, public e2::ContextHolder_Vk
	{
		ObjectDeclaration()
	public:
		IThreadContext_Vk(IRenderContext* context, e2::ThreadContextCreateInfo const& createInfo);
		virtual ~IThreadContext_Vk();

		virtual e2::IWindow* createWindow(e2::WindowCreateInfo const& createInfo) override;

		virtual e2::ICommandPool* createCommandPool(e2::CommandPoolCreateInfo const& createInfo) override;

		virtual e2::IDescriptorPool* createDescriptorPool(e2::DescriptorPoolCreateInfo const& createInfo) override;

		virtual e2::ICommandPool* getPersistentPool() override;



		e2::ICommandPool_Vk* m_persistentPool{};
		 
		e2::ThreadInfo m_threadInfo{};
	};
}

#include "vkthreadcontext.generated.hpp"