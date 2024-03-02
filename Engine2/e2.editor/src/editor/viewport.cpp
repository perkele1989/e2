
#include "editor/viewport.hpp"

#include "editor/editor.hpp"

#include "e2/ui/uicontext.hpp"

#include "e2/transform.hpp"

e2::Viewport::Viewport(e2::Editor* ed, e2::Session* session)
	: m_editor(ed)
	, m_session(session)
{

}

e2::Viewport::~Viewport()
{
	if (m_renderer)
	{
		e2::destroy(m_renderer);
		m_renderer = nullptr;
	}

}

void e2::Viewport::update(e2::UIContext* ui, double seconds)
{
	// create a widget state that stretches fully
	const e2::Name id = "viewport";
	e2::UIWidgetState* widget = ui->reserve(id, {});
	
	// recreate the renderer if the resolution changed (lets do 2x supersampling for now)
	glm::uvec2 newResolution = glm::max(glm::vec2(1.0f, 1.0f), widget->size * 2.0f);
	if (!m_renderer || m_renderer->resolution() != newResolution)
	{
		if (m_renderer)
		{
			e2::destroy(m_renderer);
			m_renderer = nullptr;
		}

		m_renderer = e2::create<e2::Renderer>(m_session, newResolution);
	}

	// update the renderview
	e2::RenderView newView;
	updateRenderView(ui, widget, seconds, newView);
	m_renderer->setView(newView);
	
	// render a frame
	m_renderer->recordFrame(seconds);

	// draw the rendered frame to the viewport
	ui->drawTexturedQuad(widget->position, widget->size, 0xFFFFFFFF, m_renderer->colorTarget());
}

e2::Editor* e2::Viewport::editor()
{
	return m_editor;
}

e2::Engine* e2::Viewport::engine()
{
	return m_editor->engine();
}

void e2::Viewport::updateRenderView(e2::UIContext* ui, e2::UIWidgetState* widget, double deltaSeconds, e2::RenderView& outView)
{
	// override this to implement camera control  
	outView.fov = 45.0f;
	outView.clipPlane = { 0.01f, 100.0f };
	outView.orientation = { 0.0f, 0.0f, 0.0f, 1.0f };
	outView.origin = { 0.0f, 0.0f, 5.0f };
}

e2::OrbitalViewport::OrbitalViewport(e2::Editor* ed, e2::Session* session)
	: e2::Viewport(ed, session)
{

}

e2::OrbitalViewport::~OrbitalViewport()
{

}

void e2::OrbitalViewport::updateRenderView(e2::UIContext* ui, e2::UIWidgetState* widget, double deltaSeconds, e2::RenderView& outView)
{
	e2::UIMouseState& mouse = ui->mouseState();
	e2::UIMouseButtonState& leftMouse = mouse.buttons[uint8_t(e2::MouseButton::Left)];
	e2::UIMouseButtonState& rightMouse = mouse.buttons[uint8_t(e2::MouseButton::Right)];
	e2::UIMouseButtonState& middleMouse = mouse.buttons[uint8_t(e2::MouseButton::Middle)];
	e2::UIKeyboardState& keyboard = ui->keyboardState();

	bool shift = keyboard.keys[int16_t(e2::Key::LeftShift)].state;

	// Handle zoom inputs
	if (!e2::nearlyEqual(mouse.scrollOffset, 0.0f, 0.001f))
	{
		float sign = glm::sign(mouse.scrollOffset);

		if (sign <= 0.0f)
			m_zoom *= 1.1f;
		else
			m_zoom /= 1.1f;
	}

	if (m_zoom < 0.1f)
		m_zoom = 0.1f;

	if (m_zoom > 200.0f)
		m_zoom = 200.0f;



	// Handle pitch and yaw inputs 
	if (!m_offsetting && !m_orbiting && widget->hovered && middleMouse.pressed && !shift)
	{
		m_orbiting = true;
	}
	else if (m_orbiting && middleMouse.released)
	{
		m_orbiting = false;
	}

	if (m_orbiting)
	{
		m_yaw -= mouse.moveDelta.x * 1.0f;
		m_pitch -= mouse.moveDelta.y * 1.0f;
	}

	if (m_pitch > 90.0f)
		m_pitch = 90.0f;

	if (m_pitch < -90.f)
		m_pitch = -90.0f;

	outView.orientation = glm::identity<glm::quat>();
	outView.orientation = glm::rotate(outView.orientation, glm::radians(-m_yaw), e2::worldUp());
	outView.orientation = glm::rotate(outView.orientation, glm::radians(-m_pitch), e2::worldRight());








	// Handle offset inputs 
	if (!m_orbiting && !m_offsetting && widget->hovered && middleMouse.pressed && shift)
	{
		m_offsetting = true;
	}
	else if (m_offsetting && middleMouse.released)
	{
		m_offsetting = false;
	}

	if (m_offsetting)
	{
		m_origin += outView.orientation * e2::worldRight() * mouse.moveDelta.x * -0.01f;
		m_origin += outView.orientation * e2::worldUp() * mouse.moveDelta.y * 0.01f;
	}

	outView.origin = m_origin + (outView.orientation * -e2::worldForward() * m_zoom);


	outView.fov = 45.0f;
	outView.clipPlane = { 0.01f, 1000.0f };
}
