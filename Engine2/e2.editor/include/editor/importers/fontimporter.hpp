
#pragma once 

#include <editor/importer.hpp>
//#include <e2/renderer/shared.hpp>
#include <e2/utils.hpp>
#include <e2/export.hpp>


#include <vector>
#include <map>

namespace e2
{

	struct FontImportConfig
	{
		std::string inputRegular;
		std::string inputBold;
		std::string inputItalic;
		std::string inputBoldItalic;

		/** The output file, e.g. ./assets/F_DefaultFont.e2a */
		std::string outputFilepath;
	};


	class FontImporter : public Importer
	{
	public:
		FontImporter(e2::Editor* editor, FontImportConfig const& config);
		virtual ~FontImporter();

		virtual void update(double seconds) override;

		bool analyze() ;

		bool writeAssets();
	protected:

		FontImportConfig m_config;

	};

}