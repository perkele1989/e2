
#pragma once 

#include <e2/export.hpp>
#include <e2/context.hpp>
#include <e2/config.hpp>

#include <vector>

namespace e2
{
	class Application;
	class Manager;
	class RenderManager;
	class GameManager;
	class AsyncManager;
	class AssetManager;
	class TypeManager;
	class UIManager;

	/** Minimal and global engine performance metrics */
	constexpr uint32_t engineMetricsWindow = 120;
	struct E2_API EngineMetrics
	{
		uint32_t cursor{};

		float frameTimeMsHigh{};
		float frameTimeMsMean{};

		float gpuWaitTimeUsHigh{};
		float gpuWaitTimeUsMean{};

		// actual probed active frames in a second, disregards empty frames
		float realCpuFps{};

		/** Frame time for an entire engine tick in main thread, in milliseconds */
		float frameTimeMs[e2::engineMetricsWindow];

		/** Time it takes to wait for the GPU fence, in microseconds. If this gets high, we are GPU bound. */
		float gpuWaitTimeUs[e2::engineMetricsWindow];

		

	};

	class E2_API Engine : public Context
	{
	public:
		Engine();
		virtual ~Engine();

		virtual Engine* engine() override;

		void run(e2::Application* app);

		void shutdown();


		inline e2::Application* application() const
		{
			return m_application;
		}

		e2::EngineMetrics& metrics();

	protected:
		friend Context;

		bool m_running{ false };

		e2::EngineMetrics m_metrics;

		e2::Application* m_application{};
		
		e2::Config* m_config{};

		e2::RenderManager* m_renderManager{};
		e2::GameManager* m_gameManager{};
		e2::AsyncManager* m_asyncManager{};
		e2::AssetManager* m_assetManager{};
		e2::TypeManager* m_typeManager{};
		e2::UIManager* m_uiManager{};

	};

}