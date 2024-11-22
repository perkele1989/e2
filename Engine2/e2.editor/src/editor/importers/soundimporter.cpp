
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
	std::string ext = std::filesystem::path(inputPath).extension().string();
	std::filesystem::path outFilename = std::filesystem::path(inputPath).replace_extension(".e2a").filename();
	std::string outFile = (fs::path(m_config.outputDirectory) / outFilename).string();
	e2::FileStream soundData(m_config.input, e2::FileMode::ReadOnly);

	if (!soundData.valid())
	{
		LogError("Failed to load sound.");
		return false;
	}
	

	e2::AssetHeader assetHeader;
	assetHeader.version = e2::AssetVersion::Latest;
	assetHeader.assetType = "e2::Sound";


	e2::HeapStream assetData;

	if (ext == ".mp3" || ext == ".ogg")
	{
		assetData << bool(true);
	}
	else
	{
		assetData << bool(false);
	}
	assetData << uint64_t(soundData.size());
	assetData << soundData;

	assetData.seek(0);
	assetHeader.size = assetData.size();

	e2::FileStream fileBuffer(outFile, e2::FileMode::ReadWrite | e2::FileMode::Truncate, true);
	fileBuffer << assetHeader;
	fileBuffer << assetData;
	if (fileBuffer.valid())
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
