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
	uv.y = 1.0 - uv.y;
#else 
	fragmentPosition.xz;
#endif 


#if defined(HasCustomTexture_albedoMap)
	vec4 albedoTexel = texture(sampler2D(customTexture_albedoMap, repeatSampler), uv).rgba;
	vec3 albedo = albedoTexel.rgb *  material.albedo.rgb;
#else
	vec3 albedo = pow(material.albedo.rgb, vec3(1.0));
#endif 

	float ss = sampleSimplex(fragmentPosition.xz*1.0);

    // vec3 grassAlbedo = vec3(0.231, 0.471, 0.02)*0.3;
	// grassAlbedo.rgb *= 0.5 + 0.5* (1.0 - smoothstep(-0.15, 0.0, fragmentPosition.y));
	// grassAlbedo = mix(vec3(dot(vec3(1.0/3.0), grassAlbedo)), grassAlbedo, 0.955);
	// grassAlbedo.rgb = shiftHue(grassAlbedo.rgb, ss*-0.12);


	albedo.rgb *= 0.55;
	albedo.rgb = shiftHue(albedo.rgb, mix(-0.45, -0.2, 1.0-ss)*1.75);
	albedo = mix(vec3(dot(vec3(1.0/3.0), albedo)), albedo, 0.925);
	//albedo = pow(albedo, vec3(mix(1.0, 1.13, smoothstep(-0.7, 0.0, fragmentPosition.y))));
	albedo.rgb *= 0.5 + 0.5* (1.0 - smoothstep(-0.15, 0.0, fragmentPosition.y));

	// albedo = mix(albedo, grassAlbedo, 0.22);

	vec3 emissive = pow(material.emissive.rgb, vec3(1.0)) * material.emissive.a;

#if defined(Vertex_Color)
	//albedo.rgb *= pow(fragmentColor.rgb, vec3(2.2));
#endif


#if defined(HasCustomTexture_roughnessMap)
	float roughness = texture(sampler2D(customTexture_roughnessMap, repeatSampler), uv).r;
#else
	float roughness = material.rmxx.x;
#endif 

roughness = 0.75;

#if defined(HasCustomTexture_metalnessMap)
	float metalness = texture(sampler2D(customTexture_metalnessMap, repeatSampler), uv).r;
#else
	float metalness = material.rmxx.y;
#endif 

#if defined(Vertex_Normals)

#if defined(HasCustomTexture_normalMap)

	vec3 n = normalize(fragmentNormal.xyz);
	vec3 t = normalize(fragmentTangent.xyz);
	vec3 b = fragmentTangent.w * cross(n, t);
	vec3 texelNormal = normalize(texture(sampler2D(customTexture_normalMap, repeatSampler), uv).xyz * 2.0 - 1.0);
	vec3 worldNormal = normalize(texelNormal.x * t + texelNormal.y * b + texelNormal.z * fragmentNormal);
#else 
	vec3 worldNormal = normalize(fragmentNormal);
#endif

	vec3 viewVector= getViewVector(fragmentPosition.xyz);

	float shadows = getCloudShadows(fragmentPosition.xyz);

	outColor.rgb = vec3(0.0, 0.0, 0.0);
	outColor.rgb += getIblColor(fragmentPosition.xyz, albedo, worldNormal, roughness, metalness, viewVector) * shadows;
	//outColor.rgb += getIblColor(fragmentPosition.xyz, albedo, -worldNormal, roughness, metalness, viewVector)*0.15;
	outColor.rgb += getSunColor(fragmentPosition.xyz, worldNormal, albedo, roughness, metalness, viewVector) * shadows;


	// outColor.rgb += albedo * getRimColor(worldNormal, viewVector, renderer.sun2.xyz * renderer.sun2.w * 4.0) ;


	outColor.rgb += emissive;
#else 
	outColor.rgb = albedo + emissive;
#endif


#endif
}