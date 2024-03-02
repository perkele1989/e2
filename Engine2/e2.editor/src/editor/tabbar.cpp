
#include "editor/tabbar.hpp"

#include "editor/editor.hpp"

#include "e2/ui/uicontext.hpp"

e2::Tabbar::Tabbar(e2::EditorWindow* wnd)
	: m_editorWindow(wnd)
{

}

e2::Tabbar::~Tabbar()
{

}

void e2::Tabbar::update(double seconds)
{
	e2::UIContext* ui = m_editorWindow->uiContext();
	e2::UIStyle& style = uiManager()->workingStyle();

	const e2::Name id_stack = "stackH";
	ui->beginStackH(id_stack);

		if (ui->button("IdPoop", "+"))
		{
			style.scale += 0.1f;
		}
		
		if (ui->button("IdPoop2", "-"))
		{
			style.scale -= 0.1f;
		}

		ui->label("IdPoop3", std::format("**Current Scale:** {}", style.scale));

		uint32_t i = 0;
		for (e2::Workspace* wrkspc : m_editorWindow->workspaces())
		{
			if (ui->button(std::format("id{}", i++), wrkspc->displayName()))
			{
				m_editorWindow->setActiveWorkspace(wrkspc);
			}
		}


	ui->endStackH();


}

e2::Editor* e2::Tabbar::editor()
{
	return m_editorWindow->editor();
}

e2::Engine* e2::Tabbar::engine()
{
	return m_editorWindow->engine();
}
