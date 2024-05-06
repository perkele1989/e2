
#pragma once

#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/utils.hpp>
#include <e2/context.hpp>

#include <e2/assets/font.hpp>
#include <e2/assets/spritesheet.hpp>

#include <e2/rhi/rendertarget.hpp>
#include <e2/rhi/texture.hpp>
#include <e2/rhi/threadcontext.hpp>

#include <e2/timer.hpp>
#include <e2/ui/uitypes.hpp>

namespace e2
{
	constexpr uint32_t maxNumUIWidgetData = 4096;
	constexpr uint32_t uiRetainFrameCount = 4;

	struct E2_API UIWidgetState
	{
		size_t id;
		uint32_t framesSinceActive{0};
		glm::vec2 position;
		glm::vec2 size;
		bool hovered{};
		bool active{};

		float dragOrigin{};
	};

	constexpr uint32_t uiIdStackSize = 32;


	struct E2_API UIMouseButtonState
	{
		// The buffered state. You probably dont want to use this.
		bool bufferedState{};

		// The current state of this button (down if true)
		bool state{};

		// The previous state of this button, last frame
		bool lastState{};

		// derivatives below 

		bool ignorePress{};

		// was pressed this frame  (state && !lastState)
		bool pressed{}; 

		// was released this frame ( !state && lastState )
		bool released{};

		// was held this frame (state && lastState)
		bool held{};

		// was clicked this frame (same as released, do we need this?)
		bool clicked{};

		// was doubleClicked this frame (pressed && timeSinceLastPressed < e2::doubleClickTreshold)
		bool doubleClicked{};

		/** used to keep track of doubleclicks. This will be updated with the state to now, so its not very useful for userspace */
		e2::Moment timeLastPressed;

		/** Client-relative mouse position when this button was last pressed, used for drags etc */
		glm::vec2 pressedPosition;

		/** The current drag offset*/
		glm::vec2 dragOffset;

		/** The size of the dragoffset */
		float dragDistance{};

		/** The maximum drag distance so far for this key, or 0.0 if not pressed */
		float maxDragDistance{};
	};



	struct E2_API UIMouseState
	{
		bool buttonState(e2::MouseButton btn);
		bool buttonPressed(e2::MouseButton btn);
		bool buttonReleased(e2::MouseButton btn);
		bool buttonDoubleClick(e2::MouseButton btn);

		glm::vec2 bufferedPosition;

		glm::vec2 position;
		glm::vec2 lastPosition;
		glm::vec2 moveDelta;

		glm::vec2 relativePosition;

		float bufferedScroll{};
		float scrollOffset{};

		bool moved{};
		std::array<e2::UIMouseButtonState, size_t(e2::MouseButton::Count)> buttons;


		std::vector<std::string> bufferedDrops;
		std::vector<std::string> drops;
	};

	struct E2_API UIKeyState
	{
		// The buffered state. You probably dont want to use this.
		bool bufferedState{};

		// The current state of this button (down if true)
		bool state{};

		// The previous state of this button, last frame
		bool lastState{};

		// derivatives below 

		// was pressed this frame  (state && !lastState)
		bool pressed{};

		// was released this frame ( !state && lastState )
		bool released{};

	};

	struct E2_API UIKeyboardState
	{
		bool state(e2::Key k);
		bool pressed(e2::Key k);
		bool released(e2::Key k);

		std::array<UIKeyState, size_t(e2::Key::Count)> keys;
	};



	class UIWindow;

	struct UIRenderState;
	using Func_getSizeForNext = void(*)(e2::UIRenderState &state, glm::vec2 const& minimumSize, glm::vec2 &outOffset, glm::vec2 &outSize);

	// Render states 
	struct E2_API UIRenderState {
		e2::Name id;

		// client area
		glm::vec2 offset;

		// size, 
		glm::vec2 size;

		// where we are currently rendering 
		glm::vec2 cursor;

		e2::Func_getSizeForNext getSizeForNext{};


		// FLEX 
		// The current row/column index
		uint32_t flexIterator = 0;
		e2::StackVector<float, 16> flexSizes;


		// WRAP
		float maxParallelSize{};
		uint32_t indentLevels{ 0 };
	};

	class E2_API UIContext : public e2::Context, public e2::IWindowInputHandler
	{
	public:
		UIContext(e2::Context* ctx, e2::IWindow* win);
		virtual ~UIContext();

		virtual e2::Engine* engine() override;

		virtual void onKeyDown(Key key) override;
		virtual void onKeyUp(Key key)override;

		virtual void onMouseScroll(float scrollOffset)override;
		virtual void onMouseMove(glm::vec2 position) override;
		virtual void onMouseDown(e2::MouseButton button) override;
		virtual void onMouseUp(e2::MouseButton button) override;

		virtual void onDrop(int32_t pathCount, char const* paths[]) override;

		void newFrame();
		void submitFrame();

		void setClientArea(glm::vec2 const& offset, glm::vec2 const& size);

		/** As long as this is called at least every e2::uiRetainFrameCount frames, the widget data will remain untouched */
		e2::UIWidgetState* reserve(e2::Name id, glm::vec2 const& minSize);

		// --- Begin Containers --- //

		UIRenderState& pushRenderState(e2::Name id, glm::vec2 const& minSize, glm::vec2 offset = {});
		void popRenderState();

		void pushFixedPanel(e2::Name id, glm::vec2 const& position, glm::vec2 const& size);
		void popFixedPanel();

		void beginFlexV(e2::Name id, float const *rowSizes, uint32_t rowSizeCount);
		void endFlexV();

		void flexVSlider(e2::Name id, float& value, float scale = 1.0f);

		void beginFlexH(e2::Name id, float const * rowSizes, uint32_t rowSizeCount);
		void endFlexH();

		void flexHSlider(e2::Name id, float& value, float scale = 1.0f);

		void beginStackV(e2::Name id, glm::vec2 offset = {});
		void endStackV();

		void beginStackH(e2::Name id, float minHeight = 0.0f);
		void endStackH();


		void beginWrap(e2::Name id);
		void endWrap();





		// --- End Containers --- //


		bool gameGridButton(e2::Name id, e2::Name iconSprite, std::string const& hoverTextMain, bool active);

		void gameLabel(std::string const& text, uint8_t fontSize = 12, e2::UITextAlign horizAlign = UITextAlign::Begin);


		// --- Begin Widgets --- //
		void log(e2::Name id, float& scrollOffset, bool autoscroll);

		void label(e2::Name id, std::string const& text, uint8_t fontSize = 12, e2::UITextAlign horizAlign = UITextAlign::Begin);
		bool button(e2::Name id, std::string const& title);
		void sprite(e2::Sprite const &spr, e2::UIColor const& col, float scale);
		bool checkbox(e2::Name id, bool& value, std::string const& text);

		/** currentIndex is the index of this button, value is the current value of the group */
		bool radio(e2::Name id, uint32_t currentIndex, uint32_t& groupValue, std::string const& text);

		bool inputText(e2::Name id, std::string& buffer);
		bool sliderInt(e2::Name id, int32_t& value, int32_t min, int32_t max, char const* format = "%d");
		bool sliderFloat(e2::Name id, float& value, float min, float max, char const* format = "%.3f");

		// --- End Widgets --- //

		// --- Begin Rendering --- //

		void clearScissor();

		void setScissor(glm::vec2 position, glm::vec2 size);

		/** Draws a quad */
		void drawQuad(glm::vec2 position, glm::vec2 size, e2::UIColor color);

		void drawTexturedQuad(glm::vec2 position, glm::vec2 size, e2::UIColor color, e2::ITexture* texture, glm::vec2 uvOffset = { 0.0f, 0.0f }, glm::vec2 uvScale = {1.0f, 1.0f}, e2::UITexturedQuadType type = UITexturedQuadType::Default);

		/** Draws a sprite */
		void drawSprite(glm::vec2 position, e2::Sprite sprite, e2::UIColor color, float scale);

		/** Draws a fancy quad with corner radius and thin bevel */
		void drawQuadFancy(glm::vec2 position, glm::vec2 size, e2::UIColor color, float cornerRadius, float bevelStrength, bool windowBorder = false);

		/** Draws the shadow for a fancy quad */
		void drawQuadShadow(glm::vec2 position, glm::vec2 size, float cornerRadius, float shadowStrength, float shadowSize);

		/** 
		 * Draw single line of text with custom "markdown" support.
		 * Normal Text 
		 * *Italic Text*
		 * **Bold Text**
		 * ***Italic Bold Text***
		 * ^sThis Text Is Sans Serif
		 * ^fThis Text Is Serif
		 * ^mThis Text Is Monospace
		 * ^3This Text Is Colored (use 0 for fg, 1 for bg, 2-9 for accents)
		 * ^-This Text Is No Longer Colored
		 */
		void drawRasterText(e2::FontFace fontFace, uint8_t fontSize,  e2::UIColor color, glm::vec2 position, std::string const& markdownUtf8, bool enableColorChange = true, bool soft = false);
		void drawSDFText(e2::FontFace fontFace, float fontSize, e2::UIColor color, glm::vec2 position, std::string const& markdownUtf8, bool enableColorChange = true, bool soft = false);
		void drawSDFTextCarousel(e2::FontFace fontFace, float fontSize, e2::UIColor color, glm::vec2 position, std::string const& markdownUtf8, float heightScale, float time);
		void drawRasterTextShadow(e2::FontFace fontFace, uint8_t fontSize,glm::vec2 position, std::string const& markdownUtf8);

		float calculateSDFTextWidth(e2::FontFace fontFace, float fontSize, std::string const& markdownUtf8);

		/** @todo build renderstate here for drawRasterText to cut CPU time for rendering text in almost half  */
		float calculateTextWidth(e2::FontFace fontFace, uint8_t fontSize, std::string const& markdownUtf8);

		// --- End Rendering --- //

		glm::uvec2 const& size();
		void resize(glm::uvec2 newSize);

		e2::ITexture* colorTexture();

		/** Non const since we can flag things in the mouse state (like ignorepress) */
		e2::UIMouseState & mouseState();

		e2::UIKeyboardState& keyboardState();

		UIRenderState& renderState();
		UIRenderState& baseState();

		glm::vec2 const& renderedSize();
		glm::vec2 renderedPadding();

		void unsetActive();

		void pushId(e2::Name name);
		void popId();

	protected:
		e2::Engine* m_engine{};
		e2::IWindow* m_window{};





		e2::StackVector<UIRenderState, 32> m_renderStates;

		glm::vec2 m_renderedSize;

		size_t getCombinedHash(e2::Name id);

		e2::StackVector<e2::Name, uiIdStackSize> m_idStack;

		std::unordered_map<size_t, e2::UIWidgetState*> m_dataMap;
		e2::Arena<UIWidgetState> m_dataArena;
		std::vector<size_t> m_clearList;

		size_t m_activeId;

		// just do devicewaitidle when we need to resize our render target. The slight possible hitch beats memory overhead of doublebuffered frameubffer attachments 
		bool m_hasRecordedData{ false };
		bool m_inFrame{ false };
		glm::uvec2 m_renderTargetSize{};
		e2::IRenderTarget* m_renderTarget{};
		e2::ITexture* m_colorTexture{};
		e2::ITexture* m_depthTexture{}; // Yes, we use depth for UI, as it allows us to make optimizations like rendering front-to-back for opaque and back-to-front for transparent surfaces, and save a bunch of shading.
		e2::Pair<e2::ICommandBuffer*> m_commandBuffers {nullptr};
		e2::PipelineSettings m_pipelineSettings;


		// 
		float m_currentZ = 1.0f;

		e2::UIMouseState m_mouseState;
		e2::UIKeyboardState m_keyboardState;

		/** This is read by any window that owns this context, to move the actual window */
		glm::vec2 m_windowMoveDelta;
	};
}

#include "uicontext.generated.hpp"