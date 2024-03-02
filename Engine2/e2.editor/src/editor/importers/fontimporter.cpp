
#include "editor/importers/fontimporter.hpp"

#include "e2/assets/asset.hpp"
#include "e2/assets/font.hpp"
#include "e2/utils.hpp"
#include "e2/managers/assetmanager.hpp"
#include "e2/rhi/texture.hpp"

#include <stb_image.h>
#include <stb_rect_pack.h>
#include <stb_truetype.h>

#include <filesystem>
#include <format>
#include <thread>

namespace fs = std::filesystem;

e2::FontImporter::FontImporter(e2::Editor* editor, FontImportConfig const& config)
	: e2::Importer(editor)
	, m_config(config)
{

}

e2::FontImporter::~FontImporter()
{

}

void e2::FontImporter::update(double seconds)
{
	e2::Importer::update(seconds);
}

bool e2::FontImporter::analyze()
{
	return true;
}

bool e2::FontImporter::writeAssets()
{
	
	bool compatStyles[size_t(e2::FontStyle::Count)] = { true, false, false, false};

	e2::Buffer regularData;
	e2::Buffer boldData;
	e2::Buffer italicData;
	e2::Buffer boldItalicData;

	if (m_config.inputRegular.size() != 0)
	{
		if (!regularData.readFromFile(m_config.inputRegular))
		{
			LogError("Failed to load regular font.");
			return false;
		}
	}
	else
	{
		LogError("No regular font.");
		return false;
	}

	if (m_config.inputBold.size() != 0)
	{
		compatStyles[size_t(FontStyle::Bold)] = true;

		if (!boldData.readFromFile(m_config.inputBold))
		{
			LogError("Failed to load bold font.");
			return false;
		}
	}

	if (m_config.inputItalic.size() != 0)
	{
		compatStyles[size_t(FontStyle::Italic)] = true;

		if (!italicData.readFromFile(m_config.inputItalic))
		{
			LogError("Failed to load italic font.");
			return false;
		}
	}

	if (m_config.inputBoldItalic.size() != 0)
	{
		compatStyles[size_t(FontStyle::BoldItalic)] = true;

		if (!boldItalicData.readFromFile(m_config.inputBoldItalic))
		{
			LogError("Failed to load bolditalic font.");
			return false;
		}
	}

	e2::AssetHeader fontHeader;
	fontHeader.version = e2::AssetVersion::Latest;
	fontHeader.assetType = "e2::Font";

	
	e2::Buffer fontData;

	for (uint8_t i = 0; i < size_t(e2::FontStyle::Count); i++)
	{
		fontData << compatStyles[i];
	}
	

	fontData << uint64_t(regularData.size());
	fontData << regularData;

	if (compatStyles[size_t(e2::FontStyle::Bold)])
	{
		fontData << uint64_t(boldData.size());
		fontData << boldData;
	}

	if (compatStyles[size_t(e2::FontStyle::Italic)])
	{
		fontData << uint64_t(italicData.size());
		fontData << italicData;
	}

	if (compatStyles[size_t(e2::FontStyle::BoldItalic)])
	{
		fontData << uint64_t(boldItalicData.size());
		fontData << boldItalicData;
	}

	fontHeader.size = fontData.size();

	e2::Buffer fileBuffer(true, 1024 + fontData.size());
	fileBuffer << fontHeader;
	fileBuffer.write(fontData.begin(), fontData.size());
	if (fileBuffer.writeToFile(m_config.outputFilepath))
	{
		assetManager()->database().invalidateAsset(m_config.outputFilepath);
		return true;
	}
	else
	{
		LogError("Failed to create font, could not write to file: {}", m_config.outputFilepath);
		return false;
	}
}
