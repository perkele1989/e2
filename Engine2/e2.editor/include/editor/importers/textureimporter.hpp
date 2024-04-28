
#pragma once 

#include <editor/importer.hpp>
//#include <e2/renderer/shared.hpp>
#include <e2/utils.hpp>
#include <e2/export.hpp>


#include <vector>
#include <map>

namespace e2
{

	struct TextureImportConfig
	{
		std::string input;
		std::string outputDirectory{"./assets/"};
	};


	class TextureImporter : public Importer
	{
	public:
		TextureImporter(e2::Editor* editor, TextureImportConfig const& config);
		virtual ~TextureImporter();

		virtual void update(double seconds) override;

		bool analyze();

		bool writeAssets();
	protected:

		TextureImportConfig m_config;

	};

	struct SheetImportConfig
	{
		std::string input;
		std::string outputDirectory{ "./assets/" };
	};


	class SheetImporter : public Importer
	{
	public:
		SheetImporter(e2::Editor* editor, SheetImportConfig const& config);
		virtual ~SheetImporter();

		virtual void update(double seconds) override;

		bool analyze();

		bool writeAssets();
	protected:

		SheetImportConfig m_config;

	};

}