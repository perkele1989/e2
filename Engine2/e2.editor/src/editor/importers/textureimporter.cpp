
#include "editor/importers/textureimporter.hpp"

#include "e2/assets/asset.hpp"
#include "e2/utils.hpp"
#include "e2/managers/assetmanager.hpp"
#include "e2/rhi/texture.hpp"

#include <stb_image.h>

#include <filesystem>
#include <format>
#include <thread>

namespace fs = std::filesystem;

e2::TextureImporter::TextureImporter(e2::Editor* editor, TextureImportConfig const& config)
	: e2::Importer(editor)
	, m_config(config)
{

}

e2::TextureImporter::~TextureImporter()
{

}

void e2::TextureImporter::update(double seconds)
{
	e2::Importer::update(seconds);
}

bool e2::TextureImporter::analyze()
{
	return true;
}

bool e2::TextureImporter::writeAssets()
{
	std::filesystem::path inputPath = std::filesystem::path(m_config.input);
	

	std::vector<std::string> mipSources;

	bool srgb = true;
	bool hdr = false;

	bool doGenMips = false;

	if (inputPath.extension() == ".mips")
	{
		std::string linesStr;
		if (!e2::readFile(m_config.input, linesStr))
		{
			LogError("failed to read mips file");
			return false;
		}
		std::vector<std::string> lines = e2::split(linesStr, '\n');
		for (std::string& l : lines)
		{
			std::string trimmed = e2::trim(l);

			if (trimmed.starts_with("#"))
			{
				if (trimmed == "#hdr")
				{
					hdr = true;
					srgb = false;
				}

				if (trimmed == "#linear")
					srgb = false;


				continue;
			}

			if(trimmed.size() > 0)
				mipSources.push_back((inputPath.parent_path() / std::filesystem::path(trimmed)).string());
		}
		
	}
	else
	{
		std::string inFileLower = e2::toLower(inputPath.filename().string());
		srgb = !inFileLower.contains("normal") && !inFileLower.contains("roughness") && !inFileLower.contains("metalness") && !inFileLower.contains("linear") && !inFileLower.contains("hdr");
		if (inFileLower.contains("srgb") || inFileLower.contains("albedo"))
			srgb = true;

		hdr = inFileLower.contains("hdr");

		doGenMips = true;
		mipSources.push_back(m_config.input);
	}
	

	std::filesystem::path outFilename = std::filesystem::path(inputPath).replace_extension(".e2a").filename();

	std::string outFile = (fs::path(m_config.outputDirectory) / outFilename).string();
	std::string outFileLower = e2::toLower(outFile);
	


	e2::AssetHeader textureHeader;
	textureHeader.version = e2::AssetVersion::Latest;
	textureHeader.assetType = "e2::Texture2D";

	e2::Buffer textureData;

	int32_t x, y, n;

	// we do this to retrieve mip 0 resoulution
	if (hdr)
		stbi_loadf(mipSources[0].c_str(), &x, &y, &n, 4);
	else
		stbi_load(mipSources[0].c_str(), &x, &y, &n, 4);

	glm::uvec2 resolution(x, y);
	textureData << resolution;

	if (doGenMips)
	{
		textureData << uint8_t(e2::calculateMipLevels(resolution));
	}
	else
	{
		textureData << uint8_t(mipSources.size());
	}

	textureData << (doGenMips ? uint8_t(1) : uint8_t(0));

	textureData << uint8_t(srgb ? e2::TextureFormat::SRGB8A8 : hdr ? e2::TextureFormat::RGBA32 : e2::TextureFormat::RGBA8);
	
	for (uint8_t i = 0; i < mipSources.size(); i++)
	{
		uint8_t* imageData{};
		if (hdr)
			imageData = reinterpret_cast<uint8_t*>(stbi_loadf(mipSources[i].c_str(), &x, &y, &n, 4));
		else
			imageData = stbi_load(mipSources[i].c_str(), &x, &y, &n, 4);

		if (!imageData)
		{
			LogError("Failed to read file {}", mipSources[i]);
			return false;
		}

		if (hdr)
			textureData << uint64_t(x * y * 4 * 4);
		else
			textureData << uint64_t(x * y * 4);

		if (hdr)
			textureData.write(imageData, uint64_t(x * y * 4 * 4));
		else
			textureData.write(imageData, uint64_t(x * y * 4));

		stbi_image_free(imageData);
	}


	textureHeader.size = textureData.size();

	e2::Buffer fileBuffer(true, 1024 + textureData.size());
	fileBuffer << textureHeader;
	fileBuffer.write(textureData.begin(), textureData.size());
	if (fileBuffer.writeToFile(outFile))
	{
		assetManager()->database().invalidateAsset(outFile);
		assetManager()->database().validate(true);

		return true;
	}
	else
	{
		LogError("Failed to create texture, could not write to file: {}", outFile);
		//return e2::UUID();
		return false;
	}
}
