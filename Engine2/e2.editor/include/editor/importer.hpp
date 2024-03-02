
#pragma once 

#include <e2/export.hpp>
#include <editor/editorcontext.hpp>
#include <e2/managers/uimanager.hpp>

#include <string>

namespace e2
{
	class Editor;
	class Importer : public e2::UIWindow, public e2::EditorContext
	{
	public:
		Importer(e2::Editor* editor);
		virtual ~Importer();

		inline virtual e2::Editor* editor() override
		{
			return m_editor;
		}

		virtual void update(double seconds) override;

		virtual e2::Engine* engine() override;

	protected:
		e2::Editor* m_editor;
	};

}