
#pragma once 

#include <e2/buildcfg.hpp>
#include <e2/context.hpp>
#include <e2/utils.hpp>
#include <e2/renderer/shadermodel.hpp>

#include <map>
#include <cstdint>

namespace e2
{
	constexpr uint32_t maxNumCustomProxies = 512;
	constexpr uint32_t maxNumCustomTextures = 8; // not really configurable, as it ties in to our customflags below
	constexpr uint32_t maxNumCustomParameters = 32; 

	enum class CustomFlags : uint16_t
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
		TextureFlagsOffset = 7,
		HasTexture0 = 1 << 7,
		HasTexture1 = 1 << 8,
		HasTexture2 = 1 << 9,
		HasTexture3 = 1 << 10,
		HasTexture4 = 1 << 11,
		HasTexture5 = 1 << 12,
		HasTexture6 = 1 << 13,
		HasTexture7 = 1 << 14,


		Count = 1 << 15

	};

	struct CustomCacheEntry
	{
		e2::IPipeline* pipeline{};
		e2::IShader* vertexShader{};
		e2::IShader* fragmentShader{};
	};

	class CustomModel;

	/**
	 * @tags(arena, arenaSize=e2::maxNumCustomProxies)
	 */
	class E2_API CustomProxy : public e2::MaterialProxy
	{
		ObjectDeclaration()
	public:
		CustomProxy(e2::Session* inSession, e2::MaterialPtr materialAsset);
		virtual ~CustomProxy();

		virtual void bind(e2::ICommandBuffer* buffer, uint8_t frameIndex, bool shadows) override;
		virtual void invalidate(uint8_t frameIndex) override;



		e2::CustomModel* model{};

		uint32_t id{};
		e2::Pair<e2::IDescriptorSet*> sets{ nullptr };

		e2::StackVector<e2::DirtyParameter<glm::vec4>, e2::maxNumCustomParameters> parameters;
		e2::StackVector<e2::DirtyParameter<e2::Texture2DPtr>, e2::maxNumCustomTextures> textures;

	};

	/** @tags(dynamic, arena, arenaSize=1) */
	class E2_API CustomModel : public e2::ShaderModel
	{
		ObjectDeclaration()
	public:
		CustomModel();
		virtual ~CustomModel();

		void source(std::string const& sourceFile);

		virtual void postConstruct(e2::Context* ctx) override;

		virtual e2::MaterialProxy* createMaterialProxy(e2::Session* session, e2::MaterialPtr material) override;

		virtual e2::IPipelineLayout* getOrCreatePipelineLayout(e2::MeshProxy* proxy, uint8_t lodIndex, uint8_t submeshIndex, bool shadows) override;
		virtual e2::IPipeline* getOrCreatePipeline(e2::MeshProxy* proxy, uint8_t lodIndex, uint8_t submeshIndex, e2::RendererFlags rendererFlags) override;

		virtual bool supportsShadows() override;
		virtual void invalidatePipelines() override;

		e2::IdArena<uint32_t, e2::maxNumCustomProxies> proxyIds;

		e2::Name name();

		bool valid();

		virtual e2::RenderLayer renderLayer() override;


		uint64_t getUniformBufferSize();
		int32_t getTextureSlot(e2::Name name);
		int32_t getParameterSlot(e2::Name name);

		uint32_t numParameters();
		uint32_t numTextures();

		

	protected:
		friend e2::CustomProxy;

		std::string m_sourceFile;
		bool m_valid{};

		uint64_t m_renderLayer{(uint64_t)e2::RenderLayer::Default};

		// Pipeline layout for creating pipelines 
		e2::IPipelineLayout* m_pipelineLayout{};
		e2::IPipelineLayout* m_pipelineLayoutShadows{};

		// descriptor set layout, and pool for Custom proxy sets 
		e2::IDescriptorSetLayout* m_descriptorSetLayout{};
		e2::IDescriptorPool* m_descriptorPool{};

		e2::StackVector<e2::CustomCacheEntry, uint16_t(e2::CustomFlags::Count)> m_pipelineCache;

		//std::unordered_map<e2::CustomFlags, CustomCacheEntry> m_pipelineCache;

		e2::Name m_name;

		bool m_shadersReadFromDisk{};
		bool m_shadersOnDiskOK{};
		std::string m_vertexSourcePath;
		std::string m_fragmentSourcePath;
		std::string m_vertexSource;
		std::string m_fragmentSource;
		std::string m_customDescriptorsString;


		e2::StackVector<e2::Name, e2::maxNumCustomTextures> m_textureSlots;
		e2::StackVector<e2::Name, e2::maxNumCustomParameters> m_parameterSlots;
		e2::StackVector<glm::vec4, e2::maxNumCustomParameters> m_parameterDefaults;
		e2::Pair<e2::IDataBuffer*> m_proxyUniformBuffers;
	};
}

EnumFlagsDeclaration(e2::CustomFlags)

#include "custom.generated.hpp"