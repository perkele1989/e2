
#pragma once 

#include <e2/export.hpp>
#include <e2/manager.hpp>

#include <fmod_studio.hpp>
#include <glm/glm.hpp>

#include <e2/assets/sound.hpp>

namespace e2
{
	class Engine;
	class GameSession;
	class Session;
	class Sound;

	class E2_API AudioManager : public Manager
	{
	public:
		AudioManager(Engine* owner);
		virtual ~AudioManager();

		virtual void initialize() override;
		virtual void shutdown() override;

		virtual void preUpdate(double deltaTime) override;
		virtual void update(double deltaTime) override;

		void setListenerTransform(glm::mat4 const& transform);

		void playOneShot(e2::Ptr<e2::Sound> sound, float volume = 1.0f, float spatiality = 0.0f, glm::vec3 const& position = {});

		inline FMOD::System* coreSystem()
		{
			return m_coreSystem;
		}
		inline FMOD::Studio::System* studioSystem()
		{
			return m_studioSystem;
		}

	protected:
		FMOD::System* m_coreSystem{};
		FMOD::Studio::System* m_studioSystem{};
		glm::mat4 m_listenerTransform;
	};

}