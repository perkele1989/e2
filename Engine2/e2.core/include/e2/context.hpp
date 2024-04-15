
#pragma once 

#include <e2/export.hpp>

namespace e2
{

	class Engine;
	class RenderManager;
	class AssetManager;
	class AsyncManager;
	class GameManager;
	class TypeManager;
	class Config;
	class IRenderContext;
	class IThreadContext;
	class GameSession;
	class UIManager;
	class Application;
	class AudioManager;

#if defined(E2_PROFILER)
	class Profiler;
#endif

	class E2_API Context
	{
	public:
		virtual ~Context();

		virtual Engine* engine() = 0;

#if defined(E2_PROFILER)
		Profiler* profiler();
#endif

		Application* application();
		Config* config();

		// extra helpers for common stuff
		e2::IRenderContext* renderContext();
		e2::IThreadContext* mainThreadContext();

		AudioManager* audioManager();
		RenderManager* renderManager();
		AssetManager* assetManager();
		AsyncManager* asyncManager();
		GameManager* gameManager();
		TypeManager* typeManager();
		UIManager* uiManager();
	};

}