
#pragma once 

#include <e2/export.hpp>
#include <e2/application.hpp>

#include <e2/managers/uimanager.hpp>

#include <editor/editorcontext.hpp>

#include <set>

namespace e2
{
	class Importer;
	class MeshImporter;
	class AssetBrowser;

	class EditorWindow;

	class Workspace : public e2::EditorContext
	{
	public:
		Workspace(e2::Editor* editor);
		virtual ~Workspace();

		virtual e2::Editor* editor() override;
		virtual e2::Engine* engine() override;

		virtual void update(double deltaSeconds);

		e2::EditorWindow* window();

		virtual std::string const& displayName();

	protected:
		friend e2::EditorWindow;
		friend e2::Editor;
		e2::Editor* m_editor{};
		e2::EditorWindow* m_window{};
	};

	class EditorWindowMenu;
	class AssetBrowser;
	class LogOutput;
	class PropertiesPanel;
	class OrbitalViewport;
	class Outliner;
	class Tabbar;
	class MeshComponent;

	class WorldEditorSession : public e2::Session
	{
	public:
		WorldEditorSession(e2::Context* ctx);
		virtual ~WorldEditorSession();

		virtual void tick(double seconds);

		e2::MeshComponent* m3{};

		e2::MeshPtr generatedMesh;

	protected:
		e2::ALJTicket m_ticket;
		bool m_loaded;
	};


	class WorldEditorWorkspace : public e2::Workspace
	{
	public:
		WorldEditorWorkspace(e2::Editor* ed);
		virtual ~WorldEditorWorkspace();

		virtual void update(double deltaSeconds) override;
	protected:

		e2::WorldEditorSession* m_editorSession{};

		e2::AssetBrowser* m_assetBrowser{};
		e2::LogOutput* m_logOutput{};
		e2::Outliner* m_outliner{};
		e2::OrbitalViewport* m_viewport{};
		e2::PropertiesPanel* m_properties{};

		float m_leftSize{ 100.0f };
		float m_rightSize{ 100.0f };

		float m_bottomSize{ 240.0f };
		float m_logSize{ 300.0f };
	};

	/** Extends an UI window with a tab-bar and maintains workspaces */
	class EditorWindow : public e2::UIWindow, public e2::EditorContext
	{
	public:
		EditorWindow(e2::EditorContext* ctx);
		virtual ~EditorWindow();

		virtual void update(double deltaTime) override;
		virtual void onSystemButton() override;

		virtual e2::Editor* editor() override;

		virtual e2::Engine* engine() override;

		/** returns true if this window contains the given workspace*/
		bool hasWorkspace(e2::Workspace* wrkspc);

		/** adds the given workspace to this window, removes it from other windows, makes it active in this window */
		void addWorkspace(e2::Workspace* wrkspc);

		/** remove the given workspace from this window */
		void removeWorkspace(e2::Workspace* wrkspc);

		/** sets the given workspace as active in this window, moves it here if it doesnt exist here */
		void setActiveWorkspace(e2::Workspace* newActive);

		std::unordered_set<e2::Workspace*> & workspaces();

	protected:
		friend e2::EditorWindowMenu;
		friend e2::Editor;

		e2::Editor* m_editor{};

		/** Non-owning set of workspaces (e2::Editor owns all workspaces and daermed basta) */
		e2::Workspace* m_activeWorkspace{};
		std::unordered_set<e2::Workspace*> m_workspaces;

		e2::EditorWindowMenu *m_menu{};
		e2::Tabbar* m_tabbar{};
	};

	/** The system menu for an editor window */
	class EditorWindowMenu : public e2::UIWindow, public e2::EditorContext
	{
	public:
		EditorWindowMenu(e2::EditorWindow* parent);
		virtual ~EditorWindowMenu();

		virtual void update(double deltaTime) override;

		virtual e2::Editor* editor() override;

		virtual e2::Engine* engine() override;
	protected:
		e2::EditorWindow* m_parent{};
	};

	class Editor : public e2::Application, public e2::EditorContext
	{
	public:

		Editor(e2::Context* ctx);
		virtual ~Editor();

		virtual e2::Editor* editor() override;

		virtual void initialize() override;
		virtual void shutdown() override;
		virtual void update(double seconds) override;

		virtual e2::ApplicationType type() override;


		e2::MeshImporter* spawnMeshImporter(std::string const& file);
		void destroyImporter(e2::Importer* importer);


		e2::EditorWindow* spawnWindow();
		void destroyWindow(e2::EditorWindow* wnd);


		e2::WorldEditorWorkspace* spawnWorldEditor();
		void destroyWorldEditor(e2::WorldEditorWorkspace* wrksp);

	protected:
		friend e2::EditorWindow;
		std::unordered_set<e2::EditorWindow*> m_windows;
		std::unordered_set<e2::EditorWindow*> m_destroySet;

		std::unordered_set<e2::Importer*> m_importers;
		std::unordered_set<e2::Importer*> m_importerDestroySet;

		std::unordered_set<e2::Workspace*> m_workspaces;
		std::unordered_set<e2::Workspace*> m_workspaceDestroySet;

	};
}