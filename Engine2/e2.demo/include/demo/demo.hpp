
#pragma once 

#define GLM_ENABLE_EXPERIMENTAL

#include <e2/export.hpp>
#include <e2/application.hpp>
#include <e2/renderer/shadermodel.hpp>
#include <e2/renderer/meshproxy.hpp>
#include <e2/managers/uimanager.hpp>
#include <set>

namespace e2
{

	class Demo;
	class ToolWindow : public e2::UIWindow
	{
	public:
		ToolWindow(e2::Demo* demo);
		virtual ~ToolWindow();

		virtual void update(double deltaTime) override;
		virtual void onSystemButton() override;

		virtual e2::Engine* engine() override;

	protected:
		
		e2::Demo* m_demo{};

	};





	enum class DemoStageIndex : uint64_t
	{
		FinalRender = 0, // 0, 0
		Height_Basic, // 1, 0
		Normal_Basic, // 2, 0
		Height_Blend, // 1, 1
		Normal_Blend, // 2, 1
		BaseLayer_SingleColor, // 3, 0
		BaseLayer_HeightBlend, // 3, 1
		BaseLayer_SunLight, // 3, 2
		BaseLayer_SunLightBlend, // 3, 3
		Reflection_Cubemap, // 4,0
		Reflection_Fresnel, // 4,1
		Reflection_FresnelHeight, // 4,2
		Reflection_Blend, // 4,3
		BottomLayer_ShoreColor, // 5, 0
		BottomLayer_DeepBlend, // 5, 1
		BottomLayer_BaseBlend, // 5,2
		BottomLayer_ShoreBlend, // 5,3
		Caustics_Mask, // 6,0
		Caustics_Only, // 6,1
		Caustics_BottomBlend, // 6,2
		Refraction_Offset, // 7,0
		Refraction_Only, // 7,1
		Refraction_Blend, // 7,2
		Refraction_Fixup, // 7,3
		Count
	};

	struct DemoStage
	{
		std::string id;
		std::string layerName;
		std::string description;
		std::vector<std::string> codeSnippets;

		uint32_t shaderIndex;
		uint32_t shaderSubIndex;

		//bool animateHeight;
		//float waterHeight;
		//float viewAngleB;
		//float viewAngleA;
	};

	enum class DemoFlags : uint16_t
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
		DemoFlagsOffset = 7,

		Count = 1 << 7

	};

	struct DemoCacheEntry
	{
		e2::IPipeline* pipeline{};
		e2::IShader* vertexShader{};
		e2::IShader* fragmentShader{};
	};


	struct DemoData
	{
		/** Albedo.rgb, ?.a */
		alignas(16) glm::vec4 albedo;
		alignas(16) glm::vec4 water;
		alignas(16) glm::uvec4 water2;
	};

	class DemoModel;

	constexpr uint64_t maxNumDemoProxies = 32;

	/**
	 * @tags(arena, arenaSize=e2::maxNumDemoProxies)
	 */
	class DemoProxy : public e2::MaterialProxy
	{
		ObjectDeclaration()
	public:
		DemoProxy(e2::Session* inSession, e2::MaterialPtr materialAsset);
		virtual ~DemoProxy();

		virtual void bind(e2::ICommandBuffer* buffer, uint8_t frameIndex, bool shadows) override;
		virtual void invalidate(uint8_t frameIndex) override;

		e2::DemoModel* model{};

		uint32_t id{};
		e2::Pair<e2::IDescriptorSet*> sets{ nullptr };

		e2::DirtyParameter<DemoData> uniformData{};

		e2::DirtyParameter<e2::ITexture*> reflectionHdr;
	};

	/** @tags(dynamic, arena, arenaSize=1) */
	class DemoModel : public e2::ShaderModel
	{
		ObjectDeclaration()
	public:
		DemoModel();
		virtual ~DemoModel();
		virtual void postConstruct(e2::Context* ctx) override;

		virtual e2::MaterialProxy* createMaterialProxy(e2::Session* session, e2::MaterialPtr material) override;

		virtual e2::IPipelineLayout* getOrCreatePipelineLayout(e2::MeshProxy* proxy, uint8_t lodIndex, uint8_t submeshIndex, bool shadows) override;
		virtual e2::IPipeline* getOrCreatePipeline(e2::MeshProxy* proxy, uint8_t lodIndex, uint8_t submeshIndex, e2::RendererFlags rendererFlags) override;

		e2::IdArena<uint32_t, e2::maxNumDemoProxies> proxyIds;

		virtual void invalidatePipelines() override;

		virtual e2::RenderLayer renderLayer() override;

	protected:
		friend e2::DemoProxy;
		// Pipeline layout for creating pipelines 
		e2::IPipelineLayout* m_pipelineLayout{};

		// descriptor set layout, and pool for water proxy sets 
		e2::IDescriptorSetLayout* m_descriptorSetLayout{};
		e2::IDescriptorPool* m_descriptorPool{};

		e2::StackVector<e2::DemoCacheEntry, uint16_t(e2::DemoFlags::Count)> m_pipelineCache;
		bool m_shadersReadFromDisk{};
		bool m_shadersOnDiskOK{};
		std::string m_vertexSource;
		std::string m_fragmentSource;

		e2::Pair<e2::IDataBuffer*> m_proxyUniformBuffers;
	};



	struct EnvMap
	{
		e2::Texture2DPtr irradiance;
		e2::Texture2DPtr radiance;
	};
	
	class SkyProxy;

	class Demo : public e2::Application
	{
	public:

		Demo(e2::Context* ctx);
		virtual ~Demo();

		virtual void initialize() override;
		virtual void shutdown() override;
		virtual void update(double seconds) override;


		virtual ApplicationType type() override;

		void setupStages();
		void buildWater();


		int32_t selectedMap{ 1 };
		int32_t selectedWaterMap{ 1 };
		float viewAngleA{ 135.0f };
		float viewAngleB{ 35.0f };
		float viewDistance{ 9.5f };
		float viewFov{ 20.0f };
		float sunAngleA{ -75.0f };
		float sunAngleB{ 21.75f };
		float sunStrength{ 10.2f };
		float iblStrength{ 8.2f };
		float exposure{ 1.0f };
		float whitepoint{ 10.0f };
		float waterHeight{ 0.20f };

		glm::quat currOrientationA{ glm::identity<glm::quat>()};
		glm::quat currOrientationB{ glm::identity<glm::quat>() };

		e2::StackVector<EnvMap, 4> envMaps;

		e2::MeshPtr nimbleMeshAsset{};
		e2::SkeletonPtr nimbleSkeletonAsset{};
		e2::AnimationPtr nimbleAnimationAsset{};
		e2::MeshProxy* nimbleMesh{};
		e2::SkinProxy* nimbleSkin{};

		e2::ToolWindow* toolWindow{};
		e2::MeshProxy* floorMesh{};
		e2::MeshPtr waterMeshAsset;
		e2::MeshPtr testMeshAsset;
		e2::MeshProxy* testMesh{};
		e2::MaterialPtr waterMaterial;
		e2::DemoProxy* waterProxy{};
		e2::SkyProxy* skyProxy{};
		e2::MaterialPtr skyMaterial;
		e2::MeshProxy* waterMesh{};

		float waterHeight_Curr{ 0.25f }; // caustics 0.4, 0.0 - 0.5 anim
		bool waterHeight_Animate = false;
		float waterHeight_AnimateTime = 0.0f;
		int32_t waterStage{ 0 };
		int32_t waterStage2{ 0 };
		bool renderWater{ true };
		bool renderNimble{ false };

		bool drawGrid{ false };
		bool animateGrid{ false };
		bool renderExtra{ true };


		e2::StackVector<DemoStage, (uint64_t)DemoStageIndex::Count> stages;
		DemoStageIndex currentStage{ DemoStageIndex::Height_Basic };

		void applyStage(DemoStageIndex index);

		float waterScale{ 0.125 };
		float normalStrength{ 20.0 };
		float normalDistance{ 0.015 };
		float refractionStrength{ 30.0 };
	protected:

		e2::GameSession* m_session{};


	};

}


EnumFlagsDeclaration(e2::DemoFlags)











namespace e2
{


	enum class SkyFlags : uint16_t
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
		DemoFlagsOffset = 7,

		Count = 1 << 7

	};

	struct SkyCacheEntry
	{
		e2::IPipeline* pipeline{};
		e2::IShader* vertexShader{};
		e2::IShader* fragmentShader{};
	};


	struct SkyData
	{
		/** Albedo.rgb, ?.a */
		alignas(16) glm::vec4 albedo;
	};

	class SkyModel;

	constexpr uint64_t maxNumSkyProxies = 32;

	/**
	 * @tags(arena, arenaSize=e2::maxNumSkyProxies)
	 */
	class SkyProxy : public e2::MaterialProxy
	{
		ObjectDeclaration()
	public:
		SkyProxy(e2::Session* inSession, e2::MaterialPtr materialAsset);
		virtual ~SkyProxy();

		virtual void bind(e2::ICommandBuffer* buffer, uint8_t frameIndex, bool shadows) override;
		virtual void invalidate(uint8_t frameIndex) override;

		e2::SkyModel* model{};
	};

	/** @tags(dynamic, arena, arenaSize=1) */
	class SkyModel : public e2::ShaderModel
	{
		ObjectDeclaration()
	public:
		SkyModel();
		virtual ~SkyModel();
		virtual void postConstruct(e2::Context* ctx) override;

		virtual e2::MaterialProxy* createMaterialProxy(e2::Session* session, e2::MaterialPtr material) override;

		virtual e2::IPipelineLayout* getOrCreatePipelineLayout(e2::MeshProxy* proxy, uint8_t lodIndex, uint8_t submeshIndex, bool shadows) override;
		virtual e2::IPipeline* getOrCreatePipeline(e2::MeshProxy* proxy, uint8_t lodIndex, uint8_t submeshIndex, e2::RendererFlags rendererFlags) override;

		virtual void invalidatePipelines() override;

		virtual e2::RenderLayer renderLayer() override;


		virtual bool supportsShadows() override;

	protected:
		friend e2::DemoProxy;

		// Pipeline layout for creating pipelines 
		e2::IPipelineLayout* m_pipelineLayout{};

		e2::StackVector<e2::SkyCacheEntry, uint16_t(e2::SkyFlags::Count)> m_pipelineCache;
		bool m_shadersReadFromDisk{};
		bool m_shadersOnDiskOK{};
		std::string m_vertexSource;
		std::string m_fragmentSource;
	};


}






EnumFlagsDeclaration(e2::SkyFlags)











#include "demo.generated.hpp"