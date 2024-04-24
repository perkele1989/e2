
#pragma once 

#include <e2/utils.hpp>
#include <e2/context.hpp>

#include <e2/export.hpp>

#include <e2/rhi/enums.hpp>

#include <e2/rhi/fence.hpp>
#include <e2/rhi/semaphore.hpp>
#include <e2/rhi/rendertarget.hpp>
#include <e2/rhi/databuffer.hpp>
#include <e2/rhi/texture.hpp>
#include <e2/rhi/pipeline.hpp>
#include <e2/rhi/descriptorsetlayout.hpp>
#include <e2/rhi/shader.hpp>
#include <e2/rhi/threadcontext.hpp>
#include <e2/rhi/vertexlayout.hpp>

#include <glm/glm.hpp>

#include <map>
#include <string>
#include <cstdint>

namespace e2
{


	class ICommandBuffer;

	using RHIFunc_CreateContext = e2::IRenderContext* (*)(e2::Context*);
	using RHIFunc_DestroyContext = void (*)(e2::IRenderContext*);

	struct E2_API RenderSubmitInfo
	{
		e2::ISemaphore* waitSemaphore{};
		e2::ISemaphore* signalSemaphore{};

		e2::ICommandBuffer* buffer{};
	};

	struct E2_API RenderCapabilities
	{
		uint32_t minUniformBufferOffsetAlignment{};
	};

	class E2_API IRenderContext : public e2::Context
	{
	public:
		IRenderContext(e2::Context* engineContext, e2::Name applicationName);
		virtual ~IRenderContext();

		virtual void tick(double seconds) = 0;

		virtual e2::RenderCapabilities const& getCapabilities() = 0;

		virtual e2::IFence *createFence(e2::FenceCreateInfo const& createInfo) = 0;

		virtual e2::ISemaphore* createSemaphore(e2::SemaphoreCreateInfo const& createInfo) = 0;

		virtual e2::IDataBuffer *createDataBuffer(e2::DataBufferCreateInfo const& createInfo) = 0;

		virtual e2::ITexture *createTexture(e2::TextureCreateInfo const& createInfo) = 0;

		virtual e2::ISampler* createShadowSampler() = 0;

		virtual e2::ISampler* createSampler(e2::SamplerCreateInfo const& createInfo) = 0;

		virtual e2::IPipeline *createPipeline(e2::PipelineCreateInfo const& createInfo) = 0;

		virtual e2::IShader *createShader(e2::ShaderCreateInfo const& createInfo) = 0;

		virtual e2::IThreadContext *createThreadContext(e2::ThreadContextCreateInfo const& createInfo) = 0;

		virtual e2::IDescriptorSetLayout* createDescriptorSetLayout(e2::DescriptorSetLayoutCreateInfo const& createInfo) = 0;

		virtual e2::IPipelineLayout* createPipelineLayout(e2::PipelineLayoutCreateInfo const& createInfo) = 0;

		virtual e2::IVertexLayout* createVertexLayout(e2::VertexLayoutCreateInfo const& createInfo) = 0;

		virtual e2::IRenderTarget* createRenderTarget(e2::RenderTargetCreateInfo const& createInfo) = 0;

		virtual void submitBuffers(e2::RenderSubmitInfo* submitInfos, uint32_t numSubmitInfos, e2::IFence* fence) = 0;
		
		virtual void waitIdle() = 0;

		virtual e2::Engine* engine() override;

	protected:
		e2::Engine* m_engine{};

#if defined(E2_DEVELOPMENT)

	public:
		void printLeaks();
	protected:
		friend e2::RenderResource;
		struct ResourceTracker
		{
			e2::RenderResource* resource{};
			std::string callStack;
		};
		std::map<e2::RenderResource*, ResourceTracker> m_trackers;
#endif

	};

}