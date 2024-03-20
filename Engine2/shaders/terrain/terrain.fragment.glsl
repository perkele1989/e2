#version 460 core

// Fragment attributes

in vec4 fragmentPosition;

in vec3 fragmentNormal;
in vec4 fragmentTangent;
in vec4 fragmentColor;

// Out color
out vec4 outColor;
out vec4 outPosition;

#include <shaders/terrain/terrain.common.glsl>

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
	//albedoGreen *= vec3(1.0, 1.0, 1.0) * 0.85;

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


	vec3 nmMountains = normalize(texture(sampler2D(mountainNormal, repeatSampler), texUv * texScaleMountains).xyz * 2.0 - 1.0);
    vec3 nmGreen = normalize(texture(sampler2D(greenNormal, repeatSampler), texUv * texScaleGreen).xyz * 2.0 - 1.0);
	vec3 nmSand = normalize(texture(sampler2D(sandNormal, repeatSampler), texUv * texScaleSand).xyz * 2.0 - 1.0);

    nmSand.y = -nmSand.y;
    nmGreen.y = -nmGreen.y;
    nmMountains.y = -nmMountains.y;

    nmGreen.z *= 4.0;
    nmGreen = normalize(nmGreen);

    //nmSand.g = -nmSand.g;
    //nmGreen.g = -nmGreen.g;
    //nmMountains.g = -nmMountains.g;

	vec3 normalTexel;
	normalTexel = normalize(heightlerp(nmSand, 0.5, nmGreen, bigSimplex * smallSimplex, greenCoeff));
	normalTexel = normalize(heightlerp(normalTexel, 0.5, nmMountains, bigSimplex, fragmentColor.g));

	vec3 fragNormal = normalize(fragmentNormal.xyz);
	vec3 fragTangent = normalize(fragmentTangent.xyz);
	vec3 fragBitangent = fragmentTangent.w * cross(fragNormal, fragTangent);
	vec3 finalNormal = normalize(normalTexel.x * fragTangent + normalTexel.y * fragBitangent + normalTexel.z * fragNormal);
	vec3 viewVector = getViewVector(fragmentPosition.xyz);

	float roughness = 0.2;
	float metalness = (1.0 - waterLineCoeff) * 0.1;

	outColor.rgb =  vec3(0.0);
	outColor.rgb += getSunColor(finalNormal, albedo);
	outColor.rgb += getIblColor(fragmentPosition.xyz, albedo, finalNormal, roughness, metalness, viewVector);

	float rimHeightCoeff = smoothstep(0.1, 1.0, -fragmentPosition.y);
	outColor.rgb += getRimColor(fragNormal, viewVector, vec3(1.0, 0.7, 0.5) * 0.45) * rimHeightCoeff;

	outColor.rgb *= getCloudShadows(fragmentPosition.xyz);


	// debug norm
	//outColor.rgb = clamp(vec3(finalNormal.x, finalNormal.z, -finalNormal.y) * 0.5 + 0.5, vec3(0.0), vec3(1.0));

	float gridCoeff2 = smoothstep(0.92, 0.95, fragmentColor.a);
	gridCoeff2 *= 0.15;
	outColor.rgb = mix(outColor.rgb, vec3(0.0), gridCoeff2);

}