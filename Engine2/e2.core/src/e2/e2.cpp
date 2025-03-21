
#include "e2/e2.hpp"

#include "e2/timer.hpp"
#include "e2/application.hpp"

#include "e2/managers/assetmanager.hpp"
#include "e2/managers/asyncmanager.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/managers/gamemanager.hpp"
#include "e2/managers/typemanager.hpp"
#include "e2/managers/uimanager.hpp"
#include "e2/managers/networkmanager.hpp"

#include "e2/managers/audiomanager.hpp"

#include "e2/game/gamesession.hpp"

#include "e2/rhi/window.hpp"

e2::Engine::Engine()
{
#if defined(E2_PROFILER)
	m_profiler = e2::create<Profiler>();
#endif
	m_config = e2::create<Config>();
	m_typeManager = e2::create<e2::TypeManager>(this);
	m_asyncManager = e2::create<e2::AsyncManager>(this);
	m_networkManager = e2::create<e2::NetworkManager>(this);
	m_assetManager = e2::create<e2::AssetManager>(this);
	m_audioManager = e2::create<e2::AudioManager>(this);
	m_renderManager = e2::create<e2::RenderManager>(this);
	m_uiManager = e2::create<e2::UIManager>(this);
	m_gameManager = e2::create<e2::GameManager>(this);

	m_config->load("e2.cfg");
}

e2::Engine::~Engine()
{
	e2::destroy(m_gameManager);
	e2::destroy(m_uiManager);

	e2::destroy(m_renderManager);
	e2::destroy(m_audioManager);
	e2::destroy(m_assetManager);
	e2::destroy(m_networkManager);
	e2::destroy(m_asyncManager);
	e2::destroy(m_typeManager);
	
	e2::destroy(m_config);
#if defined(E2_PROFILER)
	e2::destroy(m_profiler);
#endif

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
	m_networkManager->initialize();
	m_assetManager->initialize();
	m_audioManager->initialize();
	m_renderManager->initialize();
	m_uiManager->initialize();
	m_gameManager->initialize();

	e2::Moment lastFrameStart;
	double deltaTime{};
	m_running = true;

	constexpr double targetMsPerFrame = 6.944444444444444; // 144 fps
	//constexpr double targetMsPerFrame = 3.33333333333; // 300 fps


	m_application->initialize();

	e2::Moment lastMetricsProbe;

	while (m_running)
	{

		if (lastFrameStart.durationSince().milliseconds() >= targetMsPerFrame)
		{
			deltaTime = lastFrameStart.durationSince().seconds();
			lastFrameStart = e2::timeNow();

#if defined(E2_PROFILER)
			m_profiler->newFrame();
#endif

			E2_BEGIN_SCOPE(Default);
			m_networkManager->update(deltaTime);
			m_asyncManager->update(deltaTime);
			m_assetManager->update(deltaTime);
			m_audioManager->update(deltaTime);
			m_renderManager->update(deltaTime);
			m_uiManager->update(deltaTime);
			
			m_gameManager->preUpdate(deltaTime);

			m_application->update(deltaTime);

			m_gameManager->update(deltaTime);

			// Only dispatch render stuff if we are actually still running
			if (m_running)
				m_renderManager->dispatch();

			E2_END_SCOPE();

#if defined(E2_PROFILER)
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
#endif
			
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

	m_networkManager->shutdown();

	m_audioManager->shutdown();

	m_assetManager->shutdown();

	e2::ManagedObject::keepAroundPrune();

	m_uiManager->shutdown();
	m_renderManager->shutdown();

	e2::ManagedObject::keepAroundPrune();

	m_typeManager->shutdown();

}

void e2::Engine::shutdown()
{
	m_running = false;
}

#if defined(E2_PROFILER)
e2::EngineMetrics& e2::Engine::metrics()
{
	return m_metrics;

}

e2::Profiler::Profiler()
{
	m_groups.resize(size_t(e2::ProfileGroupId::Count));

	m_groups[size_t(e2::ProfileGroupId::Default)].displayName = "Default";
	m_groups[size_t(e2::ProfileGroupId::Animation)].displayName = "Animation";
	m_groups[size_t(e2::ProfileGroupId::Rendering)].displayName = "Rendering";
	m_groups[size_t(e2::ProfileGroupId::Scripting)].displayName = "Scripting";
}

void e2::Profiler::start()
{
	if (m_enabled)
		return;


	for (uint8_t i = 0; i < uint8_t(e2::ProfileGroupId::Count); i++)
	{
		e2::ProfileGroup* grp = &m_groups[i];
		grp->highTimeInFrame = 0.0;

		grp->avgTimeInFrame = 0.0;
		grp->timeInFrame = 0.0;
	}

	m_functions.clear();
	m_stack.clear();
	m_enabled = true;
	m_numFrames = 0;
}

void e2::Profiler::stop()
{
	m_enabled = false;
}

void e2::Profiler::newFrame()
{
	if (!m_enabled)
		return;

	if (!m_stack.empty())
	{
		LogError("Mismatching profiler scope brackets; new frame but we got stuff on stack.");
		m_stack.clear();
	}

	for (auto& [id, function] : m_functions)
	{
		e2::ProfileFunction* func = &function;
		
		if (func->timeInFrame > func->highTimeInFrame)
			func->highTimeInFrame = func->timeInFrame;

		func->avgTimeInFrame = (double(m_numFrames) * func->avgTimeInFrame + func->timeInFrame) / (double(m_numFrames) + 1.0);

		func->timeInFrame = 0.0;
	}

	for (uint8_t i = 0; i < uint8_t(e2::ProfileGroupId::Count); i++)
	{
		e2::ProfileGroup* grp = &m_groups[i];
		if (grp->timeInFrame > grp->highTimeInFrame)
			grp->highTimeInFrame = grp->timeInFrame;

		grp->avgTimeInFrame = (double(m_numFrames) * grp->avgTimeInFrame + grp->timeInFrame) / (double(m_numFrames) + 1.0);
		grp->timeInFrame = 0.0;
	}

	m_numFrames++;
}

void e2::Profiler::beginScope(std::string const& funcId, std::string const& funcDisplayName, e2::ProfileGroupId groupId)
{
	if (!m_enabled)
		return;


	if (!m_stack.empty())
	{
		e2::ProfileScope& top = m_stack.back();
		top.closeScope();
	}

	e2::ProfileFunction* func{};
	auto finder = m_functions.find(funcId);
	if (finder == m_functions.end())
	{
		e2::ProfileFunction newFunc;
		newFunc.displayName = funcDisplayName;
		newFunc.functionId = funcId;

		m_functions[funcId] = newFunc;
		func = &m_functions.at(funcId);
	}
	else
	{
		func = &finder->second;
	}

	e2::ProfileScope newScope;
	newScope.groupId = groupId;
	newScope.function = func;
	newScope.openScope();
	m_stack.push(newScope);
}

void e2::Profiler::endScope()
{
	if (!m_enabled)
		return;

	if (m_stack.empty())
	{
		LogError("Mismatching profiler scope brackets; ended scope but we ain't got none.");
		return;
	}

	e2::ProfileScope& top = m_stack.back();
	top.closeScope();
	double secondsInScope = top.secondsInScope;
	m_stack.pop();

	e2::ProfileGroup* grp = &m_groups[size_t(top.groupId)];
	grp->timeInFrame += secondsInScope;

	top.function->timeInFrame += secondsInScope;
	top.function->timeInScope += secondsInScope;
	if (secondsInScope > top.function->highTimeInScope)
		top.function->highTimeInScope = secondsInScope;
	top.function->avgTimeInScope = (double(top.function->timesInScope) * top.function->avgTimeInScope + secondsInScope) / (double(top.function->timesInScope) + 1.0);

	top.function->timesInScope++;

	if (!m_stack.empty())
	{
		e2::ProfileScope& top = m_stack.back();
		top.openScope();
	}
}

e2::ProfileReport e2::Profiler::report()
{
	e2::ProfileReport returner;
	for (auto& [id, func] : m_functions)
	{
		returner.functions.push_back(func);
	}
	std::sort(returner.functions.begin(), returner.functions.end(), [](e2::ProfileFunction const& lhs, e2::ProfileFunction const& rhs) -> bool { return lhs.avgTimeInFrame > rhs.avgTimeInFrame;  });

	returner.groups = m_groups;
	
	return returner;
}
#endif