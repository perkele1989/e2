
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

void e2::Sound::write(e2::IStream& destination) const
{

}

bool e2::Sound::read(e2::IStream& source)
{

	if (version >= e2::AssetVersion::AudioStream)
	{
		source >> m_fmodStream;
	}

	source >> m_fmodDataSize;
	uint8_t const* srcData = source.read(m_fmodDataSize);

	m_fmodData = new uint8_t[m_fmodDataSize];
	memcpy(m_fmodData, srcData, m_fmodDataSize);

	FMOD_CREATESOUNDEXINFO exInfo{};
	exInfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
	exInfo.length = (unsigned int)m_fmodDataSize;

	FMOD_MODE mode = FMOD_OPENMEMORY_POINT | FMOD_3D | FMOD_LOOP_NORMAL | FMOD_3D_LINEARSQUAREROLLOFF;
	if (m_fmodStream)
		mode |= FMOD_CREATECOMPRESSEDSAMPLE;
	else
		mode |= FMOD_CREATESAMPLE;

	FMOD_RESULT result = audioManager()->coreSystem()->createSound(reinterpret_cast<char*>(m_fmodData), mode, &exInfo, &m_fmodSound);
	if (result != FMOD_OK)
	{
		LogError("Fmod: {}: {}", int32_t(result), FMOD_ErrorString(result));
		return false;
	}

	return true;
}
