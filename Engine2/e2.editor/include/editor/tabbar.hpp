
#pragma once 

#include <e2/export.hpp>
#include <editor/editorcontext.hpp>

#include <string>

namespace e2
{
	class EditorWindow;

	class Tabbar : public EditorContext
	{
	public:
		Tabbar(e2::EditorWindow* editorWindow);
		virtual ~Tabbar();

		/** Draw IMGUI etc. */
		virtual void update(double seconds);

		virtual e2::Editor* editor() override;

		virtual e2::Engine* engine() override;


	protected:


		e2::EditorWindow* m_editorWindow{};
	};

}