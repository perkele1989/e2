
#include "e2/assets/sound.hpp"

#include "e2/managers/audiomanager.hpp"

#include <fmod_errors.h>

e2::Sound::Sound()
	: e2::Asset()
{

}

e2::Sound::~Sound()
{

	if (m_fmodData)
	{
		delete[] m_fmodData;
		m_fmodData = nullptr;
	}
}

void e2::Sound::write(Buffer& destination) const
{

}

bool e2::Sound::read(Buffer& source)
{
	source >> m_fmodDataSize;
	uint8_t const* srcData = source.read(m_fmodDataSize);

	m_fmodData = new uint8_t[m_fmodDataSize];
	memcpy(m_fmodData, srcData, m_fmodDataSize);

	FMOD_CREATESOUNDEXINFO exInfo{};
	exInfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
	exInfo.length = m_fmodDataSize;

	FMOD_RESULT result = audioManager()->coreSystem()->createSound(reinterpret_cast<char*>(m_fmodData), FMOD_OPENMEMORY_POINT, &exInfo, &m_fmodSound);
	if (result != FMOD_OK)
	{
		LogError("Fmod: {}: {}", int32_t(result), FMOD_ErrorString(result));
		return false;
	}

	return true;
}
