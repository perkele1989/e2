
#pragma once 

#include <e2/buildcfg.hpp>
#include <e2/context.hpp>
#include <e2/utils.hpp>
#include <e2/renderer/shadermodel.hpp>

#include <map>
#include <cstdint>

namespace e2
{
	enum class FogFlags : uint16_t
	{
		None = 0,

		// Inherited from VertexAttributeFlags
		VertexFlagsOffset = 0,
		Normal = 1 << 0, // Normal implies normal vectors AND tangent vectors (bitangent always derived in shader)
		TexCoords01 = 1 << 1, // Has at least 1 texcoords
		TexCoords23 = 1 << 2, // Has at least 3 texcoords
		Color = 1 << 3, // Has vertex colors
		Bones = 1 << 4, // Has bone weights and ids

		// Inherited from RendererFlags 
		RendererFlagsOffset = 5,
		Shadow = 1 << 5,
		Skin = 1 << 6,

		// Maps 
		MaterialFlagsOffset = 7,

		Count = 1 << 7

	};

	struct FogCacheEntry
	{
		e2::IPipeline* pipeline{};
		e2::IShader* vertexShader{};
		e2::IShader* fragmentShader{};
	};


	struct E2_API FogData
	{
		/** Albedo.rgb, ?.a */
		alignas(16) glm::vec4 albedo;
	};





	class FogModel;

	/**
	 * @tags(arena, arenaSize=e2::maxNumWaterProxies) // @todo
	 */
	class E2_API FogProxy : public e2::MaterialProxy
	{
		ObjectDeclaration()
	public:
		FogProxy(e2::Session* inSession, e2::MaterialPtr materialAsset);
		virtual ~FogProxy();

		virtual void bind(e2::ICommandBuffer* buffer, uint8_t frameIndex, bool shadows) override;
		virtual void invalidate(uint8_t frameIndex) override;

		e2::FogModel* model{};

		uint32_t id{};
		e2::Pair<e2::IDescriptorSet*> sets{ nullptr };

		e2::DirtyParameter<FogData> uniformData{};
		e2::DirtyParameter<e2::ITexture*> visibilityMasks[2];

	};

	/** @tags(dynamic, arena, arenaSize=1) */
	class E2_API FogModel : public e2::ShaderModel
	{
		ObjectDeclaration()
	public:
		FogModel();
		virtual ~FogModel();
		virtual void postConstruct(e2::Context* ctx) override;

		virtual e2::MaterialProxy* createMaterialProxy(e2::Session* session, e2::MaterialPtr material) override;

		virtual e2::IPipelineLayout* getOrCreatePipelineLayout(e2::MeshProxy* proxy, uint8_t submeshIndex, bool shadows) override;
		virtual e2::IPipeline* getOrCreatePipeline(e2::MeshProxy* proxy, uint8_t submeshIndex, e2::RendererFlags rendererFlags) override;

		e2::IdArena<uint32_t, e2::maxNumWaterProxies> proxyIds;

		virtual void invalidatePipelines() override;

		virtual e2::RenderLayer renderLayer() override;

		

	protected:
		friend e2::FogProxy;
		// Pipeline layout for creating pipelines 
		e2::IPipelineLayout* m_pipelineLayout{};

		// descriptor set layout, and pool for water proxy sets 
		e2::IDescriptorSetLayout* m_descriptorSetLayout{};
		e2::IDescriptorPool* m_descriptorPool{};

		e2::Texture2DPtr m_cubemap{};

		e2::StackVector<e2::FogCacheEntry, uint16_t(e2::FogFlags::Count)> m_pipelineCache;
		bool m_shadersReadFromDisk{};
		bool m_shadersOnDiskOK{};
		std::string m_vertexSource;
		std::string m_fragmentSource;

		e2::Pair<e2::IDataBuffer*> m_proxyUniformBuffers;
	};
}

EnumFlagsDeclaration(e2::FogFlags)

#include "fog.generated.hpp"