
#pragma once 

#include <editor/importer.hpp>
//#include <e2/renderer/shared.hpp>
#include <e2/utils.hpp>
#include <e2/export.hpp>


#include <vector>
#include <map>

namespace e2
{

	struct MaterialImportConfig
	{
		std::string input;
		std::string outputDirectory{"./assets/"};
	};


	class MaterialImporter : public Importer
	{
	public:
		MaterialImporter(e2::Editor* editor, MaterialImportConfig const& config);
		virtual ~MaterialImporter();

		virtual void update(double seconds) override;

		bool analyze();

		bool writeAssets();
	protected:

		MaterialImportConfig m_config;

	};

}