
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


// these will do, piggy
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
// oink oink

#if !defined(E2_DEVELOPMENT)
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

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
		e2::TextureImporter* newImporter = e2::create < e2::TextureImporter>(this, config);
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
	spawnWorldEditor();

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

e2::MeshImporter* e2::Editor::spawnMeshImporter(std::string const& file)
{
	e2::MeshImportConfig config;
	config.input = file;
	//config.outputDirectory = "./assets/" + assetBrowser()->currentPath();
	e2::MeshImporter* newImporter = e2::create<e2::MeshImporter>(this, config);
	m_importers.insert(newImporter);
	return newImporter;
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



	if (m_editorSession->m3)
	{
		if (ui->keyboardState().keys[int16_t(e2::Key::Right)].pressed)
		{
			m_editorSession->m3->getTransform()->translate({ 0.2f, 0.0f, 0.0f }, TransformSpace::World);
		}
		if (ui->keyboardState().keys[int16_t(e2::Key::Left)].pressed)
		{
			m_editorSession->m3->getTransform()->translate({ -0.2f, 0.0f, 0.0f }, TransformSpace::World);
		}
	}
}

e2::WorldEditorSession::WorldEditorSession(e2::Context* ctx)
	: e2::Session(ctx)
{
	e2::AssetManager* am = assetManager();
	e2::ALJDescription desc;
	am->prescribeALJ(desc, "assets/SM_CoordinateSpace.e2a");
	am->prescribeALJ(desc, "assets/SM_Cube.e2a");
	am->prescribeALJ(desc, "assets/SM_HexBase.e2a");
	m_ticket = am->queueALJ(desc);
	m_loaded = false;


	e2::MeshPtr proc = e2::create<e2::Mesh>();
	e2::destroy(proc.get()); // @todo make this usagepattern for ptr's prettier 

	proc->postConstruct(ctx, e2::UUID());


	e2::StackVector<glm::vec4, 7> vertexData;
	e2::StackVector<uint32_t, 18> indexData;

	glm::vec2 v = { 0.0f, 1.0f };

	glm::vec3 origo{};
	vertexData.push({ origo, 1.0f });
	
	float topAngle = -30.0f;
	glm::vec2 topTheta = e2::rotate2d(v, topAngle);

	glm::vec3 top{topTheta.x, 0.0f, topTheta.y };
	vertexData.push({top, 1.0f});
	uint32_t topId = 1;


	for (uint32_t i = 0; i < 6; i++)
	{

		float iAngle = 60.0f * float(i);
		float bAngle = iAngle + 30.0f;
		glm::vec2 bTheta = e2::rotate2d(v, bAngle);

		indexData.push(0);

		indexData.push(topId);

		if (i == 5)
		{
			indexData.push(1);
		}
		else
		{
			topId = vertexData.size();
			vertexData.push({ bTheta.x, 0.0f, bTheta.y, 1.0f });
			indexData.push(topId);
		}
	}



	e2::ProceduralSubmesh procSm;
	procSm.material = renderManager()->defaultMaterial();
	procSm.attributes = VertexAttributeFlags::None;
	procSm.numVertices = 7;
	procSm.numIndices = 18;
	procSm.sourcePositions = vertexData.data();
	procSm.sourceIndices = indexData.data();



	proc->addProceduralSubmesh(procSm);
	proc->flagDone();

	e2::Entity* z = persistentWorld()->spawnEntity<e2::Entity>("z");
	e2::MeshComponent* zm = z->spawnComponent<e2::MeshComponent>("zm");
	zm->mesh(proc);
	zm->getTransform()->setTransformParent(z->getTransform());

	z->getTransform()->setTranslation({ 3.0f, 0.0f, 0.0f }, TransformSpace::World);

}

e2::WorldEditorSession::~WorldEditorSession()
{

}

void e2::WorldEditorSession::tick(double seconds)
{
	e2::Session::tick(seconds);

	if (!m_loaded)
	{
		e2::AssetManager* am = assetManager();

		e2::ALJState state = am->queryALJ(m_ticket);
		if (state.status == ALJStatus::Completed)
		{
			e2::Entity* e = persistentWorld()->spawnEntity<e2::Entity>("p");
			e2::MeshComponent* m2 = e->spawnComponent<e2::MeshComponent>("m2");
			m2->mesh(am->get("assets/SM_Cube.e2a")->cast<e2::Mesh>());

			m3 = m2;

			am->returnALJ(m_ticket);

			m_loaded = true;
		}
	}

}
