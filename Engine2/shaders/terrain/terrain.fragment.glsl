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

	float texScaleMountains = 0.05;
	float texScaleFields = 0.5;
	float texScaleSand = 0.05;
	float texScaleGreen = 0.2;
    vec2 texUv = fragmentPosition.xz;
    texUv.y = -texUv.y;

	vec4 outline = texture(sampler2D(outlineTexture, clampSampler),  gl_FragCoord.xy / vec2(resolution.x, resolution.y)).rgba;

	vec3 albedoSand = texture(sampler2D(sandAlbedo, repeatSampler), texUv * texScaleSand).rgb;
	


	vec3 albedoGreen = texture(sampler2D(greenAlbedo, repeatSampler), texUv * texScaleGreen).rgb;
	albedoGreen = pow(albedoGreen, vec3(1.4));

	// = mix(albedoGreen, vec3(dot(albedoGreen, vec3(1.0/3.0))) ,0.2);

	float bigSimplex = simplex(fragmentPosition.xz * 4) * 0.5 + 0.5;
	float smallSimplex = simplex(fragmentPosition.xz * 1) * 0.5 + 0.5;
	float smallSimplexS = simplex(fragmentPosition.xz * 1.1) * 0.5 + 0.5;


	float waterKill = smoothstep(0.0, 0.15, fragmentPosition.y);

	float forestCoeff = fragmentColor.r;
	forestCoeff = mix(forestCoeff, 0.0, waterKill);
	forestCoeff = pow(forestCoeff, 1.0/0.5);
	forestCoeff =smoothstep(0.0, 0.5, forestCoeff);
	//outColor.rgb = vec3(forestCoeff);
	//return;
	


	float tundraCoeff =fragmentColor.b;
	tundraCoeff = mix(tundraCoeff, 0.0, waterKill);


	float tundraMountainCoeff = 1.0 - smoothstep(-0.5, -0.2, fragmentPosition.y);
	tundraCoeff = clamp(tundraCoeff + tundraMountainCoeff, 0.0, 1.0);

	tundraCoeff = mix(tundraCoeff, 0.0, 1.0 - clamp( pow(dot(vec3(0.0, -1.0, 0.0), fragmentNormal.xyz), 2.4), 0.0, 1.0));

	// outColor.rgb = vec3(tundraCoeff);
	// return;

	float greenCoeff = max(fragmentColor.g + fragmentColor.r, 0.0);	
	greenCoeff = mix(greenCoeff, 0.0, waterKill);

	float desertCoeff = 1.0 - max(fragmentColor.g + (fragmentColor.b), 0.0);

	tundraCoeff *= 1.0 - desertCoeff;


	//vec3 albedoForest = mix(albedoGreen, albedoSand, 0.25)*0.345;
	vec3 albedoForest = albedoGreen;
	//albedoForest = mix(albedoForest, albedoSand, 0.1);
	albedoForest = pow(shiftHue(albedoForest, 0.114), vec3(1.1));
	
    float waterLineCoeff = smoothstep(-0.1, 0.0, -fragmentPosition.y);
    float waterLineCoeff2 = pow(smoothstep(-0.1, 0.0, -fragmentPosition.y), 0.2);
	//albedoSand = heightlerp(pow(albedoSand, vec3(1.6)), 0.5, pow(albedoSand, vec3(1.4)), smallSimplexS, waterLineCoeff);
	albedoSand = pow(albedoSand, vec3(1.4));

	float mountainCoeff = 1.0 - smoothstep(-0.1, 0.0, fragmentPosition.y);


	vec3 albedoMountains = pow(texture(sampler2D(mountainAlbedo, repeatSampler), texUv * texScaleMountains).rgb, vec3( mix(2.0, 1.0, 1.0-smoothstep(-1.0, 0.0, fragmentPosition.y))));
	
	albedoMountains = desaturate_blend(albedoMountains, 0.5 * (1.0 - desertCoeff));

	albedoGreen = mix(albedoGreen, albedoForest, forestCoeff);
	vec3 grassTundraMix = heightlerp(albedoGreen, 0.5, pow(vec3(0.66, 0.67, 0.7), vec3(1.24)), bigSimplex + smallSimplex, tundraCoeff);



	albedoMountains= heightlerp(albedoMountains, 0.9, pow(vec3(0.66, 0.67, 0.7), vec3(1.24)), bigSimplex + smallSimplex, tundraCoeff);


	

	vec3 albedo;
	albedo = heightlerp(albedoSand, 1.0, grassTundraMix, bigSimplex + smallSimplex, clamp(greenCoeff + tundraCoeff, 0.0, 1.0));
	
	albedo = heightlerp(albedo, 1.0, albedoMountains, bigSimplex, mountainCoeff);

	


	vec3 nmMountains = normalize(texture(sampler2D(mountainNormal, repeatSampler), texUv * texScaleMountains).xyz * 2.0 - 1.0);
    //vec3 nmGreen = normalize(texture(sampler2D(greenNormal, repeatSampler), texUv * texScaleGreen).xyz * 2.0 - 1.0);
	vec3 nmSand = normalize(texture(sampler2D(sandNormal, repeatSampler), texUv * texScaleSand).xyz * 2.0 - 1.0);

    nmSand.y = -nmSand.y;
    //nmGreen.y = -nmGreen.y;
    //nmMountains.y = -nmMountains.y;

    nmSand.z *= mix(2.0, 4.0, clamp(greenCoeff, 0.0, 1.0));
    nmSand = normalize(nmSand);

	nmMountains.z *= 2.0;
	nmMountains = normalize(nmMountains);

    //nmSand.g = -nmSand.g;
    //nmGreen.g = -nmGreen.g;
    //nmMountains.g = -nmMountains.g;

	vec3 normalTexel = nmSand;
	//normalTexel = normalize(heightlerp(nmSand, 0.5, nmGreen, bigSimplex * smallSimplex, greenCoeff));
	normalTexel = normalize(heightlerp(normalTexel, 0.5, nmMountains, bigSimplex, mountainCoeff));



	vec3 fragNormal = normalize(fragmentNormal.xyz);
	vec3 fragTangent = normalize(fragmentTangent.xyz);
	vec3 fragBitangent = fragmentTangent.w * cross(fragNormal, fragTangent);
	vec3 finalNormal = normalize(normalTexel.x * fragTangent + normalTexel.y * fragBitangent + normalTexel.z * fragNormal);
	vec3 viewVector = getViewVector(fragmentPosition.xyz);

	float roughness = mix(0.8, 0.3, 1.0 - waterLineCoeff);
	//roughness = 0.1;

	float metalness = 0.0;//(1.0 - waterLineCoeff) * 0.1;

	float clouds = getCloudShadows(fragmentPosition.xyz);
	clouds = mix(clouds, 1.0, 1.0 - waterLineCoeff);

	//outColor.rgb = albedo;
	//return;

	outColor.rgb =  vec3(0.0);
	outColor.rgb += getSunColor(fragmentPosition.xyz, finalNormal, albedo, roughness, metalness, viewVector) * clouds;
	outColor.rgb += getIblColor(fragmentPosition.xyz, albedo, finalNormal, roughness, metalness, viewVector)* clouds;

	float rimHeightCoeff = smoothstep(0.1, 1.0, -fragmentPosition.y);
	outColor.rgb += albedo * getRimColor(fragNormal, viewVector, renderer.sun2.xyz * renderer.sun2.w * 8.0) * rimHeightCoeff ;
	//outColor.rgb *= getCloudShadows(fragmentPosition.xyz);

	float outlineCoeff = 1.0 - smoothstep(-0.05, -0.06, fragmentPosition.y);
	float outlinePower = (renderer.sun2.w + renderer.ibl1.x) * 0.5;
	outColor.rgb = mix(outColor.rgb, outline.rgb *outlinePower, outline.a * outlineCoeff);


	//outColor.rgb = getIblColor(fragmentPosition.xyz, albedo, finalNormal, roughness, metalness, viewVector);

	// debug norm
	//outColor.rgb = clamp(vec3(finalNormal.x, finalNormal.z, -finalNormal.y) * 0.5 + 0.5, vec3(0.0), vec3(1.0));

	vec4 h = getHex(fragmentPosition.xz);
	float hd = hex(h.xy);
	float hcf = pow(smoothstep(0.475, 0.5, hd), 0.25) * (1.0 - mountainCoeff);
	outColor.rgb = mix(outColor.rgb, outColor.rgb *0.75, hcf*(1.0-waterKill)* float(gridParams.x));


	//outColor.r = hcf;

#endif

}