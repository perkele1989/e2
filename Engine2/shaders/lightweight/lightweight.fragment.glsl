#version 460 core

// Fragment attributes

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

#include <shaders/common/utils.glsl>
#include <shaders/lightweight/lightweight.common.glsl>

void main()
{
	vec3 worldLight = normalize(vec3(1.0, -1.0, 1.0));
	vec3 worldView = normalize(fragmentPosition.xyz - (inverse(renderer.viewMatrix) * vec4(0.0, 0.0, 0.0, 1.0)).xyz);

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
	vec3 albedo = albedoTexel.rgb;

#else
	vec3 albedo = material.albedo.rgb;
#endif 

#if defined(Material_RoughnessTexture)
	float roughness = texture(sampler2D(roughnessTexture, repeatSampler), uv).r;
#else
	float roughness = material.rmxx.x;
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
	vec3 texelNormal = normalize(texture(sampler2D(normalTexture, repeatSampler), fragmentUv01.xy).xyz * 2.0 - 1.0);
	texelNormal.y = -texelNormal.y;
	vec3 worldNormal = normalize(texelNormal.x * t + texelNormal.y * b + texelNormal.z * fragmentNormal);
#else 
	vec3 worldNormal = normalize(fragmentNormal);
#endif

	metalness = 0.0;
	roughness = 0.0;

	vec3 ndotl = vec3(clamp(dot(worldNormal, worldLight), EPSILON, 1.0));
	vec3 worldReflect = reflect(worldView, worldNormal);

	float brdfy = clamp(dot(worldNormal, -worldView), EPSILON, 1.0);
	vec3 F0 = vec3(0.04); 
    vec3 specularCoeff = mix(F0, albedo, metalness); 
	vec3 diffuseCoeff = albedo * (1.0 - F0) * (1.0 - metalness);
	 //diffuseCoeff =  (1.0 - F0) * (1.0 - metalness);
	vec3 irr = texture(sampler2D(irradianceCube, equirectSampler), equirectangularUv(worldNormal)).rgb;
	vec3 rad = textureLod(sampler2D(radianceCube, equirectSampler), equirectangularUv(worldReflect), roughness * 6.0).rgb;
	vec2 brdf = textureLod(sampler2D(integratedBrdf, clampSampler), vec2(brdfy, 1.0 - roughness), 0.0).xy;

	outColor.rgb = vec3(0.0, 0.0, 0.0);

	// ibl specular
	outColor.rgb += rad * specularCoeff *  (brdf.x + brdf.y) ;	

	// ibl diffuse 
	outColor.rgb += diffuseCoeff * irr;

	// ndotl 
	outColor.rgb += albedo * vec3(0.76, 0.8, 1.0) * ndotl;

	// fresnel
	//outColor.rgb += vec3(1.0, 0.8, 0.76) * mountainFresnel * 0.30; 

#else 
	outColor.rgb = albedo;
#endif


	// debug refl
	//outColor.rgb = F;

	// debug norm
	//outColor.rgb = clamp(vec3(worldNormal.x, worldNormal.z, -worldNormal.y) * 0.5 + 0.5, vec3(EPSILON), vec3(1.0));
    //outColor.rgb = clamp(vec3(fragmentNormal.x, fragmentNormal.z, -fragmentNormal.y) * 0.5 + 0.5, vec3(EPSILON), vec3(1.0));

	//outColor.rgb = vec3(brdf.g);

	// debug ndotl 
	//outColor.rgb = ndotl;

    
    //outColor.rgb = fogOfWar(outColor.rgb, fragmentPosition.xyz, visibility, renderer.time.x);

}