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
#include <shaders/terrain/terrain.common.glsl>

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, EPSILON, 1.0), 5.0);
}



void main()
{
	outPosition = fragmentPosition;
	outColor = vec4(1.0, 1.0, 1.0, 1.0);

	float texScaleMountains = 0.1;
	float texScaleFields = 0.5;
	float texScaleSand = 0.1;
	float texScaleGreen = 0.2;
    vec2 texUv = fragmentPosition.xz;
    texUv.y = -texUv.y;


	vec3 albedoSand = texture(sampler2D(sandAlbedo, repeatSampler), texUv * texScaleSand).rgb;
	//albedoSand *= vec3(1.0, 1.0, 1.0) * 0.8;
	
	vec3 albedoMountains = pow(texture(sampler2D(mountainAlbedo, repeatSampler), texUv * texScaleMountains).rgb, vec3(1.1));
	albedoMountains *= vec3(1.0, 1.0, 1.0) * 0.8;

	vec3 albedoGreen = texture(sampler2D(greenAlbedo, repeatSampler), texUv * texScaleGreen).rgb;
	albedoGreen *= vec3(1.0, 1.0, 1.0) * 0.85;

	float bigSimplex = simplex(fragmentPosition.xz * 6) * 0.5 + 0.5;
	float smallSimplex = simplex(fragmentPosition.xz * 2) * 0.5 + 0.5;

	float smallSimplexS = simplex(fragmentPosition.xz * 1.1) * 0.5 + 0.5;

	float mainHeight = sampleBaseHeight(fragmentPosition.xz);
	float greenCoeff = clamp(smoothstep(0.4 + 0.1 * smallSimplexS, 0.5+ 0.1 * smallSimplexS, mainHeight), 0.0, 1.0);

	greenCoeff = clamp(fragmentColor.r * greenCoeff, 0.0, 1.0);


    float waterLineCoeff = smoothstep(-0.1, 0.0, -fragmentPosition.y);
    float waterLineCoeff2 = pow(smoothstep(-0.1, 0.0, -fragmentPosition.y), 0.2);
	albedoSand = heightlerp(pow(albedoSand, vec3(1.2)), 0.5, albedoSand, smallSimplexS, waterLineCoeff);

	vec3 albedo;
	albedo = heightlerp(albedoSand, 0.5, albedoGreen, bigSimplex * smallSimplex, greenCoeff);
	albedo = heightlerp(albedo, 0.5, albedoMountains, bigSimplex, fragmentColor.g);

	//albedo = mix(albedoSand, albedoGreen, greenCoeff);
	//albedo = mix(albedo, albedoMountains, fragmentColor.g);







	vec3 nmMountains = normalize(texture(sampler2D(mountainNormal, repeatSampler), texUv * texScaleMountains).xyz * 2.0 - 1.0);
    vec3 nmGreen = normalize(texture(sampler2D(greenNormal, repeatSampler), texUv * texScaleGreen).xyz * 2.0 - 1.0);
	vec3 nmSand = normalize(texture(sampler2D(sandNormal, repeatSampler), texUv * texScaleSand).xyz * 2.0 - 1.0);

    nmSand.y = -nmSand.y;
    nmGreen.y = -nmGreen.y;
    nmMountains.y = -nmMountains.y;

    nmGreen.z *= 2.0;
    nmGreen = normalize(nmGreen);

    //nmSand.g = -nmSand.g;
    //nmGreen.g = -nmGreen.g;
    //nmMountains.g = -nmMountains.g;

	vec3 nm;
    //nm = normalize(mix(nmSand, nmGreen, fragmentColor.r));	
	//nm = normalize(mix(nm, nmMountains, fragmentColor.g));

	nm = normalize(heightlerp(nmSand, 0.5, nmGreen, bigSimplex * smallSimplex, greenCoeff));
	nm = normalize(heightlerp(nm, 0.5, nmMountains, bigSimplex, fragmentColor.g));
    //nm = nmSand;
    
    //nm = nmMountains;


	vec3 n = normalize(fragmentNormal.xyz);
	vec3 t = normalize(fragmentTangent.xyz);
	vec3 b = fragmentTangent.w * cross(n, t);



	vec3 wn = normalize(nm.x * t + nm.y * b + nm.z * n);
	vec3 l = normalize(vec3(1.0, -1.0, 1.0));
	vec3 ndotl = vec3(clamp(dot(wn, l), EPSILON, 1.0));
	vec3 softl = vec3(dot(wn,l) *0.5 + 0.5);
	vec3 v = normalize(fragmentPosition.xyz - (inverse(renderer.viewMatrix) * vec4(0.0, 0.0, 0.0, 1.0)).xyz);
    //v = normalize((inverse(renderer.viewMatrix) * vec4(0.0, 0.0, -1.0, 0.0)).xyz);
	//float vdotn = pow(1.0 - clamp(dot(v, wn), EPSILON, 1.0), 5.0);
	vec3 flatv = v;
	flatv.y = 0.0;
	flatv = normalize(flatv);
	float vdotn2 = max(0.0, pow(dot(flatv, n), 1.0 / 16.0));
	float mountainFresnel = vdotn2 * smoothstep(0.1, 1.0, -fragmentPosition.y);
	//float vdotn3 = pow(1.0 - clamp(dot(v, wn), EPSILON, 1.0), 0.25);

	float brdfy = clamp(dot(wn, -v), EPSILON, 1.0);
	float roughness = 0.2;
	float metallic = (1.0 - waterLineCoeff) * 0.5;
	 
     metallic = 0.0;

    //albedo= vec3(1.0, 1.0, 1.0);

	vec3 F0 = vec3(0.04); 
    vec3 specularCoeff = mix(F0, albedo, metallic) * waterLineCoeff2; 

	vec3 diffuseCoeff = albedo * (1.0 - F0) * (1.0 - metallic);

	float lod = roughness * 6.0;
	vec3 r = reflect(v, wn);
	
	vec3 irr = texture(sampler2D(irradianceCube, equirectSampler), equirectangularUv(wn)).rgb;

	vec3 rad = textureLod(sampler2D(radianceCube, equirectSampler), equirectangularUv(r), lod).rgb;
	vec2 brdf = textureLod(sampler2D(integratedBrdf, clampSampler), vec2(brdfy, 1.0 - roughness), 0.0).xy;
	
	float shadowSimplex = (simplex((fragmentPosition.xz * 0.1) - vec2(0.4, 0.6) * renderer.time.x * 0.05 ) * 0.5 + 0.5);
	float shadowCoeff = pow(shadowSimplex, 0.62);
	shadowCoeff = smoothstep(0.4, 0.7, shadowCoeff) * 0.5 + 0.5;

	// indirect light, base
	outColor.rgb =  vec3(0.0);

	// soft base sun light, beautiful
	//outColor.rgb += albedo * vec3(0.76, 0.8, 1.0) * softl * 1.0;
    outColor.rgb = albedo * vec3(0.76, 0.8, 1.0) * ndotl * 1.0 * shadowCoeff;

	// bravissimo!!! mountain fresnel is beaut!!
	outColor.rgb += vec3(1.0, 0.8, 0.76) * mountainFresnel * 0.60; 

	// ibl specular
	outColor.rgb += rad * ( specularCoeff* brdf.x + brdf.y) * shadowCoeff ;	
    
	// ibl diffuse 
	outColor.rgb += diffuseCoeff * irr * shadowCoeff;

    //outColor.rgb = ndotl;


	// debug norm
	//outColor.rgb = clamp(vec3(wn.x, wn.z, -wn.y) * 0.5 + 0.5, vec3(EPSILON), vec3(1.0));
    //outColor.rgb = vec3(ndotl);


    //outColor.rgb = v;




    //vec3 visibility = textureLod(sampler2D(visibilityMask, clampSampler), gl_FragCoord.xy / vec2(resolution.x, resolution.y), 0).xyz;
    //outColor.rgb = fogOfWar(outColor.rgb, fragmentPosition.xyz, visibility, renderer.time.x);

	float gridCoeff2 = smoothstep(0.92, 0.95, fragmentColor.a);
	gridCoeff2 *= 0.15;
	//outColor.rgb = mix(outColor.rgb, vec3(0.0), gridCoeff2 * visibility.y);

}