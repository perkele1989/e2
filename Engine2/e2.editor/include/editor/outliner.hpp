
#pragma once 

#include <e2/export.hpp>
#include <editor/editorcontext.hpp>

#include <string>

namespace e2
{
	class UIContext;

	class Outliner : public EditorContext
	{
	public:
		Outliner(e2::Editor* ed);
		virtual ~Outliner();

		/** Draw IMGUI etc. */
		virtual void update(e2::UIContext* ui, double seconds);

		virtual e2::Editor* editor() override;

		virtual e2::Engine* engine() override;


	protected:


		e2::Editor* m_editor{};
	};

}