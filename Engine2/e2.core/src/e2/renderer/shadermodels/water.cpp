
#include "e2/renderer/shadermodels/water.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/game/gamesession.hpp"
#include "e2/assets/mesh.hpp"
#include "e2/assets/material.hpp"
#include "e2/renderer/meshproxy.hpp"

#include <e2/rhi/threadcontext.hpp>

#include "e2/utils.hpp"

static char const* commonSource = R"SRC(

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

	const mat2 myt = mat2(.12121212, .13131313, -.13131313, .12121212);
	const vec2 mys = vec2(1e4, 1e6);

	vec2 rhash(vec2 uv) {
	  uv *= myt;
	  uv *= mys;
	  return fract(fract(uv / mys) * uv);
	}

	vec3 hash(vec3 p) {
	  return fract(sin(vec3(dot(p, vec3(1.0, 57.0, 113.0)),
							dot(p, vec3(57.0, 113.0, 1.0)),
							dot(p, vec3(113.0, 1.0, 57.0)))) *
				   43758.5453);
	}

	float voronoi2d(const in vec2 point) {
	  vec2 p = floor(point);
	  vec2 f = fract(point);
	  float res = 0.0;
	  for (int j = -1; j <= 1; j++) {
		for (int i = -1; i <= 1; i++) {
		  vec2 b = vec2(i, j);
		  vec2 r = vec2(b) - f + rhash(p + b);
		  res += 1. / pow(dot(r, r), 8.);
		}
	  }
	  return pow(1. / res, 0.0625);
	}


	float sampleHeight(vec2 position)
	{
		return pow(voronoi2d(position), 2.0);
	}

	vec3 sampleNormal(vec2 position)
	{
		float eps = 0.1;
		float eps2 = eps * 2;
		vec3 off = vec3(1.0, 1.0, 0.0)* eps;
		float hL = sampleHeight(position.xy - off.xz);
		float hR = sampleHeight(position.xy + off.xz);
		float hD = sampleHeight(position.xy - off.zy);
		float hU = sampleHeight(position.xy + off.zy);

		return normalize(vec3(hL - hR, -eps2, hD - hU));
	}

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



	float sampleWaterDepth(vec2 position)
	{
		float baseHeight = sampleBaseHeight(position);

		float depthCoeff = smoothstep(0.03, 0.39, baseHeight);

		return 1.0 - depthCoeff;
	}


	float sampleWaterHeight(vec2 position)
	{
		float depthCoeff = sampleWaterDepth(position);

		float variance1 = 0.2;
		float varianceOffset1 = 1.0 - variance1;
		float varianceSpeed1 = 3.0;
		float waveCoeff1 = (sin(renderer.time.x * varianceSpeed1) * 0.5 + 0.5) * variance1 + varianceOffset1;

		float variance2 = 0.5;
		float varianceOffset2 = 1.0 - variance2;
		float varianceSpeed2 = 2.5;
		float waveCoeff2 = (sin(renderer.time.x * varianceSpeed2) * 0.5 + 0.5) * variance2 + varianceOffset2;

		float speed1 = 0.5;
		vec2 dir1 = normalize(vec2(0.2, 0.4));
		vec2 pos1 = position + (dir1 * speed1 * renderer.time.x);
		float height1 = sampleHeight(pos1 * 2.0) * waveCoeff1;

		float speed2 = 0.3;
		vec2 dir2 = normalize(vec2(-0.22, -0.3));
		vec2 pos2 = position + (dir2 * speed2 * renderer.time.x);
		float height2 = sampleHeight(pos2) *  waveCoeff2;

		float weight1 = 0.36;
		float weight2 = (1.0 - weight1)* depthCoeff;

		float multiplier = mix(0.5, 1.0, depthCoeff);
 
		return (height1 * weight1 + height2 * weight2) * multiplier;
	}

	vec3 sampleWaterNormal(vec2 position)
	{
		float eps = 0.1;
		float eps2 = eps * 2;
		vec3 off = vec3(1.0, 1.0, 0.0)* eps;
		float hL = sampleWaterHeight(position.xy - off.xz);
		float hR = sampleWaterHeight(position.xy + off.xz);
		float hD = sampleWaterHeight(position.xy - off.zy);
		float hU = sampleWaterHeight(position.xy + off.zy);

		return normalize(vec3(hR - hL, -eps2 * 0.2, hU - hD));
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
	out vec4 fragmentPosition2;

	out vec3 fragmentNormal;
)SRC";

static char const* vertexSource = R"SRC(
void main()
{
	vec4 vertexWorld = mesh.modelMatrix * vertexPosition;
	vec4 waterPosition = vertexPosition;
	waterPosition.y -= sampleWaterHeight(vertexWorld.xz) * 0.24;

	fragmentNormal = sampleWaterNormal(vertexWorld.xz);

	gl_Position = renderer.projectionMatrix * renderer.viewMatrix * mesh.modelMatrix * waterPosition;

	// Write fragment attributes (in worldspace where applicable)
	// @todo skinned 
	fragmentPosition = mesh.modelMatrix * waterPosition;
	fragmentPosition2 = mesh.modelMatrix * vertexPosition;
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

static char const* fragmentHeader = R"SRC(
	#version 460 core

	// Fragment attributes

	in vec4 fragmentPosition;
	in vec4 fragmentPosition2;

	in vec3 fragmentNormal;

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
void main()
{
	outPosition = fragmentPosition;
	outColor = vec4(1.0, 1.0, 1.0, 1.0);

	float h = sampleWaterHeight(fragmentPosition.xz);
	float d = sampleWaterDepth(fragmentPosition.xz);

	vec3 n = sampleWaterNormal(fragmentPosition.xz);
	
	vec3 l = normalize(vec3(-1.0, -1.0, -1.0));
	vec3 ndotl = vec3(clamp(dot(n, l), 0.0, 1.0));
	//vec3 softl = vec3(dot(n,l) *0.5 + 0.5);

	vec3 v = normalize(fragmentPosition.xyz - (inverse(renderer.viewMatrix) * vec4(0.0, 0.0, 0.0, 1.0)).xyz);
	float vdotn = pow(clamp(-dot(v, n), 0.0, 1.0), 4.0);

	vec3 darkWater = vec3(0.0, 80.0, 107.0) / 255.0;
	vec3 lightWater = vec3(28.0, 255.0, 255.0) / 255.0;
	vec3 superLightWater = vec3(1.0, 1.0 ,1.0);//pow(lightWater, vec3(110.1));

	vec3 shoreDark  = pow(darkWater, vec3(0.6));
	vec3 oceanDark  = pow(darkWater, vec3(2.4));

	vec3 finalDark = mix(shoreDark, oceanDark, d);
	vec3 finalLight = mix(pow(lightWater, vec3(0.4)), lightWater, d);

	//vec3 finalLight = lightWater;

	vec3 r = reflect(v, n);

	vec3 hdr = texture(sampler2D(albedoTexture, albedoSampler), equirectangularUv(r)).rgb;


	vec3 frontBuffer = texture(sampler2D(frontBufferColor, frontBufferSampler), gl_FragCoord.xy / vec2(resolution.x, resolution.y)).rgb;
	vec3 frontPosition = texture(sampler2D(frontBufferPosition, frontBufferSampler), gl_FragCoord.xy / vec2(resolution.x, resolution.y)).xyz;


	float viewDistanceToDepth = distance(frontPosition, fragmentPosition.xyz);
	float viewDepthCoeff = 1.0 - smoothstep(0.0, 0.2, viewDistanceToDepth);
	float viewDepthCoeff2 = 1.0 - smoothstep(0.0, 1.0, viewDistanceToDepth);

	float timeSin = (sin(renderer.time.x) * 0.5 + 0.5);
	float timeSin2 = 1.0 - (sin( (renderer.time.x + timeSin * 0.3) * 1.2) * 0.5 + 0.5);

	float viewDepthCoeffFoam = 1.0 - smoothstep(0.0,  (0.33 + timeSin2 * 0.6 ) * pow((simplex(fragmentPosition.xz * 4.0 + vec2(cos(renderer.time.x * 0.25), sin(renderer.time.x*0.2))) * 0.5 + 0.5), 1.0), viewDistanceToDepth);

	vec3 baseColor = mix(finalDark, finalLight,  pow(h, 1.2) * (ndotl * 0.5 + 0.5) );
	vec3 dimBaseColor = mix(baseColor * frontBuffer, frontBuffer, 0.05);
	baseColor = mix(baseColor, dimBaseColor, viewDepthCoeff2);
	//baseColor = mix(baseColor, frontBuffer, viewDepthCoeff);
	
	vec3 foamColor = vec3(0.867, 0.89, 0.9);
	baseColor = mix(baseColor, foamColor, (pow(viewDepthCoeffFoam, 0.25)*0.75));

	outColor.rgb =vec3(0.0, 0.0, 0.0);

	// basecolor
	outColor.rgb +=  baseColor;

	// fresnel reflection
	outColor.rgb += hdr * vdotn * 0.15;

	// reflection 
	outColor.rgb += hdr * 0.05;


	// debug normal 
	//outColor.rgb = clamp(vec3(n.x, n.z, -n.y) * 0.5 + 0.5, vec3(0.0), vec3(1.0));

	//outColor.rgb = vec3(foamCoeff);

	outColor.rgb = clamp(outColor.rgb, vec3(0.0), vec3(1.0));
	outColor.a = 1.0; //clamp(pow( smoothstep(0.4, 0.5, d), 16.0), 0.0, 1.0);// * 0.2 + 0.8;

}
)SRC";

e2::WaterModel::WaterModel()
	: e2::ShaderModel()
{
	m_pipelineCache.resize(uint16_t(e2::WaterFlags::Count));
	for (uint16_t i = 0; i < uint16_t(e2::WaterFlags::Count); i++)
	{
		m_pipelineCache[i] = {};
	}
}

e2::WaterModel::~WaterModel()
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

void e2::WaterModel::postConstruct(e2::Context* ctx)
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
	poolCreateInfo.maxSets = e2::maxNumWaterProxies * 2;
	poolCreateInfo.numTextures = e2::maxNumWaterProxies * 2 * 1;
	poolCreateInfo.numSamplers = e2::maxNumWaterProxies * 2 * 1;
	poolCreateInfo.numUniformBuffers = e2::maxNumWaterProxies * 2 * 1;
	m_descriptorPool = mainThreadContext()->createDescriptorPool(poolCreateInfo);


	e2::DataBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.dynamic = true;
	bufferCreateInfo.size = renderManager()->paddedBufferSize(e2::maxNumWaterProxies * sizeof(e2::WaterData));
	bufferCreateInfo.type = BufferType::UniformBuffer;
	m_proxyUniformBuffers[0] = renderContext()->createDataBuffer(bufferCreateInfo);
	m_proxyUniformBuffers[1] = renderContext()->createDataBuffer(bufferCreateInfo);

	e2::SamplerCreateInfo samplerInfo{};
	samplerInfo.filter = SamplerFilter::Anisotropic;
	samplerInfo.wrap = SamplerWrap::Clamp;
	m_sampler = renderContext()->createSampler(samplerInfo);

	//std::string cubemapName = "assets/lakeside_4k.e2a";
	//std::string cubemapName = "assets/the_sky_is_on_fire_4k.e2a";
	//std::string cubemapName = "assets/studio_small_03_4k.e2a";
	std::string cubemapName = "assets/kloofensky_rad.e2a";

	e2::ALJDescription aljDesc;
	assetManager()->prescribeALJ(aljDesc, cubemapName);
	assetManager()->queueWaitALJ(aljDesc);

	m_cubemap = assetManager()->get(cubemapName).cast<e2::Texture2D>();
}

e2::MaterialProxy* e2::WaterModel::createMaterialProxy(e2::Session* session, e2::MaterialPtr material)
{
	e2::WaterProxy* newProxy = e2::create<e2::WaterProxy>(session, material);

	for (uint8_t i = 0; i < 2; i++)
	{
		newProxy->sets[i] = m_descriptorPool->createDescriptorSet(m_descriptorSetLayout);
		newProxy->sets[i]->writeUniformBuffer(0, m_proxyUniformBuffers[i], sizeof(e2::WaterData), renderManager()->paddedBufferSize(sizeof(e2::WaterData)) * newProxy->id);
		newProxy->sets[i]->writeTexture(1, m_cubemap->handle());
		newProxy->sets[i]->writeSampler(2, m_sampler);
	}

	e2::WaterData newData;
	newData.albedo = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	newProxy->uniformData.set(newData);

	return newProxy;
}

e2::IPipelineLayout* e2::WaterModel::getOrCreatePipelineLayout(e2::MeshProxy* proxy, uint8_t submeshIndex)
{
	return m_pipelineLayout;
}

e2::IPipeline* e2::WaterModel::getOrCreatePipeline(e2::MeshProxy* proxy, uint8_t submeshIndex, e2::RendererFlags rendererFlags)
{
	e2::SubmeshSpecification const& spec = proxy->asset->specification(submeshIndex);
	e2::WaterProxy* lwProxy = static_cast<e2::WaterProxy*>(proxy->materialProxies[submeshIndex]);

	uint16_t geometryFlags = (uint16_t)spec.attributeFlags;

	uint16_t materialFlags = 0;
	if (lwProxy->albedoTexture.data())
		materialFlags |= uint16_t(e2::WaterFlags::Albedo);

	uint16_t lwFlagsInt = (geometryFlags << (uint16_t)e2::WaterFlags::VertexFlagsOffset)
		| (uint16_t(rendererFlags) << (uint16_t)e2::WaterFlags::RendererFlagsOffset)
		| (materialFlags);

	e2::WaterFlags lwFlags = (e2::WaterFlags)lwFlagsInt;

	if (m_pipelineCache[uint16_t(lwFlags)].pipeline)
	{
		return m_pipelineCache[uint16_t(lwFlags)].pipeline;
	}

	e2::WaterCacheEntry newEntry;

	e2::ShaderCreateInfo shaderInfo; 

	// @todo utility functions to get defines from these 
	if ((lwFlags & e2::WaterFlags::Normal) == e2::WaterFlags::Normal)
		shaderInfo.defines.push({ "Vertex_Normals", "1" });

	if ((lwFlags & e2::WaterFlags::TexCoords01) == e2::WaterFlags::TexCoords01)
		shaderInfo.defines.push({ "Vertex_TexCoords01", "1" });

	if ((lwFlags & e2::WaterFlags::TexCoords23) == e2::WaterFlags::TexCoords23)
		shaderInfo.defines.push({ "Vertex_TexCoords23", "1" });

	if ((lwFlags & e2::WaterFlags::Color) == e2::WaterFlags::Color)
		shaderInfo.defines.push({ "Vertex_Color", "1" });

	if ((lwFlags & e2::WaterFlags::Bones) == e2::WaterFlags::Bones)
		shaderInfo.defines.push({ "Vertex_Bones", "1" });

	if ((lwFlags & e2::WaterFlags::Shadow) == e2::WaterFlags::Shadow)
		shaderInfo.defines.push({ "Renderer_Shadow", "1" });

	if ((lwFlags & e2::WaterFlags::Skin) == e2::WaterFlags::Skin)
		shaderInfo.defines.push({ "Renderer_Skin", "1" });

	if ((lwFlags & e2::WaterFlags::Albedo) == e2::WaterFlags::Albedo)
		shaderInfo.defines.push({ "Material_Albedo", "1" });

	std::string vertexSource;
	if (!e2::readFileWithIncludes("shaders/water/water.vertex.glsl", vertexSource))
		LogError("broken distribution");

	shaderInfo.stage = ShaderStage::Vertex;
	shaderInfo.source = vertexSource.c_str();
	newEntry.vertexShader = renderContext()->createShader(shaderInfo);

	std::string fragmentSource;
	if (!e2::readFileWithIncludes("shaders/water/water.fragment.glsl", fragmentSource))
		LogError("broken distribution");

	shaderInfo.stage = ShaderStage::Fragment;
	shaderInfo.source = fragmentSource.c_str();
	newEntry.fragmentShader = renderContext()->createShader(shaderInfo);

	e2::PipelineCreateInfo pipelineInfo;
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.shaders = { newEntry.vertexShader, newEntry.fragmentShader };
	pipelineInfo.colorFormats = { e2::TextureFormat::RGBA8,  e2::TextureFormat::RGBA32 };
	pipelineInfo.depthFormat = { e2::TextureFormat::D32 };
	pipelineInfo.alphaBlending = true;
	newEntry.pipeline = renderContext()->createPipeline(pipelineInfo);

	m_pipelineCache[uint16_t(lwFlags)] = newEntry;
	return newEntry.pipeline;
}

void e2::WaterModel::invalidatePipelines()
{
	for (uint16_t i = 0; i < uint16_t(e2::WaterFlags::Count); i++)
	{
		e2::WaterCacheEntry& entry = m_pipelineCache[i];

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

e2::RenderLayer e2::WaterModel::renderLayer()
{
	return RenderLayer::Water;
}

e2::WaterProxy::WaterProxy(e2::Session* inSession, e2::MaterialPtr materialAsset)
	: e2::MaterialProxy(inSession, materialAsset)
{
	// @todo dont use gamesession here, it uses the default game session and thus this breaks in PIE etc.
	// instead take a sesson as parametr when creating proxies or similar
	session->registerMaterialProxy(this);
	model = static_cast<e2::WaterModel*>(materialAsset->model());
	id = model->proxyIds.create();
}

e2::WaterProxy::~WaterProxy()
{
	session->unregisterMaterialProxy(this);
	model->proxyIds.destroy(id);

	sets[0]->keepAround();
	e2::destroy(sets[0]);

	sets[1]->keepAround();
	e2::destroy(sets[1]);
}

void e2::WaterProxy::bind(e2::ICommandBuffer* buffer, uint8_t frameIndex)
{
	buffer->bindDescriptorSet(model->m_pipelineLayout, 2, sets[frameIndex]);
}

void e2::WaterProxy::invalidate(uint8_t frameIndex)
{
	if (uniformData.invalidate(frameIndex))
	{
		uint32_t proxyOffset = renderManager()->paddedBufferSize(sizeof(WaterData)) * id;
		model->m_proxyUniformBuffers[frameIndex]->upload(reinterpret_cast<uint8_t const*>(&uniformData.data()), sizeof(WaterData), 0, proxyOffset);
	}

	if (albedoTexture.invalidate(frameIndex))
	{
		e2::ITexture* tex = albedoTexture.data();
		if (tex)
			sets[frameIndex]->writeTexture(1, tex);
	}
}
