#include <shaders/header.glsl>

// Fragment attributes

#if !defined(Renderer_Shadow)
in vec4 fragmentPosition;
in vec3 fragmentNormal;
in vec4 fragmentTangent;
in vec4 fragmentColor;
#endif

#if !defined(Renderer_Shadow)
// Out color
out vec4 outColor;
out vec4 outPosition;
#endif

#include <shaders/terrain/terrain.common.glsl>




void main()
{
#if !defined(Renderer_Shadow)
	outPosition = fragmentPosition;
	outColor = vec4(1.0, 1.0, 1.0, 1.0);

	float texScaleMountains = 0.1;
	float texScaleFields = 0.5;
	float texScaleSand = 0.1;
	float texScaleGreen = 0.2;
    vec2 texUv = fragmentPosition.xz;
    texUv.y = -texUv.y;

	vec4 outline = texture(sampler2D(outlineTexture, clampSampler),  gl_FragCoord.xy / vec2(resolution.x, resolution.y)).rgba;

	vec3 albedoSand = texture(sampler2D(sandAlbedo, repeatSampler), texUv * texScaleSand).rgb;
	


	vec3 albedoGreen = texture(sampler2D(greenAlbedo, repeatSampler), texUv * texScaleGreen).rgb*0.5;
	albedoGreen = mix(albedoGreen, vec3(dot(albedoGreen, vec3(1.0/3.0))) ,0.2);

	float bigSimplex = simplex(fragmentPosition.xz * 6) * 0.5 + 0.5;
	float smallSimplex = simplex(fragmentPosition.xz * 2) * 0.5 + 0.5;
	float smallSimplexS = simplex(fragmentPosition.xz * 1.1) * 0.5 + 0.5;


	float waterKill = smoothstep(0.0, 0.15, fragmentPosition.y);

	float forestCoeff = fragmentColor.r;
	forestCoeff = mix(forestCoeff, 0.0, waterKill);
	forestCoeff = pow(forestCoeff, 1.0/3.0);
	forestCoeff =smoothstep(0.5, 0.9, forestCoeff);
	


	float tundraCoeff =fragmentColor.b;
	tundraCoeff = mix(tundraCoeff, 0.0, waterKill);
	float greenCoeff = max(fragmentColor.g + fragmentColor.r, 0.0);	
	greenCoeff = mix(greenCoeff, 0.0, waterKill);

	float desertCoeff = 1.0 - max(fragmentColor.g + (fragmentColor.b), 0.0);

	vec3 albedoForest = mix(albedoGreen, albedoSand, 0.25)*0.345;
	
    float waterLineCoeff = smoothstep(-0.1, 0.0, -fragmentPosition.y);
    float waterLineCoeff2 = pow(smoothstep(-0.1, 0.0, -fragmentPosition.y), 0.2);
	albedoSand = heightlerp(pow(albedoSand, vec3(1.2)), 0.5, albedoSand, smallSimplexS, waterLineCoeff);

	float mountainCoeff = 1.0 - smoothstep(-0.2, 0.0, fragmentPosition.y);

	vec3 albedoMountains = pow(texture(sampler2D(mountainAlbedo, repeatSampler), texUv * texScaleMountains).rgb, vec3(1.1));
	albedoMountains *= vec3(1.0, 1.0, 1.0) * 0.8;
	albedoMountains = mix(albedoMountains, mix(albedoSand, albedoMountains, 0.25)*0.35, desertCoeff);


	albedoGreen = heightlerp(albedoGreen, 1.0, albedoForest, smallSimplex, forestCoeff);
	vec3 grassTundraMix = heightlerp(albedoGreen, 0.5, vec3(0.7, 0.67, 0.66), bigSimplex + smallSimplex, tundraCoeff);

	

	vec3 albedo;
	albedo = heightlerp(albedoSand, 0.9, grassTundraMix, bigSimplex + smallSimplex, clamp(greenCoeff + tundraCoeff, 0.0, 1.0));
	
	albedo = heightlerp(albedo, 0.9, albedoMountains, bigSimplex, mountainCoeff);

	


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
	normalTexel = normalize(heightlerp(normalTexel, 0.5, nmMountains, bigSimplex, mountainCoeff));

	vec3 fragNormal = normalize(fragmentNormal.xyz);
	vec3 fragTangent = normalize(fragmentTangent.xyz);
	vec3 fragBitangent = fragmentTangent.w * cross(fragNormal, fragTangent);
	vec3 finalNormal = normalize(normalTexel.x * fragTangent + normalTexel.y * fragBitangent + normalTexel.z * fragNormal);
	vec3 viewVector = getViewVector(fragmentPosition.xyz);

	float roughness = 0.25;
	float metalness = (1.0 - waterLineCoeff) * 0.1;

	outColor.rgb =  vec3(0.0);
	outColor.rgb += getSunColor(fragmentPosition.xyz, finalNormal, albedo);
	outColor.rgb += getIblColor(fragmentPosition.xyz, albedo, finalNormal, roughness, metalness, viewVector);

	float rimHeightCoeff = smoothstep(0.1, 1.0, -fragmentPosition.y);
	outColor.rgb += getRimColor(fragNormal, viewVector, vec3(1.0, 0.7, 0.5) * 0.35 * (renderer.sun2.w * 1/1.75)) * rimHeightCoeff * (1.0 - desertCoeff * 0.5);
	outColor.rgb *= getCloudShadows(fragmentPosition.xyz);

	float outlineCoeff = 1.0 - smoothstep(-0.05, -0.06, fragmentPosition.y);
	outColor.rgb = mix(outColor.rgb, outline.rgb, outline.a * outlineCoeff);


	// debug norm
	//outColor.rgb = clamp(vec3(finalNormal.x, finalNormal.z, -finalNormal.y) * 0.5 + 0.5, vec3(0.0), vec3(1.0));

	float gridCoeff2 = smoothstep(0.92, 0.95, fragmentColor.a);
	gridCoeff2 *= 0.2;
	//outColor.rgb = mix(outColor.rgb, vec3(0.0), gridCoeff2);
#endif

}