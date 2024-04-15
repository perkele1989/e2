
#include "e2/managers/audiomanager.hpp"

#include "e2/application.hpp"
#include "e2/log.hpp"
#include "e2/transform.hpp"

#include <fmod_errors.h>

e2::AudioManager::AudioManager(Engine* owner)
	: e2::Manager(owner)
{

}

e2::AudioManager::~AudioManager()
{

}

void e2::AudioManager::initialize()
{
	FMOD_RESULT result = FMOD::Studio::System::create(&m_studioSystem);
	if (result != FMOD_OK)
	{
		LogError("Fmod: {}: {}", int32_t(result), FMOD_ErrorString(result));
		return;
	}

	constexpr uint32_t maxFmodChannels = 512;
	result = m_studioSystem->initialize(maxFmodChannels, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL | FMOD_INIT_3D_RIGHTHANDED, 0);
	if (result != FMOD_OK)
	{
		LogError("Fmod: {}: {}", int32_t(result), FMOD_ErrorString(result));
		return;
	}

	result = m_studioSystem->getCoreSystem(&m_coreSystem);
	if (result != FMOD_OK)
	{
		LogError("Fmod: {}: {}", int32_t(result), FMOD_ErrorString(result));
		return;
	}

}

void e2::AudioManager::shutdown()
{
	m_studioSystem->release();
}

void e2::AudioManager::preUpdate(double deltaTime)
{
}

namespace
{
	FMOD_VECTOR glmToFmod(glm::vec3 const& vec)
	{
		FMOD_VECTOR returner;
		returner.x = vec.x;
		returner.y = -vec.y;
		returner.z = -vec.z;
		return returner;
	}
}

void e2::AudioManager::update(double deltaTime)
{
	glm::vec4 worldFwd = { e2::worldForwardf(), 0.0f };
	glm::vec4 worldUp = { e2::worldUpf(), 0.0f };
	
	glm::vec3 pos = m_listenerTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	glm::vec3 fwd = m_listenerTransform * worldFwd;
	glm::vec3 up = m_listenerTransform * worldUp;
	
	FMOD_3D_ATTRIBUTES attribs;
	attribs.position = ::glmToFmod(pos);
	attribs.forward = ::glmToFmod(fwd);
	attribs.up = ::glmToFmod(up);
	attribs.velocity = ::glmToFmod({}); // @todo
	m_studioSystem->setListenerAttributes(0, &attribs);
}

void e2::AudioManager::setListenerTransform(glm::mat4 const& transform)
{
	m_listenerTransform = transform;
}
