
#pragma once 

#include <e2/export.hpp>
#include <e2/rhi/renderresource.hpp>
#include <e2/rhi/descriptorsetlayout.hpp>
#include <e2/rhi/window.hpp>
#include <e2/rhi/enums.hpp>
#include <e2/rhi/texture.hpp>
#include <glm/glm.hpp>

#include <cstdint>

namespace e2
{

	class IPipeline;
	class IDescriptorPool;
	class IDataBuffer;
	class IRenderTarget;
	class IVertexLayout;
	class IPipelineLayout;
	class IDescriptorSet;

	enum class FrontFace : uint8_t
	{
		CW = 0,
		CCW
	};

	enum class CullMode : uint8_t
	{
		None = 0,
		Front = 1 << 0,
		Back = 1 << 1,
		Both = Front | Back
	};

	struct PipelineSettings
	{
		e2::CullMode cullMode { e2::CullMode::Back };
		e2::FrontFace frontFace { e2::FrontFace::CW };
		bool depthTest{ true };
		bool depthWrite{ true };
		bool depthBoundsTest{ false };
		bool stencilTest{ false };

		/*
			vkCmdSetCullMode(m_vkHandle, VK_CULL_MODE_NONE);
			vkCmdSetFrontFace(m_vkHandle, VK_FRONT_FACE_CLOCKWISE);
			vkCmdSetDepthTestEnable(m_vkHandle, VK_FALSE);
			vkCmdSetDepthWriteEnable(m_vkHandle, VK_TRUE);
			vkCmdSetDepthCompareOp(m_vkHandle, VK_COMPARE_OP_LESS);
			vkCmdSetDepthBoundsTestEnable(m_vkHandle, VK_FALSE);
			vkCmdSetStencilTestEnable(m_vkHandle, VK_FALSE);
			vkCmdSetStencilOp(m_vkHandle, VK_STENCIL_FACE_FRONT_AND_BACK, VK_STENCIL_OP_ZERO, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_ZERO, VK_COMPARE_OP_EQUAL );
			vkCmdSetDepthBiasEnable(m_vkHandle, VK_FALSE);
		*/
	};



	struct E2_API CommandBufferCreateInfo
	{
		bool secondary{};
	};


	class E2_API ICommandBuffer : public e2::ThreadResource
	{
		ObjectDeclaration()
	public:
		ICommandBuffer(IThreadContext* context, e2::CommandBufferCreateInfo const& createInfo);
		virtual ~ICommandBuffer();

		virtual void reset() = 0;

		virtual void beginRecord(bool oneShot, e2::PipelineSettings const& defaultSettings) = 0;
		virtual void endRecord() = 0;

		// dynamic states
		virtual void setDepthTest(bool newDepthTest) = 0;
		virtual void setDepthWrite(bool newDepthWrite) = 0;
		virtual void setStencilTest(bool newStencilWrite) = 0;
		virtual void setFrontFace(e2::FrontFace newFrontFace) = 0;
		virtual void setCullMode(e2::CullMode newCullMode) = 0;
		virtual void setScissor(glm::uvec2 offset, glm::uvec2 size) = 0;
		// @todo rest of dynamic states 


		virtual void beginRender(e2::IRenderTarget *renderTarget) = 0;
		virtual void endRender() = 0;

		virtual void clearColor(uint32_t attachmentIndex, glm::vec4 const& color)=0;
		virtual void clearColor(uint32_t attachmentIndex, glm::uvec4 const& color) = 0;
		virtual void clearColor(uint32_t attachmentIndex, glm::ivec4 const& color) = 0;
		virtual void clearDepth(float depth) = 0;
		virtual void clearStencil(uint32_t stencil) = 0;


		virtual void bindPipeline(e2::IPipeline *pipeline) = 0;

		virtual void bindDescriptorSet(e2::IPipelineLayout* layout, uint32_t setIndex, e2::IDescriptorSet* descriptorSet, uint32_t numDynamicOffsets = 0, uint32_t* dynamicOffsets = nullptr) = 0;
		virtual void nullVertexLayout() =0;
		virtual void bindVertexLayout(e2::IVertexLayout* vertexLayout) = 0;
		virtual void bindVertexBuffer(uint32_t binding, e2::IDataBuffer* dataBuffer) = 0;
		virtual void bindIndexBuffer(e2::IDataBuffer* dataBuffer) = 0;

		virtual void pushConstants(e2::IPipelineLayout* layout, uint32_t offset, uint32_t size, uint8_t const* data) = 0;

		virtual void draw(uint32_t indexCount, uint32_t instanceCount) = 0;
		virtual void drawNonIndexed(uint32_t vertexCount, uint32_t instanceCount) = 0;

		virtual void useAsDescriptor(e2::ITexture* texture) = 0;
		virtual void useAsAttachment(e2::ITexture* texture) = 0;
		virtual void useAsDefault(e2::ITexture* texture) = 0;
		virtual void useAsDepthStencilAttachment(e2::ITexture* texture) = 0;
		virtual void useAsDepthAttachment(e2::ITexture* texture) = 0;
		virtual void useAsTransferDst(e2::ITexture* texture) = 0;
		virtual void useAsTransferSrc(e2::ITexture* texture) = 0;

		virtual void blit(e2::ITexture* dst, e2::ITexture* src, uint8_t dstMip, uint8_t srcMip) = 0;
	};

	struct E2_API CommandPoolCreateInfo
	{
		/** If true, hint to driver that command buffers are not very persistent */
		bool transient{};

		/** If true, individual command buffers can be reset and reused */
		bool enableReset{};
	};

	class E2_API ICommandPool : public e2::ThreadResource
	{
		ObjectDeclaration()
	public:
		ICommandPool(IThreadContext* context, e2::CommandPoolCreateInfo const& createInfo);
		virtual ~ICommandPool();

		virtual void reset() = 0;

		virtual ICommandBuffer* createBuffer(CommandBufferCreateInfo const& createInfo) = 0;

	};




	class E2_API IDescriptorSet : public e2::ThreadResource
	{
		ObjectDeclaration()
	public:
		IDescriptorSet(e2::IDescriptorPool* descriptorPool, e2::IDescriptorSetLayout* setLayout);
		virtual ~IDescriptorSet();

		virtual void writeSampler(uint32_t binding, e2::ISampler* sampler) = 0;

		virtual void writeTexture(uint32_t binding, e2::ITexture* textures, uint32_t elementOffset = 0, uint32_t numTextures = 1) = 0;

		virtual void writeUniformBuffer(uint32_t binding, e2::IDataBuffer* uniformBuffer, uint32_t size, uint32_t offset) = 0;

		virtual void writeDynamicBuffer(uint32_t binding, e2::IDataBuffer* dynamicBuffer, uint32_t constantSize, uint32_t constantOffset) = 0;

		virtual void writeStorageBuffer(uint32_t binding, e2::IDataBuffer* storageBuffer, uint32_t size, uint32_t offset) = 0;
	};



	struct E2_API DescriptorPoolCreateInfo
	{
		/** Maximum number of sets */
		uint32_t maxSets{ };

		/** Total number of textures */
		uint32_t numTextures{};

		/** Total number of samplers */
		uint32_t numSamplers{};

		/** Total number of uniform buffers */
		uint32_t numUniformBuffers{};

		/** Total number of dynamic (uniform) buffers */
		uint32_t numDynamicBuffers{};

		/** Total number of storage buffers*/
		uint32_t numStorageBuffers{};

		bool allowUpdateAfterBind{ false };
	};

	class E2_API IDescriptorPool : public e2::ThreadResource
	{
		ObjectDeclaration()
	public:
		IDescriptorPool(IThreadContext* context, e2::DescriptorPoolCreateInfo const& createInfo);
		virtual ~IDescriptorPool();

		virtual e2::IDescriptorSet* createDescriptorSet(IDescriptorSetLayout* setLayout) = 0;

	};


	struct E2_API ThreadContextCreateInfo
	{

	};

	class E2_API IThreadContext : public e2::RenderResource
	{
		ObjectDeclaration()
	public:
		IThreadContext(IRenderContext* context, ThreadContextCreateInfo const& createInfo);
		virtual ~IThreadContext();

		virtual e2::IWindow* createWindow(e2::WindowCreateInfo const& createInfo) = 0;

		virtual e2::ICommandPool *createCommandPool(e2::CommandPoolCreateInfo const& createInfo) = 0;

		virtual e2::IDescriptorPool *createDescriptorPool(e2::DescriptorPoolCreateInfo const& createInfo) = 0;

		/** Returns a pool that is never reset, but can have reset flags on the buffers it generates */
		virtual e2::ICommandPool* getPersistentPool() = 0;

	};

}

#include "threadcontext.generated.hpp"