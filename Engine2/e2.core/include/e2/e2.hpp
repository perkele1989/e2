
#pragma once 

#include <e2/export.hpp>
#include <e2/context.hpp>
#include <e2/config.hpp>
#include <e2/timer.hpp>
#include <e2/utils.hpp>

#include <vector>
#include <unordered_map>
#include <unordered_set>

#if defined(E2_PROFILER)
#define E2_BEGIN_SCOPE(g) profiler()->beginScope(__FUNCDNAME__, __FUNCTION__, e2::ProfileGroupId::##g);
#define E2_END_SCOPE() profiler()->endScope();

#define E2_BEGIN_SCOPE_CTX(g, x) x->profiler()->beginScope(__FUNCDNAME__, __FUNCTION__, e2::ProfileGroupId::##g);
#define E2_END_SCOPE_CTX(x) x->profiler()->endScope();


#define E2_PROFILE_SCOPE(g) ProfileBlock __profileBlock__##__LINE__(profiler(), __FUNCDNAME__, __FUNCTION__, e2::ProfileGroupId::##g)
#define E2_PROFILE_SCOPE_CTX(g, c) ProfileBlock __profileBlock__##__LINE__(c->profiler(), __FUNCDNAME__, __FUNCTION__, e2::ProfileGroupId::##g)
#else 
#define E2_BEGIN_SCOPE(g) void()
#define E2_END_SCOPE() void()
#define E2_BEGIN_SCOPE_CTX(g, x) void()
#define E2_END_SCOPE_CTX(x) void()
#define E2_PROFILE_SCOPE(g) void()
#define E2_PROFILE_SCOPE_CTX(g, x) void()
#endif



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
	class AudioManager;

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

#if defined(E2_PROFILER)

	enum class ProfileGroupId : uint8_t
	{
		Default = 0,
		Animation,
		Rendering,

		Count

	};

	struct E2_API ProfileGroup
	{
		std::string displayName;
		double timeInFrame{}; // time in seconds this function has been executing, in this frame
		double avgTimeInFrame{}; // average time this function spent executing, per frame
		double highTimeInFrame{}; // highest time this function spent executing, per frame
	};

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
		e2::ProfileGroupId groupId = e2::ProfileGroupId::Default;
		/** The function in which this scope was opened (and is closed) */
		ProfileFunction* function{};

		void openScope()
		{
			openTime = e2::timeNow();
		}

		void closeScope()
		{
			secondsInScope += openTime.durationSince().seconds();
		}

		/** moment in time this scope was opened */
		double secondsInScope{};
		e2::Moment openTime;
	};


	struct ProfileReport
	{
		std::vector<e2::ProfileFunction> functions;

		e2::StackVector<e2::ProfileGroup, size_t(e2::ProfileGroupId::Count)> groups;
	};

	class E2_API Profiler
	{
	public:

		Profiler();

		void start();
		void stop();

		void newFrame();

		void beginScope(std::string const& funcId, std::string const& funcDisplayName, e2::ProfileGroupId groupId = ProfileGroupId::Default);

		void endScope();

		ProfileReport report();

		uint64_t frameCount()
		{
			return m_numFrames;
		}

	protected:
		bool m_enabled{};
		uint64_t m_numFrames{};
		e2::StackVector<e2::ProfileScope, profileStackSize> m_stack;
		std::unordered_map<std::string, e2::ProfileFunction> m_functions;
		e2::StackVector<e2::ProfileGroup, size_t(e2::ProfileGroupId::Count)> m_groups;

	};

	struct ProfileBlock
	{
		Profiler* profiler{};

		ProfileBlock(Profiler* inProfiler, std::string const& funcId, std::string const& funcDisplayName, e2::ProfileGroupId groupId)
			: profiler(inProfiler)
		{
			profiler->beginScope(funcId, funcDisplayName, groupId);
		}

		~ProfileBlock()
		{
			profiler->endScope();
		}
	};
#endif

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

#if defined(E2_PROFILER)
		e2::EngineMetrics& metrics();
#endif
	protected:
		friend Context;

		bool m_running{ false };

#if defined(E2_PROFILER)
		e2::EngineMetrics m_metrics;
#endif

		e2::Application* m_application{};
		
		e2::Config* m_config{};

#if defined(E2_PROFILER)
		e2::Profiler* m_profiler{};
#endif
		e2::AudioManager* m_audioManager{};
		e2::RenderManager* m_renderManager{};
		e2::GameManager* m_gameManager{};
		e2::AsyncManager* m_asyncManager{};
		e2::AssetManager* m_assetManager{};
		e2::TypeManager* m_typeManager{};
		e2::UIManager* m_uiManager{};
		
	};

}