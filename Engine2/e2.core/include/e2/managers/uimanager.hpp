
#pragma once 

#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/manager.hpp>
#include <e2/managers/rendermanager.hpp>
#include <e2/input/inputtypes.hpp>

#include <e2/rhi/shader.hpp>
#include <e2/rhi/texture.hpp>
#include <e2/rhi/threadcontext.hpp>
#include <e2/rhi/rendercontext.hpp>
#include <e2/rhi/pipeline.hpp>
#include <e2/ui/uitypes.hpp>

#include <e2/assets/spritesheet.hpp>

#include <unordered_set>
#include <unordered_map>

namespace e2
{
	class Engine;
	class UIWindow;
	class UIContext;

	struct E2_API UIPipeline
	{
		e2::IShader* vertexShader{};
		e2::IShader* fragmentShader{};
		e2::IPipelineLayout* layout{};
		e2::IPipeline* pipeline{};
	};

	struct E2_API UIQuadPushConstants
	{
		glm::vec4 quadColor;
		glm::vec2 surfaceSize;
		glm::vec2 quadPosition;
		glm::vec2 quadSize;
		float quadZ;
	};

	struct E2_API UITexturedQuadPushConstants
	{
		glm::vec4 quadColor;
		glm::vec2 surfaceSize;
		glm::vec2 quadPosition;
		glm::vec2 quadSize;
		glm::vec2 uvOffset;
		glm::vec2 uvSize;
		float quadZ;
		uint32_t textureIndex;
		uint32_t type;
	};

	struct E2_API UIFancyQuadPushConstants
	{
		glm::vec4 quadColor;
		glm::vec2 surfaceSize;
		glm::vec2 quadPosition;
		glm::vec2 quadSize;
		float quadZ;
		float cornerRadius;
		float bevelStrength;
		int32_t type;
		float pixelScale;
	};

	struct E2_API UIQuadShadowPushConstants
	{
		glm::vec4 quadColor;
		glm::vec2 surfaceSize;
		glm::vec2 quadPosition;
		glm::vec2 quadSize;
		float quadZ;
		float cornerRadius;
		float shadowStrength;
		float shadowSize;
	};

	/** @todo move to buildcfg */
	constexpr uint32_t maxConcurrentUITextures = 128;

	enum class UIResizerType
	{
		None,
		NW,
		N,
		NE,
		E,
		SE,
		S,
		SW,
		W
	};

	class E2_API UIManager : public Manager, public e2::RenderCallbacks
	{
	public:
		UIManager(Engine* owner);
		virtual ~UIManager();

		virtual void onNewFrame(uint8_t frameIndex) override;

		virtual void preDispatch(uint8_t frameIndex) override;
		virtual void postDispatch(uint8_t frameIndex) override;


		virtual void initialize() override;
		virtual void shutdown() override;

		virtual void preUpdate(double deltaTime) override;
		virtual void update(double deltaTime) override;

		inline e2::UIStyle& workingStyle()
		{
			return m_workingStyle;
		}

		/** Only available for the current frame */
		uint32_t idFromTexture(e2::ITexture* tex);

		void queueDestroy(e2::UIWindow* wnd);

		e2::IDataBuffer* quadVertexBuffer{};
		e2::IDataBuffer* quadIndexBuffer{};
		e2::IVertexLayout* quadVertexLayout{};

		UIPipeline quadPipeline;


		void registerGlobalSpritesheet(e2::Name id, e2::SpritesheetPtr sheet);
		void unregisterGlobalSpritesheet(e2::Name id);

		// fullId = "x.y" where x = spritesheet id, and y = sprite id
		e2::Sprite* globalSprite(e2::Name fullId);

	protected:

		friend e2::UIWindow;
		friend e2::UIContext;


		std::unordered_map<e2::Name, e2::SpritesheetPtr> m_globalSheets;
		std::unordered_map<e2::Name, e2::Sprite*> m_globalSprites;

		e2::UIStyle m_workingStyle;

		std::unordered_set<e2::UIWindow*> m_newQueue;
		std::unordered_set<e2::UIWindow*> m_removeQueue;

		std::unordered_set<e2::UIWindow*> m_windows;
		std::unordered_set<e2::UIContext*> m_contexts;

		// Global, static vertex buffers for a quad between 0,0 and 1,1

		UIPipeline m_texturedQuadPipeline;
		UIPipeline m_fancyQuadPipeline;
		UIPipeline m_quadShadowPipeline;
		UIPipeline m_textPipeline;

		e2::ISampler* m_texturedQuadSampler{};
		e2::IDescriptorSetLayout* m_texturedQuadSetLayout{};
		e2::IDescriptorPool* m_texturedQuadPool{};
		e2::Pair<e2::IDescriptorSet*> m_texturedQuadSets;

		//e2::StackVector<e2::ITexture*, e2::maxConcurrentUITextures> m_frameTextures;
		uint32_t m_frameTextureIndex = 0;
		std::unordered_map<e2::ITexture*, uint32_t> m_frameTextureIndices;
	};



	enum UIWindowFlags : uint8_t
	{
		WF_Resizable = 0b0000'0001,
		WF_Closeable = 0b0000'0010,
		WF_SystemMenu = 0b0000'0100,
		WF_Minimizeable = 0b0000'1000,
		WF_Titlebar = 0b0001'0000,
		WF_Transient = 0b0010'0000, // smaller frame + auto destroys when focus is lost
		WF_TopMost = 0b0100'0000,
		WF_ScaleToFit = 0b1000'0000,

		/** Default capable window */
		WF_Default = WF_Resizable | WF_Closeable | WF_SystemMenu | WF_Minimizeable | WF_Titlebar,

		WF_Dialog = WF_Resizable | WF_Closeable | WF_Minimizeable | WF_Titlebar,

		WF_PopupMenu = WF_Transient | WF_TopMost | WF_ScaleToFit,

	};

	/**
	 * A high-level window that wraps a RHI IWindow, a UI context, as well as
	 * functionality to render a custom window border, moving, closing etc.
	 * Exclusively to be used for tools development, as these facilities are not optimized for game builds
	 * For ingame UI please look elsewhere, you filthy hobo
	 */


	class E2_API UIWindow : public virtual e2::Context
	{
	public:
		UIWindow(e2::Context* ctx, UIWindowFlags flags = WF_Default);
		virtual ~UIWindow();

		virtual e2::Engine* engine() override;

		virtual void update(double deltaTime);
		virtual void onSystemButton();

		void cursor(e2::CursorShape newCursor);

		e2::IWindow* rhiWindow();

		e2::UIContext* uiContext();

		virtual void preSubmit();

		void destroy();

	protected:


		void windowResizer(e2::UIResizerType type, glm::vec2 position, glm::vec2 size);


		e2::Engine* m_engine{};
		e2::IWindow* m_rhiWindow{};
		e2::UIContext* m_uiContext{};

		UIWindowFlags m_flags{};

		bool m_hasChangedCursor{};

		glm::vec2 m_minimumSize;

		//
		bool m_wantsMove{};
		bool m_moving{};
		//glm::vec2 m_moveOrigin;
		
		bool m_wantsSize{};
		bool m_sizing{};
		UIResizerType m_sizingType{ UIResizerType::None};
		//glm::vec2 m_sizeOrigin;
	};

}
