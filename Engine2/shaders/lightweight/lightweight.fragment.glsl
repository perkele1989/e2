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

#include <shaders/lightweight/lightweight.common.glsl>

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


#if defined(Material_AlbedoTexture)
	vec4 albedoTexel = texture(sampler2D(albedoTexture, repeatSampler), uv).rgba;
#if defined(Material_AlphaClip)
	if(albedoTexel.a < 0.5)
		discard;
#endif 
	vec3 albedo = albedoTexel.rgb *  pow(material.albedo.rgb, vec3(1.0));// * 0.65;
	//albedo = mix(vec3(dot(vec3(1.0/3.0), albedo)), albedo, 0.5);

#else
	vec3 albedo = pow(material.albedo.rgb, vec3(1.0));

#endif 

	vec3 emissive = pow(material.emissive.rgb, vec3(1.0)) * material.emissive.a;

#if defined(Vertex_Color)
	//albedo.rgb *= pow(fragmentColor.rgb, vec3(2.2));
#endif


#if defined(Material_RoughnessTexture)
	float roughness = texture(sampler2D(roughnessTexture, repeatSampler), uv).r;
#else
	float roughness = material.rmxx.x;
	//roughness = 0.4;
#endif 

#if defined(Material_MetalnessTexture)
	float metalness = texture(sampler2D(roughnessTexture, repeatSampler), uv).r;
#else
	float metalness = material.rmxx.y;
#endif 

#if defined(Vertex_Normals)

#if defined(Material_NormalTexture)

	vec3 n = normalize(fragmentNormal.xyz);
	vec3 t = normalize(fragmentTangent.xyz);
	vec3 b = fragmentTangent.w * cross(n, t);
	vec3 texelNormal = normalize(texture(sampler2D(normalTexture, repeatSampler), uv).xyz * 2.0 - 1.0);
	//texelNormal.y = -texelNormal.y;
	vec3 worldNormal = normalize(texelNormal.x * t + texelNormal.y * b + texelNormal.z * fragmentNormal);
#else 
	vec3 worldNormal = normalize(fragmentNormal);
#endif

	vec3 viewVector= getViewVector(fragmentPosition.xyz);

	outColor.rgb = vec3(0.0, 0.0, 0.0);
	outColor.rgb += getIblColor(fragmentPosition.xyz, albedo, worldNormal, roughness, metalness, viewVector);
	outColor.rgb += getSunColor(fragmentPosition.xyz, worldNormal, albedo);
	outColor.rgb *= getCloudShadows(fragmentPosition.xyz);
	//outColor.rgb = mix(outColor.rgb, albedo, 0.95);
	//outColor.rgb += outColor.rgb * getRimColor(worldNormal, viewVector, vec3(1.0, 1.0, 1.0));
	outColor.rgb += emissive;
#else 
	outColor.rgb = albedo + emissive;
#endif

	// debug refl
	//outColor.rgb = F;
	// debug norm
	//outColor.rgb = clamp(vec3(worldNormal.x, worldNormal.z, -worldNormal.y) * 0.5 + 0.5, vec3(EPSILON), vec3(1.0));
    //outColor.rgb = clamp(vec3(fragmentNormal.x, fragmentNormal.z, -fragmentNormal.y) * 0.5 + 0.5, vec3(EPSILON), vec3(1.0));
	// debug ndotl 
	//outColor.rgb = vec3( max(0.0, dot(fragmentNormal, normalize(vec3(1.0, -1.0, 1.0)) )));
	//outColor.rgb = albedo;

//#else
//#if defined(Material_AlbedoTexture) && defined(Material_AlphaClip)
//	float alpha = texture(sampler2D(albedoTexture, repeatSampler), uv).a;
//	if(alpha < 0.5)
//		discard;
//#endif
#endif
}