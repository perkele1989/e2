
#include "e2/renderer/shadermodels/terrain.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/game/gamesession.hpp"
#include "e2/assets/mesh.hpp"
#include "e2/assets/material.hpp"
#include "e2/renderer/meshproxy.hpp"

#include <e2/rhi/threadcontext.hpp>

#include "e2/utils.hpp"

static char const* commonSource = R"SRC(

	#define EPSILON 1e-5

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

	layout(set = 2, binding = 1) uniform sampler mainSampler;
	layout(set = 2, binding = 2) uniform texture2D irradianceCube;
	layout(set = 2, binding = 3) uniform texture2D radianceCube;

	layout(set = 2, binding = 4) uniform texture2D mountainAlbedo;
	layout(set = 2, binding = 5) uniform texture2D mountainNormal;

	layout(set = 2, binding = 6) uniform texture2D fieldsAlbedo;
	layout(set = 2, binding = 7) uniform texture2D fieldsNormal;

	layout(set = 2, binding = 8) uniform texture2D sandAlbedo;
	layout(set = 2, binding = 9) uniform texture2D sandNormal;

	layout(set = 2, binding = 10) uniform texture2D greenAlbedo;
	layout(set = 2, binding = 11) uniform texture2D greenNormal;

	
	// End Set2

	vec3 permute(vec3 x)
	{
		return mod(((x*34.0)+1.0)*x, 289.0);
	}

	float simplex(vec2 v){
		  const vec4 C = vec4(0.211324865405187, 0.366025403784439, -0.577350269189626, 0.024390243902439);
		  vec2 i  = floor(v + dot(v, C.yy) );
		  vec2 x0 = v -   i + dot(i, C.xx);
		  vec2 i1;
		  i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
		  vec4 x12 = x0.xyxy + C.xxzz;
		  x12.xy -= i1;
		  i = mod(i, 289.0);
		  vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
		  + i.x + vec3(0.0, i1.x, 1.0 ));
		  vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy),
			dot(x12.zw,x12.zw)), 0.0);
		  m = m*m ;
		  m = m*m ;
		  vec3 x = 2.0 * fract(p * C.www) - 1.0;
		  vec3 h = abs(x) - 0.5;
		  vec3 ox = floor(x + 0.5);
		  vec3 a0 = x - ox;
		  m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );
		  vec3 g;
		  g.x  = a0.x  * x0.x  + h.x  * x0.y;
		  g.yz = a0.yz * x12.xz + h.yz * x12.yw;
		  return 130.0 * dot(m, g);
	}

	float sampleSimplex(vec2 position, float scale)
	{
		return simplex(position*scale) * 0.5 + 0.5;
	}

	float sampleBaseHeight(vec2 position)
	{
		float h1p = 0.42;
		float scale1 = 0.058;
		float h1 = pow(sampleSimplex(position, scale1), h1p);

		float semiStart = 0.31;
		float semiSize = 0.47;
		float h2p = 0.013;
		float h2 = smoothstep(semiStart, semiStart + semiSize, sampleSimplex(position, h2p));

		float semiStart2 = 0.65; 
		float semiSize2 = 0.1;
		float h3p = (0.75 * 20) / 5000;
		float h3 = 1.0 - smoothstep(semiStart2, semiStart2 + semiSize2, sampleSimplex(position, h3p));
		return h1 * h2 * h3;
	}


)SRC";

static char const* vertexHeader = R"SRC(
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

	out vec3 fragmentNormal;
	out vec4 fragmentTangent;
	out vec4 fragmentColor;
)SRC";

static char const* vertexSource = R"SRC(
void main()
{
	gl_Position = renderer.projectionMatrix * renderer.viewMatrix * mesh.modelMatrix * vertexPosition;

	fragmentPosition = mesh.modelMatrix * vertexPosition;
#if defined(Vertex_Normals)

	fragmentNormal = normalize(mesh.modelMatrix * normalize(vertexNormal)).xyz;
	fragmentTangent =  vec4(normalize(mesh.modelMatrix * normalize(vec4(vertexTangent.xyz, 0.0f))).xyz, vertexTangent.w);
	//fragmentBitangent = normalize(cross(fragmentNormal.xyz, fragmentTangent.xyz));
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

static char const* fragmentHeader = R"SRC(
	#version 460 core

	// Fragment attributes

	in vec4 fragmentPosition;

	in vec3 fragmentNormal;
	in vec4 fragmentTangent;
	//in vec3 fragmentBitangent;
	in vec4 fragmentColor;

	// Out color
	out vec4 outColor;
	out vec4 outPosition;

	const vec2 invAtan = vec2(0.1591, 0.3183);
	vec2 equirectangularUv(vec3 direction)
	{
		vec2 uv = vec2(atan(direction.z, direction.x), asin(-direction.y));
		uv *= invAtan;
		uv += 0.5;
		return uv;
	}
)SRC";

static char const* fragmentSource = R"SRC(


vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, EPSILON, 1.0), 5.0);
}


vec3 heightblend(vec3 input1, float height1, vec3 input2, float height2)
{
    float height_start = max(height1, height2) - 0.05;
    float level1 = max(height1 - height_start, 0);
    float level2 = max(height2 - height_start, 0);
    return ((input1 * level1) + (input2 * level2)) / (level1 + level2);
}

vec3 heightlerp(vec3 input1, float height1, vec3 input2, float height2, float t)
{
    t = clamp(t, 0, 1);
    return heightblend(input1, height1 * (1 - t), input2, height2 * t);
}

void main()
{
	outPosition = fragmentPosition;
	outColor = vec4(1.0, 1.0, 1.0, 1.0);

	float texScaleMountains = 0.1;
	float texScaleFields = 0.5;
	float texScaleSand = 0.2;
	float texScaleGreen = 0.2;

	vec3 n = normalize(fragmentNormal.xyz);
	vec3 t = normalize(fragmentTangent.xyz);
	vec3 b = fragmentTangent.w * cross(n, t);

	vec3 nmMountains = normalize(texture(sampler2D(mountainNormal, mainSampler), fragmentPosition.xz * texScaleMountains).xyz * 2.0 - 1.0);
	vec3 nmFields = normalize(texture(sampler2D(fieldsNormal, mainSampler), fragmentPosition.xz * texScaleFields).xyz * 2.0 - 1.0);
	vec3 nmSand = normalize(texture(sampler2D(sandNormal, mainSampler), fragmentPosition.xz * texScaleFields).xyz * 2.0 - 1.0);
	
	vec3 nm;
	//nm = normalize(mix(nmSand, nmFields, fragmentColor.r));	
	//nm = normalize(mix(nm, nmMountains, fragmentColor.g));
	nm = normalize(mix(nmSand, nmMountains, fragmentColor.g));
	
	vec3 wn = normalize(nm.x * t + nm.y * b + nm.z * n);

	vec3 l = normalize(vec3(-1.0, -1.0, -1.0));

	vec3 ndotl = vec3(clamp(dot(wn, l), EPSILON, 1.0));
	vec3 softl = vec3(dot(wn,l) *0.5 + 0.5);

	vec3 v = normalize(fragmentPosition.xyz - (inverse(renderer.viewMatrix) * vec4(0.0, 0.0, 0.0, 1.0)).xyz);//-normalize((inverse(renderer.viewMatrix) * vec4(0.0, 0.0, -1.0, 0.0)).xyz);
	float vdotn = pow(1.0 - clamp(dot(v, wn), EPSILON, 1.0), 5.0);




	vec3 flatv = v;
	flatv.y = 0.0;
	flatv = normalize(flatv);
	float vdotn2 = pow(1.0 - clamp(dot(flatv, n), EPSILON, 1.0), 16.0);
	float heightCoeff = smoothstep(0.1, 1.0, -fragmentPosition.y);	
	float mountainFresnel = vdotn2 * heightCoeff;
	
	float vdotn3 = pow(1.0 - clamp(dot(v, wn), EPSILON, 1.0), 0.25);

	vec3 albedoSand = texture(sampler2D(sandAlbedo, mainSampler), fragmentPosition.xz * texScaleSand).rgb;
	vec3 albedoMountains = texture(sampler2D(mountainAlbedo, mainSampler), fragmentPosition.xz * texScaleMountains).rgb;
	vec3 albedoFields = texture(sampler2D(fieldsAlbedo, mainSampler), fragmentPosition.xz * texScaleFields).rgb;
	vec3 albedoGreen = texture(sampler2D(greenAlbedo, mainSampler), fragmentPosition.xz * texScaleGreen).rgb;

	float bigSimplex = simplex(fragmentPosition.xz * 6) * 0.5 + 0.5;
	float smallSimplex = simplex(fragmentPosition.xz * 2) * 0.5 + 0.5;

	float smallSimplexS = simplex(fragmentPosition.xz * 1.1) * 0.5 + 0.5;

	float mainHeight = sampleBaseHeight(fragmentPosition.xz);
	float greenCoeff = clamp(smoothstep(0.4 + 0.1 * smallSimplexS, 0.5+ 0.1 * smallSimplexS, mainHeight), 0.0, 1.0);

	greenCoeff = clamp(fragmentColor.r * greenCoeff, 0.0, 1.0);



	albedoSand = heightlerp(pow(albedoSand, vec3(1.4)), 0.5, albedoSand, smallSimplexS, smoothstep(-0.1, 0.0, -fragmentPosition.y));

	vec3 albedo;
	albedo = heightlerp(albedoSand, 0.5, albedoGreen, bigSimplex * smallSimplex, greenCoeff);
	albedo = heightlerp(albedo, 0.5, albedoMountains, bigSimplex, fragmentColor.g);

	//albedo = mix(albedoSand, albedoGreen, greenCoeff);
	//albedo = mix(albedo, albedoMountains, fragmentColor.g);

	float brdfy = clamp(dot(wn, v), EPSILON, 1.0);
	float roughness = 0.3;
	float metallic = 0.0;
	 
	vec3 F0 = vec3(0.04); 
    vec3 specularCoeff = mix(F0, albedo, metallic); 

	vec3 diffuseCoeff = albedo * (1.0 - F0) * (1.0 - metallic);

	float lod = roughness * 6.0;
	vec3 r = reflect(-v, wn);
	float iblStrength = 1.0;
	vec3 irr = texture(sampler2D(irradianceCube, mainSampler), equirectangularUv(wn)).rgb* iblStrength;
	vec3 rad = textureLod(sampler2D(radianceCube, mainSampler), equirectangularUv(r), lod).rgb* iblStrength;

	vec2 brdf = textureLod(sampler2D(integratedBrdf, brdfSampler), vec2(brdfy, roughness), 0.0).xy;
	


	// indirect light, base
	outColor.rgb =  vec3(0.0);

	// soft base sun light, beautiful
	outColor.rgb += albedo * vec3(0.76, 0.8, 1.0) * softl * 1.0;

	// bravissimo!!! mountain fresnel is beaut!!
	outColor.rgb += vec3(1.0, 0.8, 0.76) * mountainFresnel * 0.65;

	// ibl specular
	outColor.rgb += rad * specularCoeff *  (brdf.x + brdf.y) ;	

	// ibl diffuse 
	outColor.rgb += diffuseCoeff * irr;

	// debug refl
	//outColor.rgb = F;

	// debug norm
	//outColor.rgb = clamp(vec3(wn.x, wn.z, -wn.y) * 0.5 + 0.5, vec3(EPSILON), vec3(1.0));

	//outColor.rgb = vec3(brdf.g);

	// debug ndotl 
	//outColor.rgb = ndotl;


	float gridCoeff2 = smoothstep(0.92, 0.95, fragmentColor.a);
	gridCoeff2 *= 0.22;
	outColor.rgb = mix(outColor.rgb, vec3(0.0), gridCoeff2);

}
)SRC";

e2::TerrainModel::TerrainModel()
	: e2::ShaderModel()
{
	m_pipelineCache.resize(uint16_t(e2::TerrainFlags::Count));
	for (uint16_t i = 0; i < uint16_t(e2::TerrainFlags::Count); i++)
	{
		m_pipelineCache[i] = {};
	}
}

e2::TerrainModel::~TerrainModel()
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

void e2::TerrainModel::postConstruct(e2::Context* ctx)
{
	e2::ShaderModel::postConstruct(ctx);
	m_specification.requiredAttributes = e2::VertexAttributeFlags::Normal | e2::VertexAttributeFlags::TexCoords01;

	e2::DescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
	setLayoutCreateInfo.bindings = {
		{ e2::DescriptorBindingType::UniformBuffer , 1}, // ubo params
		{ e2::DescriptorBindingType::Sampler, 1}, // sampler
		{ e2::DescriptorBindingType::Texture, 1}, // irr
		{ e2::DescriptorBindingType::Texture, 1}, // rad
		{ e2::DescriptorBindingType::Texture, 1}, // mountainAlbedo
		{ e2::DescriptorBindingType::Texture, 1}, // mountainNormal
		{ e2::DescriptorBindingType::Texture, 1}, // fieldsAlbedo
		{ e2::DescriptorBindingType::Texture, 1}, // fieldsNormal
		{ e2::DescriptorBindingType::Texture, 1}, // sandAlbedo
		{ e2::DescriptorBindingType::Texture, 1}, // sandNormal
		{ e2::DescriptorBindingType::Texture, 1}, // greenAlbedo
		{ e2::DescriptorBindingType::Texture, 1}, // greenNormal
		
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
	poolCreateInfo.maxSets = e2::maxNumTerrainProxies * 2;
	poolCreateInfo.numTextures = e2::maxNumTerrainProxies * 2 * 4;
	poolCreateInfo.numSamplers = e2::maxNumTerrainProxies * 2 * 1;
	poolCreateInfo.numUniformBuffers = e2::maxNumTerrainProxies * 2 * 1;
	m_descriptorPool = mainThreadContext()->createDescriptorPool(poolCreateInfo);


	e2::DataBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.dynamic = true;
	bufferCreateInfo.size = renderManager()->paddedBufferSize(e2::maxNumTerrainProxies * sizeof(e2::TerrainData));
	bufferCreateInfo.type = BufferType::UniformBuffer;
	m_proxyUniformBuffers[0] = renderContext()->createDataBuffer(bufferCreateInfo);
	m_proxyUniformBuffers[1] = renderContext()->createDataBuffer(bufferCreateInfo);

	e2::SamplerCreateInfo samplerInfo{};
	samplerInfo.filter = SamplerFilter::Anisotropic;
	samplerInfo.wrap = SamplerWrap::Repeat;
	m_sampler = renderContext()->createSampler(samplerInfo);

	//std::string cubemapName = "assets/lakeside_4k.e2a";
	//std::string cubemapName = "assets/the_sky_is_on_fire_4k.e2a";
	//std::string cubemapName = "assets/studio_small_03_4k.e2a";
	
	std::string mountainAlbedoPath = "assets/Landscape_albedo_srgb.e2a";
	std::string mountainNormalPath = "assets/Landscape_normal_linear.e2a";

	std::string fieldsAlbedoPath = "assets/Fields_albedo_srgb.e2a";
	std::string fieldsNormalPath = "assets/Fields_normal_linear.e2a";

	std::string sandAlbedoPath = "assets/T_SandDesert_Albedo.e2a";
	std::string sandNormalPath = "assets/T_SandDesert_Normal.e2a";

	std::string greenAlbedoPath = "assets/T_Greenlands_Albedo.e2a";
	std::string greenNormalPath = "assets/T_Greenlands_Normal.e2a";

	std::string irrName = "assets/kloofendal_irr.e2a";
	std::string radName = "assets/kloofendal_rad.e2a";

	e2::ALJDescription aljDesc;
	assetManager()->prescribeALJ(aljDesc, mountainAlbedoPath);
	assetManager()->prescribeALJ(aljDesc, mountainNormalPath);
	assetManager()->prescribeALJ(aljDesc, fieldsAlbedoPath);
	assetManager()->prescribeALJ(aljDesc, fieldsNormalPath);
	assetManager()->prescribeALJ(aljDesc, sandAlbedoPath);
	assetManager()->prescribeALJ(aljDesc, sandNormalPath);
	assetManager()->prescribeALJ(aljDesc, greenAlbedoPath);
	assetManager()->prescribeALJ(aljDesc, greenNormalPath);
	assetManager()->prescribeALJ(aljDesc, irrName);
	assetManager()->prescribeALJ(aljDesc, radName);
	assetManager()->queueWaitALJ(aljDesc);

	m_irradianceCube = assetManager()->get(irrName).cast<e2::Texture2D>();
	m_radianceCube = assetManager()->get(radName).cast<e2::Texture2D>();
	m_mountainAlbedo = assetManager()->get(mountainAlbedoPath).cast<e2::Texture2D>();
	m_mountainNormal = assetManager()->get(mountainNormalPath).cast<e2::Texture2D>();

	m_sandAlbedo = assetManager()->get(sandAlbedoPath).cast<e2::Texture2D>();
	m_sandNormal = assetManager()->get(sandNormalPath).cast<e2::Texture2D>();

	m_fieldsAlbedo = assetManager()->get(fieldsAlbedoPath).cast<e2::Texture2D>();
	m_fieldsNormal = assetManager()->get(fieldsNormalPath).cast<e2::Texture2D>();


	m_greenAlbedo = assetManager()->get(greenAlbedoPath).cast<e2::Texture2D>();
	m_greenNormal = assetManager()->get(greenNormalPath).cast<e2::Texture2D>();
}

e2::MaterialProxy* e2::TerrainModel::createMaterialProxy(e2::Session* session, e2::MaterialPtr material)
{
	e2::TerrainProxy* newProxy = e2::create<e2::TerrainProxy>(session, material);

	for (uint8_t i = 0; i < 2; i++)
	{
		newProxy->sets[i] = m_descriptorPool->createDescriptorSet(m_descriptorSetLayout);
		newProxy->sets[i]->writeUniformBuffer(0, m_proxyUniformBuffers[i], sizeof(e2::TerrainData), renderManager()->paddedBufferSize(sizeof(e2::TerrainData)) * newProxy->id);
		newProxy->sets[i]->writeSampler(1, m_sampler);
		newProxy->sets[i]->writeTexture(2, m_irradianceCube->handle());
		newProxy->sets[i]->writeTexture(3, m_radianceCube->handle());

		newProxy->sets[i]->writeTexture(4, m_mountainAlbedo->handle());
		newProxy->sets[i]->writeTexture(5, m_mountainNormal->handle());

		newProxy->sets[i]->writeTexture(6, m_fieldsAlbedo->handle());
		newProxy->sets[i]->writeTexture(7, m_fieldsNormal->handle());

		newProxy->sets[i]->writeTexture(8, m_sandAlbedo->handle());
		newProxy->sets[i]->writeTexture(9, m_sandNormal->handle());
		
		newProxy->sets[i]->writeTexture(10, m_greenAlbedo->handle());
		newProxy->sets[i]->writeTexture(11, m_greenNormal->handle());

	}

	e2::TerrainData newData;
	newData.albedo = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	newProxy->uniformData.set(newData);

	return newProxy;
}

e2::IPipelineLayout* e2::TerrainModel::getOrCreatePipelineLayout(e2::MeshProxy* proxy, uint8_t submeshIndex)
{
	return m_pipelineLayout;
}

e2::IPipeline* e2::TerrainModel::getOrCreatePipeline(e2::MeshProxy* proxy, uint8_t submeshIndex, e2::RendererFlags rendererFlags)
{
	e2::SubmeshSpecification const& spec = proxy->asset->specification(submeshIndex);
	e2::TerrainProxy* lwProxy = static_cast<e2::TerrainProxy*>(proxy->materialProxies[submeshIndex]);

	uint16_t geometryFlags = (uint16_t)spec.attributeFlags;

	uint16_t materialFlags = 0;
	if (lwProxy->albedoTexture.data())
		materialFlags |= uint16_t(e2::TerrainFlags::Albedo);

	uint16_t lwFlagsInt = (geometryFlags << (uint16_t)e2::TerrainFlags::VertexFlagsOffset)
		| (uint16_t(rendererFlags) << (uint16_t)e2::TerrainFlags::RendererFlagsOffset)
		| (materialFlags);

	e2::TerrainFlags lwFlags = (e2::TerrainFlags)lwFlagsInt;

	if (m_pipelineCache[uint16_t(lwFlags)].pipeline)
	{
		return m_pipelineCache[uint16_t(lwFlags)].pipeline;
	}

	e2::TerrainCacheEntry newEntry;

	e2::ShaderCreateInfo shaderInfo; 

	// @todo utility functions to get defines from these 
	if ((lwFlags & e2::TerrainFlags::Normal) == e2::TerrainFlags::Normal)
		shaderInfo.defines.push({ "Vertex_Normals", "1" });

	if ((lwFlags & e2::TerrainFlags::TexCoords01) == e2::TerrainFlags::TexCoords01)
		shaderInfo.defines.push({ "Vertex_TexCoords01", "1" });

	if ((lwFlags & e2::TerrainFlags::TexCoords23) == e2::TerrainFlags::TexCoords23)
		shaderInfo.defines.push({ "Vertex_TexCoords23", "1" });

	if ((lwFlags & e2::TerrainFlags::Color) == e2::TerrainFlags::Color)
		shaderInfo.defines.push({ "Vertex_Color", "1" });

	if ((lwFlags & e2::TerrainFlags::Bones) == e2::TerrainFlags::Bones)
		shaderInfo.defines.push({ "Vertex_Bones", "1" });

	if ((lwFlags & e2::TerrainFlags::Shadow) == e2::TerrainFlags::Shadow)
		shaderInfo.defines.push({ "Renderer_Shadow", "1" });

	if ((lwFlags & e2::TerrainFlags::Skin) == e2::TerrainFlags::Skin)
		shaderInfo.defines.push({ "Renderer_Skin", "1" });

	if ((lwFlags & e2::TerrainFlags::Albedo) == e2::TerrainFlags::Albedo)
		shaderInfo.defines.push({ "Material_Albedo", "1" });

	std::string vertexSource;
	if (!e2::readFileWithIncludes("shaders/terrain/terrain.vertex.glsl", vertexSource))
		LogError("broken distribution");

	shaderInfo.stage = ShaderStage::Vertex;
	shaderInfo.source = vertexSource.c_str();
	newEntry.vertexShader = renderContext()->createShader(shaderInfo);

	std::string fragmentSource;
	if (!e2::readFileWithIncludes("shaders/terrain/terrain.fragment.glsl", fragmentSource))
		LogError("broken distribution");

	shaderInfo.stage = ShaderStage::Fragment;
	shaderInfo.source = fragmentSource.c_str();
	newEntry.fragmentShader = renderContext()->createShader(shaderInfo);

	e2::PipelineCreateInfo pipelineInfo;
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.shaders = { newEntry.vertexShader, newEntry.fragmentShader };
	pipelineInfo.colorFormats = { e2::TextureFormat::RGBA8, e2::TextureFormat::RGBA32 };
	pipelineInfo.depthFormat = { e2::TextureFormat::D32 };
	pipelineInfo.alphaBlending = true;
	newEntry.pipeline = renderContext()->createPipeline(pipelineInfo);

	m_pipelineCache[uint16_t(lwFlags)] = newEntry;
	return newEntry.pipeline;
}

void e2::TerrainModel::invalidatePipelines()
{
	for (uint16_t i = 0; i < uint16_t(e2::TerrainFlags::Count);i++)
	{
		e2::TerrainCacheEntry &entry = m_pipelineCache[i];

		if(entry.vertexShader)
			e2::discard(entry.vertexShader);
		entry.vertexShader = nullptr;

		if(entry.fragmentShader)
			e2::discard(entry.fragmentShader);
		entry.fragmentShader = nullptr;

		if(entry.pipeline)
			e2::discard(entry.pipeline);
		entry.pipeline = nullptr;
	}
}

e2::TerrainProxy::TerrainProxy(e2::Session* inSession, e2::MaterialPtr materialAsset)
	: e2::MaterialProxy(inSession, materialAsset)
{
	// @todo dont use gamesession here, it uses the default game session and thus this breaks in PIE etc.
	// instead take a sesson as parametr when creating proxies or similar
	session->registerMaterialProxy(this);
	model = static_cast<e2::TerrainModel*>(materialAsset->model());
	id = model->proxyIds.create();
}

e2::TerrainProxy::~TerrainProxy()
{
	session->unregisterMaterialProxy(this);
	model->proxyIds.destroy(id);

	sets[0]->keepAround();
	e2::destroy(sets[0]);

	sets[1]->keepAround();
	e2::destroy(sets[1]);
}

void e2::TerrainProxy::bind(e2::ICommandBuffer* buffer, uint8_t frameIndex)
{
	buffer->bindDescriptorSet(model->m_pipelineLayout, 2, sets[frameIndex]);
}

void e2::TerrainProxy::invalidate(uint8_t frameIndex)
{
	if (uniformData.invalidate(frameIndex))
	{
		uint32_t proxyOffset = renderManager()->paddedBufferSize(sizeof(TerrainData)) * id;
		model->m_proxyUniformBuffers[frameIndex]->upload(reinterpret_cast<uint8_t const*>(&uniformData.data()), sizeof(TerrainData), 0, proxyOffset);
	}

	if (albedoTexture.invalidate(frameIndex))
	{
		e2::ITexture* tex = albedoTexture.data();
		if (tex)
			sets[frameIndex]->writeTexture(1, tex);
	}
}
