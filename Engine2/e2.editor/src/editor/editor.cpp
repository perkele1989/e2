
#include "editor/editor.hpp"
#include "editor/assetbrowser.hpp"
#include "editor/logoutput.hpp"
#include "editor/properties.hpp"
#include "editor/viewport.hpp"
#include "editor/tabbar.hpp"
#include "editor/outliner.hpp"
#include "editor/importer.hpp"
#include "editor/importers/meshimporter.hpp"
#include "editor/importers/textureimporter.hpp"
#include "editor/importers/fontimporter.hpp"

#include "e2/game/world.hpp"
#include "e2/game/entity.hpp"
#include "e2/game/meshcomponent.hpp"

#include "e2/ui/uicontext.hpp"

#include "e2/dmesh/dmesh.hpp"
#include "e2/hex/hex.hpp"

#include "e2/e2.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/managers/typemanager.hpp"

#include <filesystem>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


// these will do, piggy
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
// oink oink

//#if !defined(E2_DEVELOPMENT)
//#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
//#endif

e2::Editor::Editor(e2::Context* ctx)
	: e2::Application(ctx)
{

}

e2::Editor::~Editor()
{

}

e2::Editor* e2::Editor::editor()
{
	return this;
}

void e2::Editor::initialize()
{

#define reimportDefaultFonts false
	if (reimportDefaultFonts)
	{
		e2::FontImportConfig config;
		config.inputRegular = "OpenSans-Regular.ttf";
		config.inputBold = "OpenSans-Bold.ttf";
		config.inputItalic = "OpenSans-Italic.ttf";
		config.inputBoldItalic = "OpenSans-BoldItalic.ttf";
		config.outputFilepath = assetManager()->database().cleanPath("./engine/F_DefaultFont_Sans.e2a");
		e2::FontImporter* newImporter = e2::create<e2::FontImporter>(this, config);
		newImporter->analyze();
		newImporter->writeAssets();
		e2::destroy(newImporter);
	}


	if (reimportDefaultFonts)
	{
		e2::FontImportConfig config;
		config.inputRegular = "LibreBaskerville-Regular.ttf";
		config.inputBold = "LibreBaskerville-Bold.ttf";
		config.inputItalic = "LibreBaskerville-Italic.ttf";
		config.outputFilepath = assetManager()->database().cleanPath("./engine/F_DefaultFont_Serif.e2a");
		e2::FontImporter* newImporter = e2::create<e2::FontImporter>(this, config);
		newImporter->analyze();
		newImporter->writeAssets();
		e2::destroy(newImporter);
	}

	if (reimportDefaultFonts)
	{
		e2::FontImportConfig config;
		config.inputRegular = "SourceCodePro-Regular.ttf";
		config.inputBold = "SourceCodePro-Bold.ttf";
		config.inputItalic = "SourceCodePro-Italic.ttf";
		config.inputBoldItalic = "SourceCodePro-BoldItalic.ttf";
		config.outputFilepath = assetManager()->database().cleanPath("./engine/F_DefaultFont_Monospace.e2a");
		e2::FontImporter* newImporter = e2::create<e2::FontImporter>(this, config);
		newImporter->analyze();
		newImporter->writeAssets();
		e2::destroy(newImporter);
	}

	if (false)
	{
		e2::TextureImportConfig config;
		config.input = "test.png";
		config.outputDirectory = "./engine/";
		e2::TextureImporter* newImporter = e2::create<e2::TextureImporter>(this, config);
		newImporter->analyze();
		newImporter->writeAssets();
		e2::destroy(newImporter);
	}

	if (false)
	{
		e2::MeshImportConfig config;
		config.input = "SM_Peach.fbx";
		config.outputDirectory = "./assets/";
		e2::MeshImporter* newImporter = e2::create < e2::MeshImporter>(this, config);
		newImporter->analyze();
		newImporter->writeAssets();
		e2::destroy(newImporter);
	}

	if (reimportDefaultFonts)
	{
		engine()->shutdown();
		return;
	}

	spawnWindow();
	spawnProcGen();
	//spawnWorldEditor();

}

void e2::Editor::shutdown()
{
	for (e2::Workspace* wrkspc : m_workspaces)
		e2::destroy(wrkspc);

	m_workspaces.clear();
	m_workspaceDestroySet.clear();

	for (e2::EditorWindow* wnd : m_windows)
		e2::destroy(wnd);

	m_windows.clear();
	m_destroySet.clear();

	for (e2::Importer* importer : m_importers)
		e2::destroy(importer);

	m_importers.clear();
	m_importerDestroySet.clear();

}

void e2::Editor::update(double seconds)
{

	for (e2::Workspace* wrkspc : m_workspaceDestroySet)
	{
		if (wrkspc->m_window)
			wrkspc->m_window->removeWorkspace(wrkspc);

		m_workspaces.erase(wrkspc);
		e2::destroy(wrkspc);
	}
	m_workspaceDestroySet.clear();


	for (e2::Importer* importer : m_importerDestroySet)
	{
		m_importers.erase(importer);
		e2::destroy(importer);
	}
	m_importerDestroySet.clear();

	for (e2::EditorWindow* wnd : m_destroySet)
	{
		m_windows.erase(wnd);
		e2::destroy(wnd);
	}
	m_destroySet.clear();

	// If no windows are open, this is our signal to say our goodbyes
	if (m_windows.size() == 0)
		engine()->shutdown();
}

e2::ApplicationType e2::Editor::type()
{
	return e2::ApplicationType::Tool;
}

void e2::Editor::oneShotImporter(e2::Importer* importer)
{
	m_importers.insert(importer);
}

void e2::Editor::destroyImporter(e2::Importer* importer)
{
	m_importerDestroySet.insert(importer);
}

e2::EditorWindow* e2::Editor::spawnWindow()
{
	return e2::create<e2::EditorWindow>(this);
}

void e2::Editor::destroyWindow(e2::EditorWindow* wnd)
{
	m_destroySet.insert(wnd);
	
}

e2::WorldEditorWorkspace* e2::Editor::spawnWorldEditor()
{		
	// create a new workspace
	e2::WorldEditorWorkspace* newWorkspace = e2::create<e2::WorldEditorWorkspace>(this);
	m_workspaces.insert(newWorkspace);

	e2::EditorWindow* defaultWindow = nullptr;
	if (m_windows.size() > 0)
		defaultWindow = *m_windows.begin();
	else
		defaultWindow = spawnWindow();

	// add it to the default window 
	if(defaultWindow)
		defaultWindow->setActiveWorkspace(newWorkspace);

	return newWorkspace;
}

void e2::Editor::destroyWorldEditor(e2::WorldEditorWorkspace* wrksp)
{
	m_workspaceDestroySet.insert(wrksp);
}

e2::ProcGenWorkspace* e2::Editor::spawnProcGen()
{
	// create a new workspace
	e2::ProcGenWorkspace* newWorkspace = e2::create<e2::ProcGenWorkspace>(this);
	m_workspaces.insert(newWorkspace);

	e2::EditorWindow* defaultWindow = nullptr;
	if (m_windows.size() > 0)
		defaultWindow = *m_windows.begin();
	else
		defaultWindow = spawnWindow();

	// add it to the default window 
	if (defaultWindow)
		defaultWindow->setActiveWorkspace(newWorkspace);

	return newWorkspace;
}

void e2::Editor::destroyProcGen(e2::ProcGenWorkspace* wrksp)
{
	m_workspaceDestroySet.insert(wrksp);
}

int main(int argc, char** argv)
{

	e2::Engine engine;
	e2::Editor editor(&engine);

	engine.run(&editor);

	return 0;
}

e2::Workspace::Workspace(e2::Editor* ed)
	: m_editor(ed)
{

}

e2::Workspace::~Workspace()
{

}

e2::Editor* e2::Workspace::editor()
{
	return m_editor;
}

e2::Engine* e2::Workspace::engine()
{
	return editor()->engine();
}

void e2::Workspace::update(double deltaSeconds)
{

}

e2::EditorWindow* e2::Workspace::window()
{
	return m_window;
}

std::string const& e2::Workspace::displayName()
{
	static std::string s = "workspace";
	return s;
}

e2::EditorWindow::EditorWindow(e2::EditorContext* ctx)
	: e2::UIWindow(ctx, e2::UIWindowFlags( WF_Default))
	, m_editor(ctx->editor())
{
	m_editor->m_windows.insert(this);

	m_tabbar = e2::create<e2::Tabbar>(this);
}

e2::EditorWindow::~EditorWindow()
{
	e2::destroy(m_tabbar);

	m_editor->m_windows.erase(this);
}

void e2::EditorWindow::update(double deltaTime)
{
	e2::UIWindow::update(deltaTime);

	if (m_rhiWindow->wantsClose())
	{
		editor()->destroyWindow(this);
		return;
	}

	e2::UIStyle &style = uiManager()->workingStyle();

	e2::UIContext* ui = uiContext();


	static e2::Name id_rootV = "rootV";
	static e2::Name id_empty = "empty";

	float rootV[] = { 24.f * style.scale, 0.0f };
	ui->beginFlexV(id_rootV, rootV, 2);
		m_tabbar->update(deltaTime);
		if (m_activeWorkspace)
		{
			m_activeWorkspace->update(deltaTime);
		}
		else
		{
			ui->label(id_empty, "No workspace open.", 12, UITextAlign::Middle);
		}
	ui->endFlexV();	
}

void e2::EditorWindow::onSystemButton()
{
	if (m_menu)
	{
		m_menu->destroy();
		m_menu = nullptr;
	}
	else
	{
		e2::UIStyle& style = uiManager()->workingStyle();
		m_menu = e2::create<e2::EditorWindowMenu>(this);
		m_menu->rhiWindow()->position(uiContext()->mouseState().position);
		m_menu->rhiWindow()->size(glm::vec2( 120, 240 ) * style.scale);
	}
	
}

e2::Editor* e2::EditorWindow::editor()
{
	return m_editor;
}

e2::Engine* e2::EditorWindow::engine()
{
	return m_editor->engine();
}

bool e2::EditorWindow::hasWorkspace(e2::Workspace* wrkspc)
{
	return m_workspaces.contains(wrkspc);
}

void e2::EditorWindow::addWorkspace(e2::Workspace* wrkspc)
{
	// already exists here
	if (wrkspc->m_window == this)
		return;

	// if workspace is already in other window, remove it from there
	if (wrkspc->m_window)
	{
		wrkspc->m_window->removeWorkspace(wrkspc);
	}

	m_workspaces.insert(wrkspc);
	wrkspc->m_window = this;

	// make active 
	m_activeWorkspace = wrkspc;
}

void e2::EditorWindow::removeWorkspace(e2::Workspace* wrkspc)
{
	// not here
	if (wrkspc->m_window != this)
		return;

	wrkspc->m_window = nullptr;
	m_workspaces.erase(wrkspc);

	// resolve the active workspace (tab) for this window
	if (m_activeWorkspace == wrkspc)
	{
		if (m_workspaces.size() == 0)
			m_activeWorkspace = nullptr;
		else
			m_activeWorkspace = *m_workspaces.begin();
	}
}

void e2::EditorWindow::setActiveWorkspace(e2::Workspace* newActive)
{
	// If we dont have the workspace in this window, move it here
	if (!hasWorkspace(newActive))
	{
		addWorkspace(newActive);
	}

	m_activeWorkspace = newActive;

}

std::unordered_set<e2::Workspace*>& e2::EditorWindow::workspaces()
{
	return m_workspaces;
}

e2::EditorWindowMenu::EditorWindowMenu(e2::EditorWindow* parent)
	: e2::UIWindow(parent, WF_PopupMenu)
	, m_parent(parent)
{

}

e2::EditorWindowMenu::~EditorWindowMenu()
{
	m_parent->m_menu = nullptr;
}

void e2::EditorWindowMenu::update(double deltaTime)
{
	e2::UIWindow::update(deltaTime);
	e2::UIStyle const& style = uiManager()->workingStyle();

	e2::UIContext* ui = uiContext();

	//uiContext()->drawRasterText(FontFace::Monospace, 11, style.accents[UIAccent_Blue], {}, "asddd");

	const e2::Name id_menuStack = "menuStack";
	const e2::Name id_newWorld = "newWorld";
	const std::string newWorldText = "New World Editor..";

	ui->beginStackV(id_menuStack);
	if (ui->button(id_newWorld, newWorldText))
	{
		editor()->spawnWorldEditor();
		destroy();
	}

	//uiContext()->label("test1", "^sClose1", 11);
	//uiContext()->label("test2", "^sClose2", 11);
	//uiContext()->label("test3", "^sClose3", 11);

	uiContext()->endStackV();

}

e2::Editor* e2::EditorWindowMenu::editor()
{
	return m_parent->editor();
}

e2::Engine* e2::EditorWindowMenu::engine()
{
	return m_parent->engine();
}

e2::WorldEditorWorkspace::WorldEditorWorkspace(e2::Editor* ed)
	: e2::Workspace(ed)
{
	m_editorSession = e2::create<e2::WorldEditorSession>(ed);

	m_assetBrowser = e2::create<e2::AssetBrowser>(ed);
	m_logOutput = e2::create<e2::LogOutput>(ed);
	m_outliner = e2::create<e2::Outliner>(ed);
	m_properties = e2::create<e2::PropertiesPanel>(ed);
	m_viewport = e2::create<e2::OrbitalViewport>(ed, m_editorSession);
}

e2::WorldEditorWorkspace::~WorldEditorWorkspace()
{
	e2::destroy(m_viewport);
	e2::destroy(m_properties);
	e2::destroy(m_outliner);
	e2::destroy(m_logOutput);
	e2::destroy(m_assetBrowser);

	e2::destroy(m_editorSession);
}

void e2::WorldEditorWorkspace::update(double deltaSeconds)
{
	e2::Workspace::update(deltaSeconds);

	m_editorSession->tick(deltaSeconds);

	e2::UIStyle& style = uiManager()->workingStyle();
	e2::UIContext* ui = window()->uiContext();

	const e2::Name id_flexH = "flexH";
	const e2::Name id_flexHSL = "flexHSL";
	const e2::Name id_flexHSR = "flexHSR";
	const e2::Name id_flexB = "flexB";
	const e2::Name id_flexV = "flexV";
	const e2::Name id_flexVS = "flexVS";
	const e2::Name id_flexBS = "flexBS";
	const float flexV[] = { 0.0f,  4.0f * style.scale, m_bottomSize};

	const float sliderSize = 4.0 * style.scale;

	ui->beginFlexV(id_flexV, flexV, 3);
	{
		const float flexH [] = {m_leftSize, sliderSize, 0.0f, sliderSize, m_rightSize};
		ui->beginFlexH(id_flexH, flexH, 5);
		{
			m_outliner->update(ui, deltaSeconds);
			ui->flexHSlider(id_flexHSL, m_leftSize, 1.0f);
			m_viewport->update(ui, deltaSeconds);
			ui->flexHSlider(id_flexHSR, m_rightSize, -1.0f);
			m_properties->update(ui, deltaSeconds);
		}
;		ui->endFlexH();

		ui->flexVSlider(id_flexVS, m_bottomSize, -1.0f);

		const float flexB[] = {0.0f, sliderSize, m_logSize};
		ui->beginFlexH(id_flexB, flexB, 3);
		{
			m_assetBrowser->update(ui, deltaSeconds);
			ui->flexHSlider(id_flexBS, m_logSize, -1.0f);
			m_logOutput->update(ui, deltaSeconds);
		}
		ui->endFlexH();

	}
	ui->endFlexV();

}

e2::WorldEditorSession::WorldEditorSession(e2::Context* ctx)
	: e2::Session(ctx)
{
	e2::AssetManager* am = assetManager();
	e2::ALJDescription desc;
	am->prescribeALJ(desc, "assets/SM_CoordinateSpace.e2a");
	am->prescribeALJ(desc, "assets/SM_Cube.e2a");
	am->prescribeALJ(desc, "assets/SM_HexBase.e2a");
	am->queueWaitALJ(desc);
	
	e2::Entity* e = persistentWorld()->spawnEntity<e2::Entity>("p");
	e2::MeshComponent* m2 = e->spawnComponent<e2::MeshComponent>("m2");
	m2->mesh(am->get("assets/SM_Cube.e2a")->cast<e2::Mesh>());


}

e2::WorldEditorSession::~WorldEditorSession()
{

}

void e2::WorldEditorSession::tick(double seconds)
{
	e2::Session::tick(seconds);


}












/*
	if(simplex > 0.75f)
		newTileData.flags |= TileFlags::BiomeMountain;
	else if (simplex > 0.5f)
		newTileData.flags |= TileFlags::BiomeGrassland;
	else if (simplex > 0.25f)
		newTileData.flags |= TileFlags::BiomeShallow;
	else
		newTileData.flags |= TileFlags::BiomeOcean;

*/



e2::ProcGenWorkspace::ProcGenWorkspace(e2::Editor* ed)
	: e2::Workspace(ed)
{
	glm::uvec2 resolution{ 512, 512};

	e2::TextureCreateInfo texInf{};
	texInf.initialLayout = e2::TextureLayout::ShaderRead;
	texInf.format = TextureFormat::SRGB8A8;
	texInf.resolution = { resolution, 1 };
	m_outputTexture = renderContext()->createTexture(texInf);

	e2::RenderTargetCreateInfo renderTargetInfo{};
	renderTargetInfo.areaExtent = resolution;

	e2::RenderAttachment colorAttachment{};
	colorAttachment.target = m_outputTexture;
	colorAttachment.clearMethod = ClearMethod::ColorFloat;
	colorAttachment.clearValue.clearColorf32 = { 0.f, 0.f, 0.f, 1.0f };
	colorAttachment.loadOperation = LoadOperation::Clear;
	colorAttachment.storeOperation = StoreOperation::Store;
	renderTargetInfo.colorAttachments.push(colorAttachment);
	m_outputTarget = renderContext()->createRenderTarget(renderTargetInfo);

	e2::PipelineLayoutCreateInfo layInf{};
	layInf.pushConstantSize = sizeof(e2::ProcGenConstants);
	m_pipelineLayout = renderContext()->createPipelineLayout(layInf);

	m_commandBuffers[0] = renderManager()->framePool(0)->createBuffer({});
	m_commandBuffers[1] = renderManager()->framePool(1)->createBuffer({});

	std::string sourceData;
	if (!e2::readFileWithIncludes("shaders/procgen.fragment.glsl", sourceData))
	{
		LogError("Failed to read shader file.");
	}

	e2::ShaderCreateInfo shdrInf{};
	shdrInf.source = sourceData.c_str();
	shdrInf.stage = ShaderStage::Fragment;
	m_fragShader = renderContext()->createShader(shdrInf);

	e2::PipelineCreateInfo pipeInf{};
	pipeInf.shaders.push(renderManager()->fullscreenTriangleShader());
	pipeInf.shaders.push(m_fragShader);
	pipeInf.colorFormats = { e2::TextureFormat::SRGB8A8 };
	pipeInf.layout = m_pipelineLayout;
	m_pipeline = renderContext()->createPipeline(pipeInf);

	m_watchdogStamp = std::filesystem::last_write_time(std::filesystem::path("shaders/procgen.fragment.glsl"));

	m_watchdogHandle = FindFirstChangeNotificationW(L"shaders/", false, FILE_NOTIFY_CHANGE_LAST_WRITE);
	if (m_watchdogHandle == INVALID_HANDLE_VALUE)
	{
		LogError("{}", GetLastError());
	}
}

e2::ProcGenWorkspace::~ProcGenWorkspace()
{
	e2::discard(m_commandBuffers[1]);
	e2::discard(m_commandBuffers[0]);
	e2::discard(m_pipeline);
	e2::discard(m_pipelineLayout);
	e2::discard(m_fragShader);
	e2::discard(m_outputTexture);
	e2::discard(m_outputTarget);
}

void e2::ProcGenWorkspace::update(double deltaSeconds)
{
	e2::Workspace::update(deltaSeconds);

	updateShaderWatchdog();

	e2::UIStyle& style = uiManager()->workingStyle();
	e2::UIContext* ui = window()->uiContext();


	e2::ICommandBuffer* buff = m_commandBuffers[renderManager()->frameIndex()];

	m_constants.resolution = { 512, 512 };


	const e2::Name id_flexH = "flexH";
	const e2::Name id_flexHL = "flexHL";
	const e2::Name id_flexHSL = "flexHSL";
	const e2::Name id_flexHR = "flexHR";

	const float sliderSize = 4.0 * style.scale;

	const float flexH[] = { m_leftSize, sliderSize, 0.0f};
	ui->beginFlexH(id_flexH, flexH, 3);
	{
		
		ui->beginStackV("stackV_shiii");
		ui->sliderFloat("view", m_constants.zoom, 0.00f, 5000.0f);

		ui->renderState().cursor.y += 12.0f;

		ui->sliderFloat("param1", m_constants.param1, 0.0f, 5.0f);
		ui->sliderFloat("param2", m_constants.param2, 0.0f, 5.0f);
		ui->sliderFloat("param3", m_constants.param3, 0.0f, 5.0f);
		ui->sliderFloat("param4", m_constants.param4, 0.0f, 5.0f);
		
		ui->renderState().cursor.y += 12.0f;

		ui->sliderFloat("param5", m_constants.param5, 0.0f, 5.0f);
		ui->sliderFloat("param6", m_constants.param6, 0.0f, 5.0f);
		ui->sliderFloat("param7", m_constants.param7, 0.0f, 5.0f);
		ui->sliderFloat("param8", m_constants.param8, 0.0f, 5.0f);
		
		ui->renderState().cursor.y += 12.0f;

		ui->sliderFloat("param9", m_constants.param9, 0.0f, 5.0f);
		ui->sliderFloat("param10", m_constants.param10, 0.0f, 5.0f);
		ui->sliderFloat("param11", m_constants.param11, 0.0f, 5.0f);
		ui->sliderFloat("param12", m_constants.param12, 0.0f, 5.0f);
		
		ui->renderState().cursor.y += 12.0f;

		ui->sliderFloat("param13", m_constants.param13, 0.0f, 5.0f);
		ui->sliderFloat("param14", m_constants.param14, 0.0f, 5.0f);
		ui->sliderFloat("param15", m_constants.param15, 0.0f, 5.0f);
		ui->sliderFloat("param16", m_constants.param16, 0.0f, 5.0f);


		ui->endStackV();


		ui->flexHSlider(id_flexHSL, m_leftSize, 1.0f);

		e2::UIWidgetState* widgetState = ui->reserve(id_flexHR, {512.f, 512.f});
		glm::vec2 actualSize((glm::min)(widgetState->size.x, widgetState->size.y));

		ui->drawTexturedQuad(widgetState->position, actualSize,  0xFFFFFFFF, m_outputTexture);
	}
	ui->endFlexH();

	e2::PipelineSettings defaultSettings;
	buff->beginRecord(true, defaultSettings);
	buff->useAsAttachment(m_outputTexture);
	buff->beginRender(m_outputTarget);
	buff->bindPipeline(m_pipeline);
	buff->nullVertexLayout();
	buff->pushConstants(m_pipelineLayout, 0, sizeof(e2::ProcGenConstants), reinterpret_cast<uint8_t*>(&m_constants));
	buff->drawNonIndexed(3, 1);
	buff->endRender();
	buff->useAsDefault(m_outputTexture);
	buff->endRecord();

	renderManager()->queue(buff, nullptr, nullptr);

}

void e2::ProcGenWorkspace::updateShaderWatchdog()
{
	if (WaitForSingleObject(m_watchdogHandle, 0) == WAIT_OBJECT_0)
	{
		auto newStamp = std::filesystem::last_write_time(std::filesystem::path("shaders/procgen.fragment.glsl"));
		if (m_watchdogStamp != newStamp)
		{
			std::string sourceData;
			if (!e2::readFileWithIncludes("shaders/procgen.fragment.glsl", sourceData))
			{
				LogError("Failed to read shader file.");
			}

			e2::ShaderCreateInfo shdrInf{};
			shdrInf.source = sourceData.c_str();
			shdrInf.stage = ShaderStage::Fragment;
			e2::IShader* newShader = renderContext()->createShader(shdrInf);


			if (newShader->valid())
			{
				e2::discard(m_pipeline);
				e2::discard(m_fragShader);

				m_fragShader = newShader;

				e2::PipelineCreateInfo pipeInf{};
				pipeInf.shaders.push(renderManager()->fullscreenTriangleShader());
				pipeInf.shaders.push(m_fragShader);
				pipeInf.colorFormats = { e2::TextureFormat::SRGB8A8 };
				pipeInf.layout = m_pipelineLayout;
				m_pipeline = renderContext()->createPipeline(pipeInf);
			}
			else
			{
				e2::discard(newShader);
			}

			FindNextChangeNotification(m_watchdogHandle);
			m_watchdogStamp = newStamp;
		}
	}
}
