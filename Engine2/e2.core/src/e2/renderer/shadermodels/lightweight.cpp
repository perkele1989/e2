
#include "e2/renderer/shadermodels/lightweight.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/game/gamesession.hpp"
#include "e2/assets/mesh.hpp"
#include "e2/assets/material.hpp"
#include "e2/renderer/meshproxy.hpp"

#include <e2/rhi/threadcontext.hpp>

#include "e2/utils.hpp"


static char const* vertexSource = R"SRC(
#version 460 core


// Vertex attributes 
in vec4 vertexPosition;

#if defined(Vertex_Normals)
in vec4 vertexNormal;
in vec4 vertexTangent;
#endif

#if defined(Vertex_TexCoords01)
in vec4 vertexUv01;
#endif 

#if defined(Vertex_TexCoords23)
in vec4 vertexUv23;
#endif 

#if defined(Vertex_Color)
in vec4 vertexColor;
#endif

#if defined(Vertex_Bones)
in vec4 vertexWeights;
in uvec4 vertexIds;
#endif

// Fragment attributes

out vec4 fragmentPosition;

#if defined(Vertex_Normals)
out vec3 fragmentNormal;
out vec3 fragmentTangent;
out vec3 fragmentBitangent;
#endif

#if defined(Vertex_TexCoords01)
out vec4 fragmentUv01;
#endif

#if defined(Vertex_TexCoords23)
out vec4 fragmentUv23;
#endif

#if defined(Vertex_Color)
out vec4 fragmentColor;
#endif

// Push constants
layout(push_constant) uniform ConstantData
{
	mat4 normalMatrix;
	uvec2 resolution;
};

// Begin Set0: Renderer
layout(set = 0, binding = 0) uniform RendererData
{
	mat4 viewMatrix;
	mat4 projectionMatrix;
	vec4 time; // t, sin(t), cos(t), tan(t)
} renderer;

layout(set = 0, binding = 1)  uniform texture2D integratedBrdf;
layout(set = 0, binding = 2) uniform sampler brdfSampler;
layout(set = 0, binding = 3)  uniform texture2D frontBufferColor;
layout(set = 0, binding = 4)  uniform texture2D frontBufferPosition;
layout(set = 0, binding = 5)  uniform texture2D frontBufferDepth;
layout(set = 0, binding = 6) uniform sampler frontBufferSampler;
// End Set0

// Begin Set1: Mesh 
layout(set = 1, binding = 0) uniform MeshData 
{
	mat4 modelMatrix;
} mesh;
// End Set1

// Begin Set2: Material
layout(set = 2, binding = 0) uniform MaterialData
{
	vec4 albedo;
} material;

layout(set = 2, binding = 1) uniform texture2D albedoTexture;
layout(set = 2, binding = 2) uniform sampler albedoSampler;
// End Set2

void main()
{
	gl_Position = renderer.projectionMatrix * renderer.viewMatrix * mesh.modelMatrix * vertexPosition;

	// Write fragment attributes (in worldspace where applicable)
	// @todo skinned 
	fragmentPosition = mesh.modelMatrix * vertexPosition;
#if defined(Vertex_Normals)

	fragmentNormal = normalize(mesh.modelMatrix * normalize(vertexNormal)).xyz;
	fragmentTangent =  normalize(mesh.modelMatrix * normalize(vertexTangent)).xyz;
	fragmentBitangent = normalize(cross(fragmentNormal.xyz, fragmentTangent.xyz));
#endif

#if defined(Vertex_TexCoords01)
	fragmentUv01 = vertexUv01;
#endif

#if defined(Vertex_TexCoords23)
	fragmentUv23 = vertexUv23;
#endif

#if defined(Vertex_Color)
	fragmentColor = vertexColor;
#endif
}
)SRC";

static char const* fragmentSource = R"SRC(
#version 460 core

// Fragment attributes

in vec4 fragmentPosition;

#if defined(Vertex_Normals)
in vec3 fragmentNormal;
in vec3 fragmentTangent;
in vec3 fragmentBitangent;
#endif

#if defined(Vertex_TexCoords01)
in vec4 fragmentUv01;
#endif

#if defined(Vertex_TexCoords23)
in vec4 fragmentUv23;
#endif

#if defined(Vertex_Color)
in vec4 fragmentColor;
#endif

// Out color
out vec4 outColor;
out vec4 outPosition;

// Push constants
layout(push_constant) uniform ConstantData
{
	mat4 normalMatrix;
	uvec2 resolution;
};

// Begin Set0: Renderer
layout(set = 0, binding = 0) uniform RendererData
{
	mat4 viewMatrix;
	mat4 projectionMatrix;
	vec4 time; // t, sin(t), cos(t), tan(t)
} renderer;

layout(set = 0, binding = 1)  uniform texture2D integratedBrdf;
layout(set = 0, binding = 2) uniform sampler brdfSampler;
layout(set = 0, binding = 3)  uniform texture2D frontBufferColor;
layout(set = 0, binding = 4)  uniform texture2D frontBufferDepth;
layout(set = 0, binding = 5) uniform sampler frontBufferSampler;
// End Set0

// Begin Set1: Mesh 
layout(set = 1, binding = 0) uniform MeshData 
{
	mat4 modelMatrix;
} mesh;
// End Set1

// Begin Set2: Material
layout(set = 2, binding = 0) uniform MaterialData
{
	vec4 albedo;
} material;

layout(set = 2, binding = 1) uniform texture2D albedoTexture;
layout(set = 2, binding = 2) uniform sampler albedoSampler;
// End Set2

float map(float value, float min1, float max1, float min2, float max2)
{
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

void main()
{
	outColor = vec4(1.0, 1.0, 1.0, 1.0);

	
#if defined(Vertex_Normals)
	
	vec3 n = normalize(fragmentNormal);

	vec3 l = normalize(vec3(1.0, 1.0, 1.0));
	vec3 ndotl = vec3(clamp(dot(n, l), 0.0, 1.0));
	vec3 softl = vec3(dot(n,l) *0.5 + 0.5);


	//outColor.rgb = (ndotl * vec3(0.79, 0.87, 1.0) * 0.15) + (softl * vec3(1.0, 0.8, 0.6) *0.85);
	outColor.rgb = (ndotl * 0.15) + (softl *0.85);

#endif

	outColor *= material.albedo;

#if defined(Material_Albedo) && defined(Vertex_TexCoords01)
	vec4 albedoSample = texture(sampler2D(albedoTexture, albedoSampler), fragmentUv01.xy);
	outColor *= albedoSample;
#endif

#if defined(Vertex_Color)
	//outColor *= fragmentColor;
	//outColor = fragmentColor;

	outColor.rgb *= fragmentColor.rgb;

	float gridCoeff = smoothstep(0.9f, 0.95, fragmentColor.a);
	gridCoeff *= 0.1;
	outColor.rgb = mix(outColor.rgb, vec3(0.0, 0.0, 0.0), gridCoeff);

#endif

	outPosition = fragmentPosition;
}

)SRC";

e2::LightweightModel::LightweightModel()
	: e2::ShaderModel()
{
	m_pipelineCache.resize(uint16_t(e2::LightweightFlags::Count));
	for (uint16_t i = 0; i < uint16_t(e2::LightweightFlags::Count); i++)
	{
		m_pipelineCache[i] = {};
	}
}

e2::LightweightModel::~LightweightModel()
{
	for (uint16_t i = 0; i < m_pipelineCache.size(); i++)
	{
		if (m_pipelineCache[i].vertexShader)
			e2::destroy(m_pipelineCache[i].vertexShader);
		if (m_pipelineCache[i].fragmentShader)
			e2::destroy(m_pipelineCache[i].fragmentShader);
		if (m_pipelineCache[i].pipeline)
			e2::destroy(m_pipelineCache[i].pipeline);
	}
	e2::destroy(m_sampler);
	e2::destroy(m_proxyUniformBuffers[0]);
	e2::destroy(m_proxyUniformBuffers[1]);
	e2::destroy(m_descriptorPool);
	e2::destroy(m_pipelineLayout);
	e2::destroy(m_descriptorSetLayout);
}

void e2::LightweightModel::postConstruct(e2::Context* ctx)
{
	e2::ShaderModel::postConstruct(ctx);
	m_specification.requiredAttributes = e2::VertexAttributeFlags::Normal | e2::VertexAttributeFlags::TexCoords01;

	e2::DescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
	setLayoutCreateInfo.bindings = {
		{ e2::DescriptorBindingType::UniformBuffer , 1}, // ubo params
		{ e2::DescriptorBindingType::Texture, 1}, // texture
		{ e2::DescriptorBindingType::Sampler, 1}, // sampler
	};
	m_descriptorSetLayout = renderContext()->createDescriptorSetLayout(setLayoutCreateInfo);


	e2::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sets = {
		renderManager()->rendererSetLayout(),
		renderManager()->modelSetLayout(),
		m_descriptorSetLayout
	};
	pipelineLayoutCreateInfo.pushConstantSize = sizeof(e2::PushConstantData);
	m_pipelineLayout = renderContext()->createPipelineLayout(pipelineLayoutCreateInfo);

	//m_pipelineCache.reserve(128);


	e2::DescriptorPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.maxSets = e2::maxNumLightweightProxies * 2;
	poolCreateInfo.numTextures = e2::maxNumLightweightProxies * 2 * 1;
	poolCreateInfo.numSamplers = e2::maxNumLightweightProxies * 2 * 1;
	poolCreateInfo.numUniformBuffers = e2::maxNumLightweightProxies * 2 * 1;
	m_descriptorPool = mainThreadContext()->createDescriptorPool(poolCreateInfo);


	e2::DataBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.dynamic = true;
	bufferCreateInfo.size = renderManager()->paddedBufferSize(e2::maxNumLightweightProxies * sizeof(e2::LightweightData));
	bufferCreateInfo.type = BufferType::UniformBuffer;
	m_proxyUniformBuffers[0] = renderContext()->createDataBuffer(bufferCreateInfo);
	m_proxyUniformBuffers[1] = renderContext()->createDataBuffer(bufferCreateInfo);

	e2::SamplerCreateInfo samplerInfo{};
	samplerInfo.filter = SamplerFilter::Anisotropic;
	samplerInfo.wrap = SamplerWrap::Clamp;
	m_sampler = renderContext()->createSampler(samplerInfo);
}

e2::MaterialProxy* e2::LightweightModel::createMaterialProxy(e2::Session* session, e2::MaterialPtr material)
{
	e2::LightweightProxy *newProxy = e2::create<e2::LightweightProxy>(session, material);

	for (uint8_t i = 0; i < 2; i++)
	{
		newProxy->sets[i] = m_descriptorPool->createDescriptorSet(m_descriptorSetLayout);
		newProxy->sets[i]->writeUniformBuffer(0, m_proxyUniformBuffers[i], sizeof(e2::LightweightData), renderManager()->paddedBufferSize(sizeof(e2::LightweightData)) * newProxy->id);
		newProxy->sets[i]->writeSampler(2, m_sampler);
	}

	e2::LightweightData newData;
	newData.albedo = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	newProxy->uniformData.set(newData);

	return newProxy;
}

e2::IPipelineLayout* e2::LightweightModel::getOrCreatePipelineLayout(e2::MeshProxy* proxy, uint8_t submeshIndex)
{
	return m_pipelineLayout;
}

e2::IPipeline* e2::LightweightModel::getOrCreatePipeline(e2::MeshProxy* proxy, uint8_t submeshIndex, e2::RendererFlags rendererFlags)
{
	e2::SubmeshSpecification const &spec = proxy->asset->specification(submeshIndex);
	e2::LightweightProxy* lwProxy = static_cast<e2::LightweightProxy*>(proxy->materialProxies[submeshIndex]);

	uint16_t geometryFlags = (uint16_t)spec.attributeFlags;
	
	uint16_t materialFlags = 0;
	if (lwProxy->albedoTexture.data())
		materialFlags |= uint16_t(e2::LightweightFlags::Albedo);

	uint16_t lwFlagsInt = (geometryFlags << (uint16_t)e2::LightweightFlags::VertexFlagsOffset)
		| (uint16_t(rendererFlags) << (uint16_t)e2::LightweightFlags::RendererFlagsOffset)
		| (materialFlags);

	e2::LightweightFlags lwFlags = (e2::LightweightFlags)lwFlagsInt;

	if (m_pipelineCache[uint16_t(lwFlags)].pipeline)
	{
		return m_pipelineCache[uint16_t(lwFlags)].pipeline;
	}
	/*auto finder = m_pipelineCache.find(lwFlags);
	if (finder != m_pipelineCache.end())
	{
		return finder->second.pipeline;
	}*/

	e2::LightweightCacheEntry newEntry;

	e2::ShaderCreateInfo shaderInfo;

	e2::applyVertexAttributeDefines(spec.attributeFlags, shaderInfo);

	if ((lwFlags & e2::LightweightFlags::Shadow) == e2::LightweightFlags::Shadow)
		shaderInfo.defines.push({ "Renderer_Shadow", "1" });

	if ((lwFlags & e2::LightweightFlags::Skin) == e2::LightweightFlags::Skin)
		shaderInfo.defines.push({ "Renderer_Skin", "1" });

	if ((lwFlags & e2::LightweightFlags::Albedo) == e2::LightweightFlags::Albedo)
		shaderInfo.defines.push({"Material_Albedo", "1"});

	shaderInfo.stage = ShaderStage::Vertex;
	shaderInfo.source = vertexSource;
	newEntry.vertexShader = renderContext()->createShader(shaderInfo);

	shaderInfo.stage = ShaderStage::Fragment;
	shaderInfo.source = fragmentSource;
	newEntry.fragmentShader = renderContext()->createShader(shaderInfo);

	e2::PipelineCreateInfo pipelineInfo;
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.shaders = { newEntry.vertexShader, newEntry.fragmentShader };
	pipelineInfo.colorFormats = { e2::TextureFormat::RGBA8,  e2::TextureFormat::RGBA32 };
	pipelineInfo.depthFormat = { e2::TextureFormat::D32 };
	newEntry.pipeline = renderContext()->createPipeline(pipelineInfo);

	m_pipelineCache[uint16_t(lwFlags)] = newEntry;
	return newEntry.pipeline;
}

void e2::LightweightModel::invalidatePipelines()
{
	for (uint16_t i = 0; i < uint16_t(e2::LightweightFlags::Count); i++)
	{
		e2::LightweightCacheEntry& entry = m_pipelineCache[i];

		if (entry.vertexShader)
			e2::discard(entry.vertexShader);
		entry.vertexShader = nullptr;

		if (entry.fragmentShader)
			e2::discard(entry.fragmentShader);
		entry.fragmentShader = nullptr;

		if (entry.pipeline)
			e2::discard(entry.pipeline);
		entry.pipeline = nullptr;
	}
}

e2::LightweightProxy::LightweightProxy(e2::Session* inSession, e2::MaterialPtr materialAsset)
	: e2::MaterialProxy(inSession, materialAsset)
{
	// @todo dont use gamesession here, it uses the default game session and thus this breaks in PIE etc.
	// instead take a sesson as parametr when creating proxies or similar
	session->registerMaterialProxy(this);
	model = static_cast<e2::LightweightModel*>(materialAsset->model());
	id = model->proxyIds.create();
}

e2::LightweightProxy::~LightweightProxy()
{
	session->unregisterMaterialProxy(this);
	model->proxyIds.destroy(id);

	sets[0]->keepAround();
	e2::destroy(sets[0]);

	sets[1]->keepAround();
	e2::destroy(sets[1]);
}

void e2::LightweightProxy::bind(e2::ICommandBuffer* buffer, uint8_t frameIndex)
{
	buffer->bindDescriptorSet(model->m_pipelineLayout, 2, sets[frameIndex]);
}

void e2::LightweightProxy::invalidate(uint8_t frameIndex)
{
	if (uniformData.invalidate(frameIndex))
	{
		uint32_t proxyOffset = renderManager()->paddedBufferSize(sizeof(LightweightData)) * id;
		model->m_proxyUniformBuffers[frameIndex]->upload(reinterpret_cast<uint8_t const*>(&uniformData.data()), sizeof(LightweightData), 0, proxyOffset);
	}

	if (albedoTexture.invalidate(frameIndex))
	{
		e2::ITexture* tex = albedoTexture.data();
		if(tex)
			sets[frameIndex]->writeTexture(1, tex);
	}
}
