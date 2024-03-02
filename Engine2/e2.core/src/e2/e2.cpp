
#include "e2/e2.hpp"

#include "e2/timer.hpp"
#include "e2/application.hpp"

#include "e2/managers/assetmanager.hpp"
#include "e2/managers/asyncmanager.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/managers/gamemanager.hpp"
#include "e2/managers/typemanager.hpp"
#include "e2/managers/uimanager.hpp"

#include "e2/game/gamesession.hpp"

#include "e2/rhi/window.hpp"

e2::Engine::Engine()
{
	m_config = new Config();
	m_typeManager = new e2::TypeManager(this);
	m_asyncManager = new e2::AsyncManager(this);
	m_assetManager = new e2::AssetManager(this);
	m_renderManager = new e2::RenderManager(this);
	m_uiManager = new e2::UIManager(this);
	m_gameManager = new e2::GameManager(this);

	m_config->load("e2.cfg");
}

e2::Engine::~Engine()
{
	delete m_gameManager;
	delete m_uiManager;
	delete m_renderManager;
	delete m_assetManager;
	delete m_asyncManager;
	delete m_typeManager;
	
	delete m_config;

#if defined(E2_DEVELOPMENT)
	e2::printLingeringObjects();
#endif 
}

e2::Engine* e2::Engine::engine()
{
	return this;
}

void e2::Engine::run(e2::Application* app)
{
	m_application = app;

	m_typeManager->initialize();
	m_asyncManager->initialize();
	m_assetManager->initialize();
	m_renderManager->initialize();
	m_uiManager->initialize();
	m_gameManager->initialize();

	e2::Moment lastFrameStart;
	double deltaTime{};
	m_running = true;

	constexpr double targetMsPerFrame = 6.944444444444444;


	m_application->initialize();

	e2::Moment lastMetricsProbe;

	while (m_running)
	{

		if (lastFrameStart.durationSince().milliseconds() >= targetMsPerFrame)
		{
			deltaTime = lastFrameStart.durationSince().seconds();
			lastFrameStart = e2::timeNow();

			m_asyncManager->update(deltaTime);
			m_assetManager->update(deltaTime);
			m_renderManager->update(deltaTime);
			m_uiManager->update(deltaTime);
			m_gameManager->update(deltaTime);

			m_application->update(deltaTime);

			// Only dispatch render stuff if we are actually still running
			if (m_running)
				m_renderManager->dispatch();

			m_metrics.frameTimeMs[m_metrics.cursor] = (float)lastFrameStart.durationSince().milliseconds();

			if (++m_metrics.cursor >= e2::engineMetricsWindow)
			{

				double secondsPerMetricsFrame = lastMetricsProbe.durationSince().seconds();
				double secondsPerFrame = secondsPerMetricsFrame / double(e2::engineMetricsWindow);
				m_metrics.realCpuFps = 1.0 / secondsPerFrame;

				m_metrics.frameTimeMsHigh = 0.0f;
				m_metrics.frameTimeMsMean = 0.0f;

				m_metrics.gpuWaitTimeUsHigh = 0.0f;
				m_metrics.gpuWaitTimeUsMean = 0.0f;
				
				for (uint32_t i = 0; i < e2::engineMetricsWindow; i++)
				{
					if (m_metrics.frameTimeMs[i] > m_metrics.frameTimeMsHigh)
					{
						m_metrics.frameTimeMsHigh = m_metrics.frameTimeMs[i];
					}

					m_metrics.frameTimeMsMean += m_metrics.frameTimeMs[i];

					if (m_metrics.gpuWaitTimeUs[i] > m_metrics.gpuWaitTimeUsHigh)
					{
						m_metrics.gpuWaitTimeUsHigh = m_metrics.gpuWaitTimeUs[i];
					}

					m_metrics.gpuWaitTimeUsMean += m_metrics.gpuWaitTimeUs[i];
				}

				m_metrics.frameTimeMsMean /= e2::engineMetricsWindow;
				m_metrics.gpuWaitTimeUsMean /= e2::engineMetricsWindow;

				m_metrics.cursor = 0;

				lastMetricsProbe = e2::timeNow();
			}
			
			e2::ManagedObject::keepAroundTick();
		}
	}

	// waits for all render commands to have completed 
	m_renderManager->renderContext()->waitIdle();

	// stops running all async threads, and joins them 
	m_asyncManager->shutdown();
	
	// shutdown application
	m_application->shutdown();

	// prune all disposed and lingering objects
	e2::ManagedObject::keepAroundPrune();

	m_gameManager->shutdown();
	m_assetManager->shutdown();

	e2::ManagedObject::keepAroundPrune();

	m_uiManager->shutdown();
	m_renderManager->shutdown();

	m_typeManager->shutdown();

}

void e2::Engine::shutdown()
{
	m_running = false;
}

e2::EngineMetrics& e2::Engine::metrics()
{
	return m_metrics;

}
