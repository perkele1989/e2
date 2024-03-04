
#pragma once 

#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/manager.hpp>

#include <e2/assets/texture2d.hpp>
#include <e2/assets/font.hpp>

#include <e2/renderer/renderer.hpp>
#include <e2/renderer/meshproxy.hpp>
#include <e2/renderer/shared.hpp>

#include <set>
#include <map>


namespace e2
{
	class Engine;
	class ShaderModel;
	class IRenderContext;
	class IThreadContext;
	class IFence;





	class E2_API RenderCallbacks
	{
	public:
		/** Invoked when a new frame is ready */
		virtual void onNewFrame(uint8_t frameIndex) = 0;

		/** Invoked before final command submission */
		virtual void preDispatch(uint8_t frameIndex) = 0;

		/** Invoked after final command submission */
		virtual void postDispatch(uint8_t frameIndex) = 0;
	};



	class E2_API RenderManager : public Manager
	{
	public:
		RenderManager(Engine *owner);
		virtual ~RenderManager();

		virtual void initialize() override;
		virtual void shutdown() override;


		virtual void update(double deltaTime) override;

		e2::IRenderContext* renderContext();

		e2::IThreadContext* mainThreadContext();

		// queue a recorded buffer for submission for the current frame
		void queue(e2::ICommandBuffer* buffer, e2::ISemaphore* waitSemaphore, e2::ISemaphore* signalSemaphore);

		// dispatches recorded buffers, flips frame index
		void dispatch();

		inline uint8_t frameIndex() const
		{
			return m_frameIndex;
		}

		e2::ICommandPool* framePool(uint8_t frameIndex);

		void registerCallbacks(e2::RenderCallbacks* callbacks);
		void unregisterCallbacks(e2::RenderCallbacks* callbacks);

		uint32_t paddedBufferSize(uint32_t originalSize);

		e2::IDescriptorPool* modelPool()
		{
			return m_modelPool;
		}

		e2::IDescriptorSetLayout* modelSetLayout()
		{
			return m_modelSetLayout;
		}

		e2::IDescriptorPool* rendererPool()
		{
			return m_rendererPool;
		}

		e2::IDescriptorSetLayout* rendererSetLayout()
		{
			return m_rendererSetLayout;
		}

		e2::IVertexLayout* getOrCreateVertexLayout(e2::VertexAttributeFlags flags);
		/*e2::IVertexLayout* nullVertexLayout()
		{
			return m_nullVertexLayout;
		}*/

		e2::ShaderModel* getShaderModel(e2::Name modelName);
		
		e2::ISampler* frontBufferSampler();
		e2::ISampler* brdfSampler();
		e2::Texture2DPtr integratedBrdf();

		e2::Texture2DPtr defaultTexture();
		e2::FontPtr defaultFont(e2::FontFace face);
		e2::MaterialPtr defaultMaterial();


		e2::IPipelineLayout* linePipelineLayout()
		{
			return m_linePipelineLayout;
		}

		e2::IShader* fullscreenTriangleShader();

		void invalidatePipelines();

	protected:

		e2::IRenderContext* m_renderContext{};
		e2::IThreadContext* m_mainThreadContext{};

		e2::ISampler* m_frontBufferSampler;
		e2::ISampler* m_brdfSampler;
		e2::Texture2DPtr m_integratedBrdf;

		e2::IShader* m_fullscreenTriangleShader{};

		e2::Texture2DPtr m_defaultTexture;
		e2::FontPtr m_defaultFont[size_t(e2::FontFace::Count)];
		e2::Material* m_defaultMaterial{};

		uint8_t m_frameIndex{};
		e2::Pair<e2::IFence*> m_fences {nullptr};
		e2::Pair<e2::ICommandPool*> m_framePools {nullptr};
		e2::Pair<bool> m_queueOpen{false};

		e2::StackVector<e2::ShaderModel*, e2::maxNumShaderModels> m_shaderModels;

		/** descriptor pool and set layout for model sets (modelmatrices) */
		e2::IDescriptorPool* m_modelPool{};
		e2::IDescriptorSetLayout* m_modelSetLayout{};

		/** descriptor pool and set layout for rendererdata (one renderer gets two sets each) */
		e2::IDescriptorPool* m_rendererPool{};
		e2::IDescriptorSetLayout* m_rendererSetLayout{};

		e2::IPipelineLayout* m_linePipelineLayout{};

		//e2::IVertexLayout* m_nullVertexLayout{};

		e2::StackVector<e2::RenderSubmitInfo, e2::maxNumQueuedBuffers> m_queue;
		e2::StackVector<e2::RenderCallbacks*, e2::maxNumRenderCallbacks> m_callbacks;

		e2::StackVector<e2::IVertexLayout*, e2::maxNumAttributePermutations> m_vertexLayoutCache;

	};

}
