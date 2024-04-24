

#if defined(Renderer_Shadow)
#define MeshSetIndex 0
#else 
#define RendererSetIndex 0
#define MeshSetIndex 1
#define MaterialSetIndex 2
#endif 

#if !defined(Renderer_Shadow)
// Push constants
layout(push_constant) uniform ConstantData
{
    mat4 normalMatrix;
    uvec2 resolution;
};

// Begin Set0: Renderer
layout(set = RendererSetIndex, binding = 0) uniform RendererData
{
    mat4 shadowView;
    mat4 shadowProjection;
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec4 time; // t, sin(t), cos(t), tan(t)
    vec4 sun1; // sun dir.xyz, ???
    vec4 sun2; // sun color.rgb, sun strength
    vec4 ibl1; // ibl strength, ??, ?? ,??
    vec4 cameraPosition; // pos.xyz, ??
} renderer;

layout(set = RendererSetIndex, binding = 1) uniform sampler clampSampler;
layout(set = RendererSetIndex, binding = 2) uniform sampler repeatSampler;
layout(set = RendererSetIndex, binding = 3) uniform sampler equirectSampler;
layout(set = RendererSetIndex, binding = 4) uniform texture2D integratedBrdf;
layout(set = RendererSetIndex, binding = 5) uniform texture2D irradianceCube;
layout(set = RendererSetIndex, binding = 6) uniform texture2D radianceCube;
layout(set = RendererSetIndex, binding = 7) uniform texture2D frontBufferColor;
layout(set = RendererSetIndex, binding = 8) uniform texture2D frontBufferPosition;
layout(set = RendererSetIndex, binding = 9) uniform texture2D frontBufferDepth;
layout(set = RendererSetIndex, binding = 10) uniform texture2D outlineTexture;
layout(set = RendererSetIndex, binding = 11) uniform texture2D shadowMap;
layout(set = RendererSetIndex, binding = 12) uniform sampler shadowSampler;
// End Set0
#else 
// Begin Set0: Shadow Renderer
// layout(set = 0, binding = 0) uniform ShadowRendererData
// {
//     mat4 shadowView;
//     mat4 shadowProjection;
// } shadowRenderer;

// Push constants
layout(push_constant) uniform ShadowConstantData
{
    mat4 shadowViewProjection;
};
// End Set0
#endif


// Begin Set1: Mesh 
layout(set = MeshSetIndex, binding = 0) uniform MeshData 
{
    mat4 modelMatrix;
} mesh;

layout(set = MeshSetIndex, binding = 1) uniform SkinData 
{
    mat4 skinMatrices[128]; // @todo make define
} skin;
// End Set1


#include <shaders/common/utils.glsl>

// specific util functions depending on renderer sets here! 
#if !defined(Renderer_Shadow)
vec3 getViewVector(vec3 fragPosition)
{
    vec3 viewVector = normalize(fragPosition - renderer.cameraPosition.xyz);
    return viewVector;
}

float calculateSunShadow(vec4 fragPosWS)
{
	float returner = 0.0;

	// Multiply this position by the VPMatrix used by the shadow-map-rendering. Also, normalize the perspective by dividing by w
	vec4 fragPosLS = renderer.shadowProjection * renderer.shadowView * fragPosWS; 
	fragPosLS.xyz /= fragPosLS.w;
	
	// Convert these coordinates from (-1 to 1) range to  (0 to 1) range
	vec2 shadowUV = fragPosLS.xy * vec2(0.5) + vec2(0.5);

	// Use the (biased) z coordinate as the comparative depth to pass to the texture sampling function.
	const float bias = 0.002;
	float depthComp = (fragPosLS.z) - bias;
	
	// If the shadow UV is out of bounds, return 1.0. Needed for directional lights
	if(shadowUV.x < 0 || shadowUV.y < 0 ||shadowUV.x > 1 ||shadowUV.y > 1)
	{
		return 1.0;
	}

	// Prepare sampling
	

	// singletap
	//return texture( sampler2DShadow(shadowMap, shadowSampler), vec3(shadowUV.x, shadowUV.y, depthComp));

	// 9-tap shadow sampling
	const int SHADOWSAMPLES = 9;
	vec2 sampleOffsets[SHADOWSAMPLES] = vec2[]
	(
	vec2(-1.0, -1.0), vec2(0.0, -1.0), vec2(1.0, -1.0),
	vec2(-1.0, 0.0), vec2(0.0, 0.0), vec2(1.0, 0.0),
	vec2(-1.0, 1.0), vec2(0.0, 1.0), vec2(1.0, 1.0)
	);

	ivec2 shadowSize = textureSize(shadowMap, 0);
	vec2 shadowFragSize = vec2(1.0) / vec2(shadowSize.x, shadowSize.y);

	
	for(int i = 0; i < SHADOWSAMPLES; i++)
	{
		vec2 offsetUV = shadowUV + (sampleOffsets[i] * shadowFragSize);
		float shadowDepth = texture(sampler2DShadow(shadowMap, shadowSampler), vec3(offsetUV.x, offsetUV.y, depthComp));
		returner += shadowDepth;
	}
	returner /= float(SHADOWSAMPLES);
	
	return returner;
}

vec3 getSunColor(vec3 fragPos, vec3 fragNormal, vec3 fragAlbedo)
{
    float shadow = calculateSunShadow(vec4(fragPos, 1.0));
    vec3 ndotl = vec3(clamp(dot(fragNormal, -renderer.sun1.xyz), 0.0, 1.0));
    return ndotl * renderer.sun2.xyz * renderer.sun2.w * fragAlbedo * shadow;
}

vec3 getIblColor(vec3 fragPosition, vec3 fragAlbedo, vec3 fragNormal,float fragRoughness, float fragMetalness, vec3 viewVector)
{
    // 0.2
    vec3 reflectionVector = reflect(viewVector, fragNormal);
	 
	vec3 F0 = vec3(0.04); 
    vec3 specularCoeff = mix(F0, fragAlbedo, vec3(fragMetalness)); 
	vec3 diffuseCoeff = fragAlbedo * (1.0 - F0) * (1.0 - fragMetalness);

	vec3 irradiance = texture(sampler2D(irradianceCube, equirectSampler), equirectangularUv(fragNormal)).rgb;
	vec3 radiance = textureLod(sampler2D(radianceCube, equirectSampler), equirectangularUv(reflectionVector), fragRoughness * 6.0).rgb;
	vec2 brdf = textureLod(sampler2D(integratedBrdf, clampSampler), vec2(clamp(dot(fragNormal, -viewVector), 0.0, 1.0), 1.0 - fragRoughness), 0.0).xy;
		
    vec3 returner = vec3(0.0, 0.0, 0.0);
	returner += irradiance * diffuseCoeff;
    returner += radiance * specularCoeff * (brdf.x + brdf.y);	

    returner *= renderer.ibl1.x;

    return returner;
}

vec3 getRimColor(vec3 fragNormal, vec3 viewVector, vec3 rimLight)
{
    vec3 flatViewVector = viewVector;
	flatViewVector.y = 0.0;
	flatViewVector = normalize(flatViewVector);
	float rimCoeff = max(0.0, pow(dot(flatViewVector, fragNormal), 1.0 / 16.0));
    return rimLight * rimCoeff; 
}

float getCloudShadows(vec3 fragPosition)
{
	float shadowSimplex = (simplex((fragPosition.xz * 0.1) - vec2(0.4, 0.6) * renderer.time.x * 0.05 ) * 0.5 + 0.5);
	float shadowCoeff = pow(shadowSimplex, 0.62);
	shadowCoeff = smoothstep(0.4, 0.7, shadowCoeff) * 0.5 + 0.5;
    return shadowCoeff;
}
#endif