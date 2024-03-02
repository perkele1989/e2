
#include "editor/importer.hpp"
#include "editor/editor.hpp"

e2::Importer::Importer(e2::Editor *editor) 
	: e2::UIWindow(editor, WF_Dialog)
	, m_editor(editor)
{

}
e2::Importer::~Importer()
{

}

void e2::Importer::update(double seconds)
{
	e2::UIWindow::update(seconds);

	if (rhiWindow()->wantsClose())
	{
		editor()->destroyImporter(this);
	}

}

e2::Engine* e2::Importer::engine()
{
	return editor()->engine();
}
