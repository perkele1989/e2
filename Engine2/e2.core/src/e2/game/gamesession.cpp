
#include "e2/game/gamesession.hpp"

#include "e2/e2.hpp"
#include "e2/managers/rendermanager.hpp"

#include <e2/rhi/window.hpp>
#include "e2/utils.hpp"

e2::GameSession::GameSession(e2::Context* ctx)
	: e2::Session(ctx)
{
	e2::WindowCreateInfo winCreateInfo{};
	winCreateInfo.title = "Reveal & Annihilate";
	winCreateInfo.resizable = true;
	//winCreateInfo.mode = WindowMode::Fullscreen;
	m_window = renderManager()->mainThreadContext()->createWindow(winCreateInfo);
	m_window->setFullscreen(true);

	m_renderer = e2::create<e2::Renderer>(this, winCreateInfo.size);

	//m_window->source(m_renderer->colorTarget());


	m_uiContext = e2::create<e2::UIContext>(this, m_window);
	m_window->registerInputHandler(m_uiContext);
	m_window->source(m_uiContext->colorTexture());

	renderManager()->registerCallbacks(this);

	//e2::WindowCreateArgs args{};
	//m_mainWindow = renderContext()->createWindow(args);
}

e2::GameSession::~GameSession()
{
	renderManager()->unregisterCallbacks(this);

	m_window->unregisterInputHandler(m_uiContext);
	e2::destroy(m_uiContext);

	e2::destroy(m_renderer);
	e2::destroy(m_window);
}

void e2::GameSession::preTick(double seconds)
{

}

void e2::GameSession::tick(double seconds)
{
	e2::Session::tick(seconds);

	if (m_window->wantsClose())
	{
		engine()->shutdown();
	}

	if (m_window->size() != m_renderer->resolution() && m_window->size().x > 0 && m_window->size().y > 0)
	{
		m_renderer->keepAround();
		e2::destroy(m_renderer);
		m_renderer = e2::create<e2::Renderer>(this, m_window->size());
	}

	uiContext()->setClientArea({}, m_window->size());

	m_renderer->recordFrame(seconds);
	m_uiContext->drawTexturedQuad({}, m_uiContext->size(), 0xFFFFFFFF, m_renderer->colorTarget());

}

void e2::GameSession::onNewFrame(uint8_t frameIndex)
{

}

void e2::GameSession::preDispatch(uint8_t frameIndex)
{

}

void e2::GameSession::postDispatch(uint8_t frameIndex)
{
	m_window->present();
}
