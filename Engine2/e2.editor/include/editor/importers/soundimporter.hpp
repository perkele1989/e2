
#pragma once 

#include <editor/importer.hpp>
//#include <e2/renderer/shared.hpp>
#include <e2/utils.hpp>
#include <e2/export.hpp>


#include <vector>
#include <map>

namespace e2
{

	struct SoundImportConfig
	{
		std::string input;
		std::string outputDirectory{ "./assets/" };
	};


	class SoundImporter : public Importer
	{
	public:
		SoundImporter(e2::Editor* editor, SoundImportConfig const& config);
		virtual ~SoundImporter();

		virtual void update(double seconds) override;

		bool analyze();

		bool writeAssets();
	protected:

		SoundImportConfig m_config;

	};

}