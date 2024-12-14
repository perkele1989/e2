#include <shaders/header.glsl>

// Fragment attributes

#if !defined(Renderer_Shadow)
in vec4 fragmentPosition;

#if defined(Vertex_TexCoords01)
in vec4 fragmentUv01;
#endif

#if defined(Vertex_TexCoords23)
in vec4 fragmentUv23;
#endif 

#if defined(Vertex_Normals)
in vec3 fragmentNormal;
in vec4 fragmentTangent;
#endif

#if defined(Vertex_Color)
in vec4 fragmentColor;
#endif

// Out color
out vec4 outColor;
out vec4 outPosition;

#endif

#include <shaders/common/renderersets.glsl>

%CustomDescriptorSets%

void main()
{
#if !defined(Renderer_Shadow)
	outPosition = fragmentPosition;
	outColor = vec4(1.0, 1.0, 1.0, 1.0);

#if defined(Vertex_TexCoords01)
	vec2 uv = fragmentUv01.xy;
	//uv.y = 1.0 - uv.y;
#else 
	vec2 uv = fragmentPosition.xz;
#endif 


#if defined(HasCustomTexture_albedoMap)
	vec4 albedoTexel = texture(sampler2D(customTexture_albedoMap, repeatSampler), uv).rgba;
	vec3 albedo = albedoTexel.rgb *  material.albedo.rgb;
#else
	vec3 albedo = pow(material.albedo.rgb, vec3(1.0));
#endif 

	albedo.rgb *= 0.5 + 0.5* (1.0 - smoothstep(-0.15, 0.0, fragmentPosition.y));

	vec3 emissive = pow(material.emissive.rgb, vec3(1.0)) * material.emissive.a;

#if defined(Vertex_Color) && !defined(HasCustomTexture_albedoMap)
	albedo.rgb *= pow(fragmentColor.rgb, vec3(2.2));
#endif


#if defined(HasCustomTexture_roughnessMap)
	float roughness = texture(sampler2D(customTexture_roughnessMap, repeatSampler), uv).r;
#else
	float roughness = material.rmxx.x;
#endif 

#if defined(HasCustomTexture_metalnessMap)
	float metalness = texture(sampler2D(customTexture_metalnessMap, repeatSampler), uv).r;
#else
	float metalness = material.rmxx.y;
#endif 

#if defined(Vertex_Normals)

	vec3 worldNormal = normalize(fragmentNormal);
	vec3 viewVector= getViewVector(fragmentPosition.xyz);

	float cloudShadows =  getGrassCloudShadows(fragmentPosition.xyz);
	cloudShadows *= getCloudShadows(fragmentPosition.xyz);

	outColor.rgb = vec3(0.0, 0.0, 0.0);

	outColor.rgb += getIblColor(fragmentPosition.xyz, albedo, worldNormal, roughness, metalness, viewVector) * cloudShadows * 0.5;
	outColor.rgb += getIblColor(fragmentPosition.xyz, albedo, -worldNormal, roughness, metalness, viewVector) * cloudShadows *  0.5;
	outColor.rgb += getSunColor(fragmentPosition.xyz, worldNormal, albedo, roughness, metalness, viewVector) * cloudShadows * 0.5;
	outColor.rgb += getSunColor(fragmentPosition.xyz, -worldNormal, albedo, roughness, metalness, viewVector) * cloudShadows * 0.5;


	// outColor.rgb = vec3(cloudShadows);
	// return;
	// outColor.rgb = worldNormal * 0.5 + 0.5;
	// return;
	//outColor.rgb += albedo * getRimColor(worldNormal, viewVector, renderer.sun2.xyz * renderer.sun2.w * 8.0) ;



	// float ss2 = sampleSimplex(fragmentPosition.xz * 10.5);
	// outColor.rgb = vec3(ss2);
	// return;

	outColor.rgb += emissive;
#else 
	outColor.rgb = vec3(0.0, 0.0, 0.0);//albedo + emissive;
#endif


#endif
}