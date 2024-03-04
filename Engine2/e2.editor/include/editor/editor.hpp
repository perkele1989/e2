
#pragma once 

#include <e2/export.hpp>
#include <e2/application.hpp>

#include <e2/managers/uimanager.hpp>

#include <editor/editorcontext.hpp>

#include <filesystem>
#include <set>

//#define WIN32_LEAN_AND_MEAN
//#include <Windows.h>

typedef void* HANDLE;

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

	protected:

	};

	struct ProcGenConstants
	{
		glm::uvec2 resolution;
		float zoom{ 1.0f };

		float param1{ 1.0f };
		float param2{ 1.0f };
		float param3{ 1.0f };
		float param4{ 1.0f };
		float param5{ 1.0f };
		float param6{ 1.0f };
		float param7{ 1.0f };
		float param8{ 1.0f };
		float param9{ 1.0f };
		float param10{ 1.0f };
		float param11{ 1.0f };
		float param12{ 1.0f };
		float param13{ 1.0f };
		float param14{ 1.0f };
		float param15{ 1.0f };
		float param16{ 1.0f };
	};

	class ProcGenWorkspace : public e2::Workspace
	{
	public:
		ProcGenWorkspace(e2::Editor* ed);
		virtual ~ProcGenWorkspace();
		virtual void update(double deltaSeconds) override;

		void updateShaderWatchdog();

	protected:

		ProcGenConstants m_constants;

		float m_leftSize{ 100.0f };
		e2::IRenderTarget* m_outputTarget{};
		e2::ITexture* m_outputTexture{};
		glm::uvec2 m_outputSize{};

		std::filesystem::file_time_type m_watchdogStamp;
		HANDLE m_watchdogHandle;

		e2::IShader* m_fragShader;
		e2::IPipelineLayout* m_pipelineLayout;
		e2::IPipeline* m_pipeline;
		e2::Pair<e2::ICommandBuffer*> m_commandBuffers{ nullptr };

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

		e2::ProcGenWorkspace* spawnProcGen();
		void destroyProcGen(e2::ProcGenWorkspace* wrksp);

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