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

#include <shaders/common/utils.glsl>
#include <shaders/common/procgen.glsl>
#include <shaders/terrain/terrain.common.glsl>

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
	gridCoeff2 *= 0.15;
	outColor.rgb = mix(outColor.rgb, vec3(0.0), gridCoeff2);

}