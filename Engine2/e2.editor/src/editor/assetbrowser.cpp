
#include "editor/assetbrowser.hpp"

#include "editor/editor.hpp"

#include "e2/ui/uicontext.hpp"

#include "editor/importers/ufbximporter.hpp"
#include "editor/importers/meshimporter.hpp"
#include "editor/importers/textureimporter.hpp"
#include "editor/importers/materialimporter.hpp"

#include <filesystem>

//#include "imgui.h"
//#include "imgui_ext.h"
//#include "misc/cpp/imgui_stdlib.h"

e2::AssetBrowser::AssetBrowser(e2::Editor* ed)
	: m_editor(ed)
{
	m_path = assetManager()->database().rootEditorEntry();
}

e2::AssetBrowser::~AssetBrowser()
{

}

void e2::AssetBrowser::update(e2::UIContext* ui, double seconds)
{
	static e2::Name id_assbro = "assbro";
	static e2::Name id_vert = "vert";
	static e2::Name id_entries = "entries";
	static e2::Name id_lastEntry = "lastEntry";
	static e2::Name id_browserContent = "browserContent";
	static e2::Name id_dotdot = "dotdotentry";

	e2::UIStyle& style = uiManager()->workingStyle();

	e2::UIMouseState& mouse = ui->mouseState();

	//e2::UIWidgetState* widget = ui->reserve(id_assbro, {});

	//if(widget->hovered)
		for (std::string file : mouse.drops)
		{
			std::string ext = e2::toLower(std::filesystem::path(file).extension().string());

			if (ext == ".mesh" || ext == ".fbx" || ext == ".lods" || ext == ".dae")
			{
				e2::UfbxImportConfig cfg;
				cfg.input = file;
				cfg.outputDirectory = std::format(".{}", m_path->fullPath());
				e2::UfbxImporter* newImporter = e2::create<e2::UfbxImporter>(editor(), cfg);
				editor()->oneShotImporter(newImporter);
			}
			else if (ext == ".mips" || ext == ".png" || ext == ".tga" || ext == ".jpg" || ext == ".jpeg" || ext == ".hdr")
			{

				e2::TextureImportConfig cfg;
				cfg.input = file;
				cfg.outputDirectory = std::format(".{}", m_path->fullPath());
				e2::TextureImporter *newImporter = e2::create<e2::TextureImporter>(editor(), cfg);
				if (!newImporter->writeAssets())
				{
					LogError("Failed to import file: {} to location {}", cfg.input, cfg.outputDirectory);
				}
				editor()->oneShotImporter(newImporter);
			}
			else if (ext == ".material")
			{
				e2::MaterialImportConfig cfg;
				cfg.input = file;
				cfg.outputDirectory = std::format(".{}", m_path->fullPath());
				e2::MaterialImporter* newImporter = e2::create<e2::MaterialImporter>(editor(), cfg);
				if (!newImporter->writeAssets())
				{
					LogError("Failed to import file: {} to location {}", cfg.input, cfg.outputDirectory);
				}
				editor()->oneShotImporter(newImporter);
			}
		}







	// --- @todo cache this
	std::vector<e2::AssetEditorEntry*> entries;
	e2::AssetEditorEntry* curr = m_path;
	while (curr)
	{
		entries.push_back(curr);
		curr = curr->parent;
	}
	std::reverse(entries.begin(), entries.end());
	// ---


	float vertSizes[] = {24.0f * style.scale, 0.0f};
	ui->beginFlexV(id_vert, vertSizes, 2);

	ui->beginStackH(id_entries, 24.0f * style.scale);

	uint32_t i = 0;
	for (e2::AssetEditorEntry* e : entries)
	{

		if (i == entries.size() - 1)
		{
			ui->label(id_lastEntry, std::format("^s{}", e->name), 12, UITextAlign::Begin);
		}
		else
		{
			if (ui->button(std::format("entry{}", i), e->name))
			{
				m_path = e;
			}
		}

		i++;
	}

	ui->endStackH();




	ui->beginWrap(id_browserContent);
	
	if (m_path->parent)
	{
		if (ui->button(id_dotdot, ".."))
		{
			m_path = m_path->parent;
		}
	}

	i = 0;
	for (e2::AssetEditorEntry* entry : m_path->children)
	{
		
		if (ui->button(std::format("file_entry{}", i), entry->name))
		{
			if (entry->isFolder())
			{
				m_path = entry;
				break;
			}
		}

		i++;
	}

	ui->endWrap();

	ui->endFlexV();

}

e2::Editor* e2::AssetBrowser::editor()
{
	return m_editor;
}

e2::Engine* e2::AssetBrowser::engine()
{
	return m_editor->engine();
}
