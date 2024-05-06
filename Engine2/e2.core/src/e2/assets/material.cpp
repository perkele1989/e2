
#include "e2/assets/material.hpp"

#include "e2/managers/rendermanager.hpp"
#include "e2/renderer/shadermodel.hpp"

#include "e2/utils.hpp"

e2::Material::~Material()
{
	for (e2::Session* s : sessions)
	{
		s->nukeDefaultMaterialProxy(e2::Ptr<Material>(this));
	}
}

void e2::Material::write(Buffer& destination) const
{

}

bool e2::Material::read(Buffer& source)
{
	e2::Name modelName;
	source >> modelName;

	m_model = renderManager()->getShaderModel(modelName);
	if (!m_model)
	{
		LogError("Incompatible shadermodel: {}", modelName);
		return false;
	}

	if (version < e2::AssetVersion::AddMaterialParameters)
		return true;

	uint8_t numDefines{};
	source >> numDefines;
	for (uint8_t i = 0; i < numDefines; i++)
	{
		e2::Name defineName;
		source >> defineName;

		e2::Name defineValue;
		source >> defineValue;

		m_defines[defineName] = defineValue;
	}

	uint8_t numVectors{};
	source >> numVectors;
	for (uint8_t i = 0; i < numVectors; i++)
	{
		e2::Name vectorName;
		source >> vectorName;

		glm::vec4 vectorValue;
		source >> vectorValue;

		m_vectorIndex[vectorName] = vectorValue;
	}

	uint8_t numTextures{};
	source >> numTextures;
	for (uint8_t i = 0; i < numTextures; i++)
	{
		e2::Name textureName;
		source >> textureName;

		
		e2::UUID textureUUID = findDependencyByName(textureName);
		if (!textureUUID.valid())
		{
			LogError("Broken asset, dependency missing");
			continue;
		}

		e2::Texture2DPtr textureAsset = assetManager()->get(textureUUID).cast<e2::Texture2D>();

		m_textureIndex[textureName] = textureAsset;
	}

	return true;
}

bool e2::Material::finalize()
{
	//IRenderContext* renderContext = renderManager()->context();
	//m_uniformBuffer = renderContext->createDataBuffer(DT_UniformBuffer, uint32_t(m_uniformBufferCPU.size()));
	//m_uniformBuffer->upload(m_uniformBufferCPU.begin(), uint32_t(m_uniformBufferCPU.size()), 0);
	//e2::MaterialPtr sharedFromThis(this);
	//m_defaultProxy = m_model->createMaterialProxy(sharedFromThis);

	return true;
}

void e2::Material::overrideModel(e2::ShaderModel* model)
{
	m_model = model;
}

glm::vec4 e2::Material::getVec4(e2::Name key, glm::vec4 const& fallback)
{
	auto finder = m_vectorIndex.find(key);
	if (finder == m_vectorIndex.end())
		return fallback;

	return finder->second;
}

e2::Ptr<e2::Texture2D> e2::Material::getTexture(e2::Name key, e2::Ptr<e2::Texture2D> fallback)
{
	auto finder = m_textureIndex.find(key);
	if (finder == m_textureIndex.end())
		return fallback;

	return finder->second;
}

e2::Name e2::Material::getDefine(e2::Name key, e2::Name fallback)
{
	auto finder = m_defines.find(key);
	if (finder == m_defines.end())
		return fallback;

	return finder->second;
}

bool e2::Material::hasDefine(e2::Name key)
{
	return m_defines.contains(key);
}
