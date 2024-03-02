
#pragma once 

#include <e2/game/session.hpp>
#include <e2/renderer/renderer.hpp>
#include <e2/managers/rendermanager.hpp>
#include <e2/ui/uicontext.hpp>

namespace e2
{

	class Renderer;
	class IWindow;
	class GameSession : public e2::Session, public e2::RenderCallbacks
	{
	public:
		GameSession(e2::Context* ctx);
		virtual ~GameSession();

		virtual void tick(double seconds) override;

		virtual void onNewFrame(uint8_t frameIndex) override;
		virtual void preDispatch(uint8_t frameIndex) override;
		virtual void postDispatch(uint8_t frameIndex) override;

		inline e2::IWindow* window()
		{
			return m_window;
		}

		inline e2::UIContext* uiContext()
		{
			return m_uiContext;
		}

		inline e2::Renderer* renderer()
		{
			return m_renderer;
		}

	protected:
		e2::IWindow *m_window{};
		e2::UIContext* m_uiContext{};
		e2::Renderer* m_renderer{};

	};
}