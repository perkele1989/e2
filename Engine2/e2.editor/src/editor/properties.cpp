
#include "editor/properties.hpp"

#include "editor/editor.hpp"

#include "e2/ui/uicontext.hpp"

e2::PropertiesPanel::PropertiesPanel(e2::Editor* ed)
	: m_editor(ed)
{

}

e2::PropertiesPanel::~PropertiesPanel()
{

}

void e2::PropertiesPanel::update(e2::UIContext* ui, double seconds)
{
	ui->label("properties", "[Properties]", 12, UITextAlign::Middle);

}

e2::Editor* e2::PropertiesPanel::editor()
{
	return m_editor;
}

e2::Engine* e2::PropertiesPanel::engine()
{
	return m_editor->engine();
}
