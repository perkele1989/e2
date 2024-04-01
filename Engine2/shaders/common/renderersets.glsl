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
    vec4 sun1; // sun dir.xyz, ???
    vec4 sun2; // sun color.rgb, sun strength
    vec4 ibl1; // ibl strength, ??, ?? ,??
    vec4 cameraPosition; // pos.xyz, ??
} renderer;

layout(set = 0, binding = 1) uniform sampler clampSampler;
layout(set = 0, binding = 2) uniform sampler repeatSampler;
layout(set = 0, binding = 3) uniform sampler equirectSampler;
layout(set = 0, binding = 4) uniform texture2D integratedBrdf;

layout(set = 0, binding = 5) uniform texture2D irradianceCube;
layout(set = 0, binding = 6) uniform texture2D radianceCube;

layout(set = 0, binding = 7) uniform texture2D frontBufferColor;
layout(set = 0, binding = 8) uniform texture2D frontBufferPosition;
layout(set = 0, binding = 9) uniform texture2D frontBufferDepth;
layout(set = 0, binding = 10) uniform texture2D outlineTexture;
// End Set0

// Begin Set1: Mesh 
layout(set = 1, binding = 0) uniform MeshData 
{
    mat4 modelMatrix;
} mesh;

layout(set = 1, binding = 1) uniform SkinData 
{
    mat4 skinMatrices[128]; // @todo make define
} skin;
// End Set1


#include <shaders/common/utils.glsl>

// specific util functions depending on renderer sets here! 

vec3 getViewVector(vec3 fragPosition)
{
    vec3 viewVector = normalize(fragPosition - renderer.cameraPosition.xyz);
    return viewVector;
}

vec3 getSunColor(vec3 fragNormal, vec3 fragAlbedo)
{
    vec3 ndotl = vec3(clamp(dot(fragNormal, renderer.sun1.xyz), 0.0, 1.0));
    return ndotl * renderer.sun2.xyz * renderer.sun2.w * fragAlbedo;
}

vec3 getIblColor(vec3 fragPosition, vec3 fragAlbedo, vec3 fragNormal,float fragRoughness, float fragMetalness, vec3 viewVector)
{
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