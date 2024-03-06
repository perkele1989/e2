
#pragma once 

#include <e2/buildcfg.hpp>
#include <e2/context.hpp>
#include <e2/utils.hpp>
#include <e2/renderer/shadermodel.hpp>

#include <map>
#include <cstdint>

namespace e2
{
	enum class TerrainFlags : uint16_t
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

	struct TerrainCacheEntry
	{
		e2::IPipeline* pipeline{};
		e2::IShader* vertexShader{};
		e2::IShader* fragmentShader{};
	};


	struct E2_API TerrainData
	{
		/** Albedo.rgb, ?.a */
		alignas(16) glm::vec4 albedo;
	};





	class TerrainModel;

	/**
	 * @tags(arena, arenaSize=e2::maxNumTerrainProxies)
	 */
	class E2_API TerrainProxy : public e2::MaterialProxy
	{
		ObjectDeclaration()
	public:
		TerrainProxy(e2::Session* inSession, e2::MaterialPtr materialAsset);
		virtual ~TerrainProxy();

		virtual void bind(e2::ICommandBuffer* buffer, uint8_t frameIndex) override;
		virtual void invalidate(uint8_t frameIndex) override;

		e2::TerrainModel* model{};

		uint32_t id{};
		e2::Pair<e2::IDescriptorSet*> sets{ nullptr };

		e2::DirtyParameter<TerrainData> uniformData{};

		e2::DirtyParameter<e2::ITexture*> visibilityMask;

	};

	/** @tags(dynamic, arena, arenaSize=1) */
	class E2_API TerrainModel : public e2::ShaderModel
	{
		ObjectDeclaration()
	public:
		TerrainModel();
		virtual ~TerrainModel();
		virtual void postConstruct(e2::Context* ctx) override;

		virtual e2::MaterialProxy* createMaterialProxy(e2::Session* session, e2::MaterialPtr material) override;

		virtual e2::IPipelineLayout* getOrCreatePipelineLayout(e2::MeshProxy* proxy, uint8_t submeshIndex) override;
		virtual e2::IPipeline* getOrCreatePipeline(e2::MeshProxy* proxy, uint8_t submeshIndex, e2::RendererFlags rendererFlags) override;

		virtual void invalidatePipelines() override;

		e2::IdArena<uint32_t, e2::maxNumTerrainProxies> proxyIds;

	protected:
		friend e2::TerrainProxy;
		// Pipeline layout for creating pipelines 
		e2::IPipelineLayout* m_pipelineLayout{};

		// descriptor set layout, and pool for Terrain proxy sets 
		e2::IDescriptorSetLayout* m_descriptorSetLayout{};
		e2::IDescriptorPool* m_descriptorPool{};
		e2::ISampler* m_sampler{};

		e2::Texture2DPtr m_irradianceCube{};
		e2::Texture2DPtr m_radianceCube{};
		
		e2::Texture2DPtr m_mountainAlbedo{};
		e2::Texture2DPtr m_mountainNormal{};

		e2::Texture2DPtr m_sandAlbedo{};
		e2::Texture2DPtr m_sandNormal{};

		e2::Texture2DPtr m_fieldsAlbedo{};
		e2::Texture2DPtr m_fieldsNormal{};


		e2::Texture2DPtr m_greenAlbedo{};
		e2::Texture2DPtr m_greenNormal{};

		e2::StackVector<e2::TerrainCacheEntry, uint16_t(e2::TerrainFlags::Count)> m_pipelineCache;
		bool m_shadersReadFromDisk{};
		bool m_shadersOnDiskOK{};
		std::string m_vertexSource;
		std::string m_fragmentSource;


		e2::Pair<e2::IDataBuffer*> m_proxyUniformBuffers;
	};
}

EnumFlagsDeclaration(e2::TerrainFlags)

#include "Terrain.generated.hpp"