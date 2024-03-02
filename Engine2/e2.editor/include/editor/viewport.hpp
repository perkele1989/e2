
#pragma once 

#include <e2/export.hpp>
#include <editor/editorcontext.hpp>
#include <e2/renderer/camera.hpp>

#include <string>

namespace e2
{
	class UIContext;
	class Session;
	class Renderer;
	class UIWidgetState;

	class Viewport : public EditorContext
	{
	public:
		Viewport(e2::Editor* ed, e2::Session* session);
		virtual ~Viewport();

		/** Draw IMGUI etc. */
		virtual void update(e2::UIContext* ui, double seconds);

		virtual e2::Editor* editor() override;

		virtual e2::Engine* engine() override;

		virtual void updateRenderView(e2::UIContext* ui, e2::UIWidgetState* widget, double deltaSeconds, e2::RenderView& outView);


	protected:

		e2::Session* m_session{};
		e2::Renderer* m_renderer{};

		e2::Editor* m_editor{};
	};

	/** A viewport that implements orbital camera controls */
	class OrbitalViewport : public e2::Viewport
	{
	public:
		OrbitalViewport(e2::Editor* ed, e2::Session* session);
		virtual ~OrbitalViewport();

		virtual void updateRenderView(e2::UIContext* ui, e2::UIWidgetState* widget, double deltaSeconds, e2::RenderView& outView) override;

	protected:

		bool m_orbiting{};
		bool m_offsetting{};

		glm::vec3 m_origin{};

		float m_pitch{};
		float m_yaw{};
		float m_zoom{};
	};

}