
#pragma once 

#include <e2/export.hpp>
#include <e2/context.hpp>
#include <e2/config.hpp>
#include <e2/timer.hpp>
#include <e2/utils.hpp>

#include <vector>
#include <unordered_map>
#include <unordered_set>

#define E2_BEGIN_SCOPE() profiler()->beginScope(__FUNCDNAME__, __FUNCTION__);
#define E2_END_SCOPE() profiler()->endScope();

#define E2_BEGIN_SCOPE_CTX(x) x->profiler()->beginScope(__FUNCDNAME__, __FUNCTION__);
#define E2_END_SCOPE_CTX(x) x->profiler()->endScope();


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


	constexpr uint64_t profileStackSize = 1024;


	struct E2_API ProfileFunction
	{
		/** decorated function name, used as unique identifier */
		std::string functionId;

		std::string displayName;

		double timeInFrame{}; // time in seconds this function has been executing, in this frame
		double avgTimeInFrame{}; // average time this function spent executing, per frame
		double highTimeInFrame{}; // highest time this function spent executing, per frame

		// variables below are never reset, and are updated every frame
		uint64_t timesInScope{}; // num times this function has had a scope, since profiling started
		double timeInScope{}; // time in seconds this function has been in scope, since profiling started

		double avgTimeInScope{}; // average time this function spent in scope, per invocation
		double highTimeInScope{}; // highest time this function spent in scope, per invocation

	};

	struct E2_API ProfileScope
	{
		/** The function in which this scope was opened (and is closed) */
		ProfileFunction* function{};

		/** moment in time this scope was opened */
		e2::Moment openTime;
	};


	class E2_API Profiler
	{
	public:

		void start();
		void stop();

		void newFrame();

		void beginScope(std::string const& funcId, std::string const& funcDisplayName);

		void endScope();

		std::vector<e2::ProfileFunction> report();

		uint64_t frameCount()
		{
			return m_numFrames;
		}

	protected:
		bool m_enabled{};
		uint64_t m_numFrames{};
		e2::StackVector<e2::ProfileScope, profileStackSize> m_stack;
		std::unordered_map<std::string, e2::ProfileFunction> m_functions;

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
		e2::Profiler* m_profiler{};

		e2::RenderManager* m_renderManager{};
		e2::GameManager* m_gameManager{};
		e2::AsyncManager* m_asyncManager{};
		e2::AssetManager* m_assetManager{};
		e2::TypeManager* m_typeManager{};
		e2::UIManager* m_uiManager{};
		
	};

}