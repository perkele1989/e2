
#include "e2/assets/material.hpp"

#include "e2/managers/rendermanager.hpp"
#include "e2/renderer/shadermodel.hpp"

e2::Material::~Material()
{

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
