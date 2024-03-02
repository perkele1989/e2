
#pragma once 

#include <e2/export.hpp>
#include <editor/editorcontext.hpp>

#include <string>

namespace e2
{
	class UIContext;
	struct AssetEditorEntry;

	class AssetBrowser : public EditorContext
	{
	public:
		AssetBrowser(e2::Editor* ed);
		virtual ~AssetBrowser();

		/** Draw IMGUI etc. */
		virtual void update(e2::UIContext* ui, double seconds);

		virtual e2::Editor* editor() override;

		virtual e2::Engine* engine() override;

		void refresh();

	protected:
		e2::Editor* m_editor{};

		e2::AssetEditorEntry* m_path{};
	};

}