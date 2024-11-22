
#pragma once 

#include <e2/export.hpp>
#include <editor/editorcontext.hpp>

#include <e2/utils.hpp>
#include <string>
#include <vector>

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
		std::vector<e2::Name> m_path;
	};

}