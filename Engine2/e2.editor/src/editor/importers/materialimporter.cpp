
#include "editor/importers/materialimporter.hpp"

#include "e2/assets/asset.hpp"
#include "e2/utils.hpp"
#include "e2/managers/assetmanager.hpp"
#include "e2/rhi/texture.hpp"

#include <stb_image.h>

#include <filesystem>
#include <format>
#include <thread>

namespace fs = std::filesystem;

e2::MaterialImporter::MaterialImporter(e2::Editor* editor, MaterialImportConfig const& config)
	: e2::Importer(editor)
	, m_config(config)
{

}

e2::MaterialImporter::~MaterialImporter()
{

}

void e2::MaterialImporter::update(double seconds)
{
	e2::Importer::update(seconds);
}

bool e2::MaterialImporter::analyze()
{
	return true;
}

bool e2::MaterialImporter::writeAssets()
{
	std::filesystem::path inputPath = std::filesystem::path(m_config.input);

	std::string model;
	std::vector<std::pair<std::string, glm::vec4>> vectors;
	std::vector<std::pair<std::string, std::string>> textures;
	std::vector <std::pair<std::string, std::string>> defines;


	std::string linesStr;
	if (!e2::readFile(m_config.input, linesStr))
	{
		LogError("failed to read material file");
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
		if (cmd == "model" && parts.size() == 2)
		{
			model = parts[1];
		}
		if (cmd == "vec4" && parts.size() == 6)
		{
			std::string newName = parts[1];
			glm::vec4 newVec;
			newVec.x = std::atof(parts[2].c_str());
			newVec.y = std::atof(parts[3].c_str());
			newVec.z = std::atof(parts[4].c_str());
			newVec.w = std::atof(parts[5].c_str());
			vectors.push_back({ newName, newVec });
		}
		if (cmd == "texture" && parts.size() == 3)
		{
			std::string newName = parts[1];
			std::string texPath = parts[2];

			textures.push_back({ newName, texPath });
		}
		if (cmd == "define" && parts.size() == 3)
		{
			std::string newName = parts[1];
			std::string newValue = parts[2];

			defines.push_back({ newName, newValue });
		}
		
	}
		

	

	std::filesystem::path outFilename = std::filesystem::path(inputPath).replace_extension(".e2a").filename();

	std::string outFile = (fs::path(m_config.outputDirectory) / outFilename).string();
	std::string outFileLower = e2::toLower(outFile);
	


	e2::AssetHeader materialHeader;
	materialHeader.version = e2::AssetVersion::Latest;
	materialHeader.assetType = "e2::Material";

	e2::Buffer materialData;

	materialData << model;
	materialData << uint8_t(defines.size());
	for (std::pair<std::string, std::string> p : defines)
	{
		materialData << p.first;
		materialData << p.second;
	}

	materialData << uint8_t(vectors.size());
	for (std::pair<std::string,glm::vec4> p : vectors)
	{
		materialData << p.first;
		materialData << p.second;
	}

	materialData << uint8_t(textures.size());
	for (std::pair<std::string, std::string> p : textures)
	{
		e2::AssetEntry* entr = assetManager()->database().entryFromPath(p.second);
		if (!entr)
		{
			LogNotice("ignoring texture {}, as asset {} doesn't exist in database (did you forget to import the textures for this?)", p.first, p.second);
			continue;
		}

		materialData << p.first;
		//materialData << entr->uuid;

		e2::DependencySlot newDep;
		newDep.name = p.first;
		newDep.uuid = entr->uuid;
		materialHeader.dependencies.push(newDep);
	}



	materialHeader.size = materialData.size();

	e2::Buffer fileBuffer(true, 1024 + materialData.size());
	fileBuffer << materialHeader;
	fileBuffer.write(materialData.begin(), materialData.size());
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
