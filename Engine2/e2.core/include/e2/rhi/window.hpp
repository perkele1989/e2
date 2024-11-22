
#pragma once 

#include <e2/export.hpp>
#include <e2/rhi/renderresource.hpp>
#include <e2/rhi/texture.hpp>
#include <e2/input/inputtypes.hpp>

namespace e2
{

	enum class WindowMode : uint8_t
	{
		Windowed,
		Fullscreen,
		Borderless
	};

	struct E2_API WindowCreateInfo
	{
		std::string title;
		glm::uvec2 size{1280, 720};
		glm::uvec2 position{0, 0};
		bool resizable{};
		uint32_t monitorIndex{0};
		e2::WindowMode mode{ e2::WindowMode::Windowed };
		bool customDecorations{};
		bool topMost{};
		bool tool{};
	};

	enum class CursorShape : uint8_t
	{
		Default = 0,
		Text,
		Link,
		Crosshair,
		ResizeWE,
		ResizeNS,
		ResizeNWSE,
		ResizeNESW,
		Count
	};

	/** A handler for input from a window. Designed for application/tools development. */
	class E2_API IWindowInputHandler
	{
	public:
		virtual void onKeyDown(Key key)=0;
		virtual void onKeyUp(Key key) = 0;

		virtual void onMouseScroll(float scrollOffset) = 0;
		virtual void onMouseMove(glm::vec2 position)=0;
		virtual void onMouseDown(MouseButton button)=0;
		virtual void onMouseUp(MouseButton button)=0;
		virtual void onDrop(int32_t pathCount, char const* paths[]) = 0;
	};

	class E2_API IWindow : public e2::ThreadResource
	{
		ObjectDeclaration()
	public:
		IWindow(IThreadContext* context, e2::WindowCreateInfo const& createInfo);
		virtual ~IWindow();

		virtual void title(std::string const& newTitle) = 0;

		virtual void source(ITexture* newSource) = 0;

		virtual bool wantsClose() const = 0;
		virtual void wantsClose(bool newValue) = 0;

		virtual void present() = 0;

		virtual bool isFullscreen() = 0;
		virtual void setFullscreen(bool newFullscreen) = 0;

		virtual glm::vec2 position() = 0;
		virtual void position(glm::vec2 const& newPosition) = 0;

		virtual glm::uvec2 size() = 0;
		virtual void size(glm::uvec2 const& newSize) = 0;

		virtual void registerInputHandler(e2::IWindowInputHandler* handler) = 0;
		virtual void unregisterInputHandler(e2::IWindowInputHandler* handler) = 0;

		virtual void cursor(e2::CursorShape newCursor) = 0;

		virtual void showCursor(bool show) = 0;

		virtual void maximize() = 0;
		virtual void restore() = 0;
		virtual void minimize() = 0;
		virtual bool isMaximized() = 0;

		virtual bool isFocused() = 0;
		
		virtual void mouseLock(bool newMouseLock) = 0;

		/*

		virtual std::string title() const = 0;
		virtual void title(std::string const& newTitle) = 0;

		virtual e2::WindowMode mode() const = 0;
		virtual void mode(e2::WindowMode newMode) = 0;

		virtual glm::uvec2 size() const = 0;
		virtual void size(glm::uvec2) = 0;

		virtual bool maximized() const = 0;
		virtual void maximized(bool newMaximized) = 0;

		virtual bool minimized() const = 0;
		virtual void minimized(bool newMinimized) = 0;

		virtual bool visible() const = 0;
		virtual void visible(bool newVisible) = 0;*/


	};
}

#include "window.generated.hpp"