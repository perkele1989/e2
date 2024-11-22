
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

	e2::HeapStream textureData;

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

	textureData.seek(0);
	textureHeader.size = textureData.size();

	e2::FileStream fileBuffer(outFile, e2::FileMode::ReadWrite | e2::FileMode::Truncate, true);
	fileBuffer << textureHeader;
	fileBuffer << textureData;
	if (fileBuffer.valid())
	{
		assetManager()->database().invalidateAsset(outFile);
		return true;
	}
	else
	{
		LogError("Failed to create texture, could not write to file: {}", outFile);
		//return e2::UUID();
		return false;
	}
}
















e2::SheetImporter::SheetImporter(e2::Editor* editor, SheetImportConfig const& config)
	: e2::Importer(editor)
	, m_config(config)
{

}

e2::SheetImporter::~SheetImporter()
{

}

void e2::SheetImporter::update(double seconds)
{
	e2::Importer::update(seconds);
}

bool e2::SheetImporter::analyze()
{
	return true;
}

bool e2::SheetImporter::writeAssets()
{
	std::filesystem::path inputPath = std::filesystem::path(m_config.input);

	struct _Sprite
	{
		std::string name;
		glm::vec2 offset;
		glm::vec2 size;
	};

	std::string texture;
	std::vector<_Sprite> sprites;


	std::string linesStr;
	if (!e2::readFile(m_config.input, linesStr))
	{
		LogError("failed to read spritesheet file");
		return false;
	}

	std::vector<std::string> lines = e2::split(linesStr, '\n');
	for (std::string& l : lines)
	{
		std::string trimmed = e2::trim(l);
		std::vector<std::string> parts = e2::split(trimmed, ' ');

		if (parts.size() < 2)
			continue;

		std::string cmd = e2::toLower(parts[0]);
		if (cmd == "texture" && parts.size() == 2)
		{
			texture = parts[1];
		}
		if (cmd == "sprite" && parts.size() == 6)
		{
			_Sprite newSprite;
			newSprite.name = parts[1];
			newSprite.offset.x = std::atof(parts[2].c_str());
			newSprite.offset.y = std::atof(parts[3].c_str());
			newSprite.size.x = std::atof(parts[4].c_str());
			newSprite.size.y = std::atof(parts[5].c_str());

			sprites.push_back(newSprite);
		}

	}

	std::filesystem::path outFilename = std::filesystem::path(inputPath).replace_extension(".e2a").filename();

	std::string outFile = (fs::path(m_config.outputDirectory) / outFilename).string();
	std::string outFileLower = e2::toLower(outFile);



	e2::AssetHeader spritesheetHeader;
	spritesheetHeader.version = e2::AssetVersion::Latest;
	spritesheetHeader.assetType = "e2::Spritesheet";

	e2::AssetEntry* entr = assetManager()->database().entryFromName(texture);
	if (!entr)
	{
		LogError("texture asset {} doesn't exist in database", texture);
		return false;
	}

	e2::DependencySlot newDep;
	newDep.dependencyName = "texture";
	newDep.assetName = entr->name;
	spritesheetHeader.dependencies.push(newDep);


	e2::HeapStream spritesheetData;

	spritesheetData << uint32_t(sprites.size());
	for (_Sprite& s : sprites)
	{
		spritesheetData << s.name;
		spritesheetData << s.offset;
		spritesheetData << s.size;
	}

	spritesheetData.seek(0);
	spritesheetHeader.size = spritesheetData.size();

	e2::FileStream fileBuffer(outFile, e2::FileMode::ReadWrite | e2::FileMode::Truncate, true);
	fileBuffer << spritesheetHeader;
	fileBuffer << spritesheetData;
	if (fileBuffer.valid())
	{
		assetManager()->database().invalidateAsset(outFile);

		return true;
	}
	else
	{
		LogError("Failed to create spritesheet, could not write to file: {}", outFile);
		return false;
	}
}
