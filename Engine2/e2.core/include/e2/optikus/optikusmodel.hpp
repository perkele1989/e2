
#pragma once 

#include <e2/buildcfg.hpp>
#include <e2/context.hpp>
#include <e2/utils.hpp>
#include <e2/renderer/shadermodel.hpp>

#include <map>
#include <cstdint>

namespace e2
{




	class OptikusModel;

	/**
	 * @tags(arena, arenaSize=e2::maxNumOptikusProxies)
	 */
	class E2_API OptikusProxy : public e2::MaterialProxy
	{
		ObjectDeclaration()
	public:
		OptikusProxy(e2::Session* inSession, e2::MaterialPtr materialAsset);
		virtual ~OptikusProxy();

		virtual void bind(e2::ICommandBuffer* buffer, uint8_t frameIndex, bool shadows) override;
		virtual void invalidate(uint8_t frameIndex) override;

		e2::OptikusModel* model{};

		uint32_t id{};
		e2::Pair<e2::IDescriptorSet*> sets{ nullptr };

		e2::StackVector<e2::DirtyParameter<glm::vec4>, e2::maxNumOptikusParams> parameters;
		e2::StackVector<e2::DirtyParameter<e2::ITexture*>, e2::maxNumOptikusTextures> textures;

	};

	/** @tags(dynamic, arena, arenaSize=1) */
	class E2_API OptikusModel : public e2::ShaderModel
	{
		ObjectDeclaration()
	public:
		OptikusModel();
		virtual ~OptikusModel();
		virtual void postConstruct(e2::Context* ctx) override;

		virtual e2::MaterialProxy* createMaterialProxy(e2::Session* session, e2::MaterialPtr material) override;

		virtual e2::IPipelineLayout* getOrCreatePipelineLayout(e2::MeshProxy* proxy, uint8_t submeshIndex, bool shadows) override;
		virtual e2::IPipeline* getOrCreatePipeline(e2::MeshProxy* proxy, uint8_t submeshIndex, e2::RendererFlags rendererFlags) override;

		e2::IdArena<uint32_t, e2::maxNumLightweightProxies> proxyIds;

		virtual void invalidatePipelines() override;

	protected:
		friend e2::OptikusProxy;
		
		e2::IDescriptorPool* m_descriptorPool{};

		e2::Pair<e2::IDataBuffer*> m_proxyUniformBuffers;
	};
}


#include "optikusmodel.generated.hpp"