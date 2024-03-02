
#include "editor/outliner.hpp"

#include "editor/editor.hpp"

#include "e2/ui/uicontext.hpp"

e2::Outliner::Outliner(e2::Editor* ed)
	: m_editor(ed)
{

}

e2::Outliner::~Outliner()
{

}

void e2::Outliner::update(e2::UIContext* ui, double seconds)
{
	ui->label("outliner", "[Outliner]", 12, UITextAlign::Middle);

}

e2::Editor* e2::Outliner::editor()
{
	return m_editor;
}

e2::Engine* e2::Outliner::engine()
{
	return m_editor->engine();
}
