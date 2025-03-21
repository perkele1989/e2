
#pragma once

#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/assets/asset.hpp>

#include <fmod.hpp>

namespace e2
{


	/** @tags(dynamic, arena, arenaSize=e2::maxNumSoundAssets) */
	class E2_API Sound : public e2::Asset
	{
		ObjectDeclaration()
	public:
		Sound();
		virtual ~Sound();

		virtual void write(e2::IStream& destination) const override;
		virtual bool read(e2::IStream& source) override;

		inline FMOD::Sound* fmodSound()
		{
			return m_fmodSound;
		}

	protected:
		uint64_t m_fmodDataSize{};
		uint8_t* m_fmodData{};
		FMOD::Sound* m_fmodSound;

		bool m_fmodStream{};
		
	};


}

#include "sound.generated.hpp"