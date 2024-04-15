
#include "editor/importers/soundimporter.hpp"

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

e2::SoundImporter::SoundImporter(e2::Editor* editor, SoundImportConfig const& config)
	: e2::Importer(editor)
	, m_config(config)
{

}

e2::SoundImporter::~SoundImporter()
{

}

void e2::SoundImporter::update(double seconds)
{
	e2::Importer::update(seconds);
}

bool e2::SoundImporter::analyze()
{
	return true;
}

bool e2::SoundImporter::writeAssets()
{
	std::filesystem::path inputPath = std::filesystem::path(m_config.input);
	std::filesystem::path outFilename = std::filesystem::path(inputPath).replace_extension(".e2a").filename();
	std::string outFile = (fs::path(m_config.outputDirectory) / outFilename).string();
	e2::Buffer soundData;

	if (!soundData.readFromFile(m_config.input))
	{
		LogError("Failed to load sound.");
		return false;
	}
	

	e2::AssetHeader assetHeader;
	assetHeader.version = e2::AssetVersion::Latest;
	assetHeader.assetType = "e2::Sound";


	e2::Buffer assetData;


	assetData << uint64_t(soundData.size());
	assetData << soundData;


	assetHeader.size = assetData.size();

	e2::Buffer fileBuffer(true, 1024 + assetData.size());
	fileBuffer << assetHeader;
	fileBuffer.write(assetData.begin(), assetData.size());
	if (fileBuffer.writeToFile(outFile))
	{
		assetManager()->database().invalidateAsset(outFile);
		return true;
	}
	else
	{
		LogError("Failed to create sound, could not write to file: {}", outFile);
		return false;
	}
}
