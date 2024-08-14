#include <shaders/header.glsl>
// Fragment attributes

in vec4 fragmentPosition;

in vec3 fragmentNormal;

// Out color
out vec4 outColor;
out vec4 outPosition;

#include <shaders/demo/demo.common.glsl>

void main()
{
	// 0 = default
	// 1 = height
	// 2 = normal
	// 3 = baselayer
	// 4 = reflection 
	// 5 = bottomlayer
	// 6 = caustics

	uint stage = material.water2.x;
	uint substage = material.water2.y;
    float time = renderer.time.x *0.67;

	float iblStrength = renderer.ibl1.x;
	float sunShadow = calculateSunShadow(fragmentPosition);
	float sunStrength =  sunShadow * renderer.sun2.w;

	outPosition = fragmentPosition;
	outColor = vec4(0.0, 0.0, 0.0, 1.0);

	float waterHeight = sampleWaterHeight(fragmentPosition.xz, time);
	vec3 waterNormal = sampleWaterNormal(fragmentPosition.xz, time);
	vec3 waterViewNormal = (renderer.viewMatrix * vec4(waterNormal, 0.0)).xyz;

	// HEIGHT
	if(stage == 1)
	{
		//n.y = -n.y;
		outColor.rgb = vec3(waterHeight);
		outColor.rgb *= iblStrength;
		return;
	}


	// NORMAL
	if(stage == 2)
	{
		//n.y = -n.y;
		outColor.rgb = clamp(vec3(waterNormal.x, waterNormal.z, -waterNormal.y) * 0.5 + 0.5, vec3(0.0), vec3(1.0));
		outColor.rgb *= iblStrength;

		return;
	}

	vec2 screenUv = getScreenPixelUV(gl_FragCoord.xy);
	vec3 frontPosition_OG = getFrontPosition(screenUv);
	vec3 frontColor_OG = getFrontColor(screenUv);
	float depth_OG = distance(frontPosition_OG, fragmentPosition.xyz);

	float depthScale_OG = smoothstep(0.0, 1.0,  depth_OG);
	vec2 pixelScale = vec2(1.0) / resolution;
	vec2 off_OG = -waterViewNormal.xz * depthScale_OG * pixelScale * material.albedo.x;

	vec3 frontColor = getFrontColor(screenUv + off_OG);
	vec3 frontPosition = getFrontPosition(screenUv + off_OG);

	float refractFixCoeff = 1.0 - smoothstep(fragmentPosition.y + 0.075, fragmentPosition.y + 0.15, frontPosition.y);

	if(stage == 7)
	{
		if(substage == 3)
		{
			outColor.rgb = vec3(refractFixCoeff);
			return;
		}
	}


	if(stage == 0)
	{
		frontColor = mix(frontColor, frontColor_OG, refractFixCoeff);
		frontPosition = mix(frontPosition, frontPosition_OG, refractFixCoeff);
	}
	
		

	float causticsCoeff = sampleWaterHeight(frontPosition.xz, time);
	causticsCoeff = clamp( smoothstep(0.0, 0.5, causticsCoeff), 0.0, 1.0);


	//vec3 n = vec3(0.0, -1.0, 0.0);

	vec3 lightToSurface = normalize(renderer.sun1.xyz);
	vec3 ndotl = vec3(clamp(dot(waterNormal, -lightToSurface), 0.0, 1.0));
	
	vec3 viewToSurface = normalize(fragmentPosition.xyz - (inverse(renderer.viewMatrix) * vec4(0.0, 0.0, 0.0, 1.0)).xyz);
	viewToSurface.y *= 0.25;
	viewToSurface = normalize(viewToSurface);
	float vdotn = clamp(dot(viewToSurface, waterNormal), 0.0, 1.0);

	vec3 lightWater = vec3(28.0, 200.0, 255.0) / 255.0;

	vec3 darkWater = vec3(0.0, 60.0, 107.0) / 255.0;
	vec3 shoreDark  = pow(darkWater, vec3(2.4));
	lightWater = darkWater*5.0;
	
	vec3 r = reflect(viewToSurface, waterNormal);

	vec3 hdr = textureLod(sampler2D(reflectionHdr, equirectSampler), equirectangularUv(r), 0.0).rgb;
	//vec3 hdr = textureLod(sampler2D(radianceCube, equirectSampler), equirectangularUv(r), 4.0).rgb;



    
	float baseHeightCoeff = waterHeight *0.5;
	vec3 baseSunCoeff = (ndotl * 0.5 + 0.5)*0.5;
	vec3 baseColor = mix(shoreDark, lightWater,  baseHeightCoeff * baseSunCoeff * sunShadow );
	
	// BASELAYER
	if(stage == 3)
	{
		if(substage == 0)
		{
			outColor.rgb = shoreDark * iblStrength;
		}
		else if(substage == 1)
		{
			outColor.rgb = mix(shoreDark, lightWater, baseHeightCoeff) * iblStrength;
		}
		else if(substage == 2)
		{
			outColor.rgb = baseSunCoeff* iblStrength;
		}
		else if(substage >= 3)
		{
			outColor.rgb = baseColor* iblStrength;
		}
		
		return;
	}

	float viewDistanceToDepth = distance(frontPosition, fragmentPosition.xyz);
	float deepCoeff = 1.0 - smoothstep(0.0, 1.0, viewDistanceToDepth);
	float deepCoeff2 = 1.0 - smoothstep(0.0, 0.2, viewDistanceToDepth);
	vec3 frontColorCaustics = mix(frontColor, frontColor + (frontColor*renderer.sun2.w), causticsCoeff *(1.0 -deepCoeff2));

	// apply deep color as we approach shooba
	float shallowCoeff = 1.0 - smoothstep(0.0, 0.35, viewDistanceToDepth);
	vec3 shallowColor = mix(baseColor, frontColorCaustics, 0.15);
	vec3 deepColor = mix(frontColorCaustics*baseColor, shallowColor, shallowCoeff);

	// basecolor
	outColor.rgb = mix(baseColor, deepColor,  0.1 + deepCoeff*0.9) * iblStrength;

	float viewDepthCoeff2 = 1.0 - smoothstep(0.0, 0.2, viewDistanceToDepth);
	outColor.rgb = mix(outColor.rgb, frontColor, viewDepthCoeff2);



    float reflCoeff = smoothstep(0.1, 0.7, waterHeight);

	// fresnel reflection
	float reflectionStrength = 1.0;
	outColor.rgb += hdr * vdotn * reflectionStrength * reflCoeff * sunStrength;
	outColor.a = 1.0;

	// REFLECTION
	if(stage == 4)
	{
		if(substage == 0)
		{
			outColor.rgb = hdr * sunStrength;
		}
		else if(substage == 1)
		{
			outColor.rgb = vec3(vdotn * sunStrength);
		}
		else if(substage == 2)
		{
			outColor.rgb = vec3(vdotn * sunStrength* reflCoeff);
		}
		else if(substage >= 3)
		{
			outColor.rgb = baseColor* iblStrength;
			outColor.rgb += hdr * vdotn * reflectionStrength * sunStrength* reflCoeff;
		}
		return;
	}

	// BOTTOMLAYER
	if(stage == 5)
	{
		vec3 baseColorDBG = baseColor;
		float viewDistanceToDepthDBG = distance(frontPosition_OG, fragmentPosition.xyz);
		float deepCoeffDBG = 1.0 - smoothstep(0.0, 1.0, viewDistanceToDepthDBG);
		float shallowCoeffDBG = 1.0 - smoothstep(0.0, 0.35, viewDistanceToDepthDBG);
		vec3 shallowColor2 = mix(baseColorDBG, frontColor_OG, 0.15);
		vec3 deepColor2 = mix(frontColor_OG*baseColorDBG, shallowColor2, shallowCoeffDBG);
		vec3 deepColor3 =  mix(baseColorDBG, deepColor2,  0.1 + deepCoeffDBG*0.9);

		if(substage == 0)
		{
			outColor.rgb = shallowColor2* iblStrength;
			
			//outColor.rgb += hdr * vdotn * reflectionStrength * sunStrength* reflCoeff;
		}
		else if(substage == 1)
		{
			outColor.rgb = deepColor2* iblStrength;
			
			//outColor.rgb += hdr * vdotn * reflectionStrength * sunStrength* reflCoeff;
		}
		else if(substage == 2)
		{
			outColor.rgb = deepColor3* iblStrength;
			
			outColor.rgb += hdr * vdotn * reflectionStrength * sunStrength* reflCoeff;
		}
		else if (substage >= 3)
		{
			float viewDepthCoeff2DBG = 1.0 - smoothstep(0.0, 0.2, viewDistanceToDepthDBG);

			outColor.rgb = deepColor3* iblStrength;
			outColor.rgb = mix(outColor.rgb, frontColor_OG, viewDepthCoeff2DBG);
			outColor.rgb += hdr * vdotn * reflectionStrength * sunStrength* reflCoeff;

			
		}

		return;
	}

	// CAUSTICS
	if(stage == 6)
	{

		float causticsCoeffDBG = sampleWaterHeight(frontPosition_OG.xz, time);
		causticsCoeffDBG = smoothstep(0.0, 0.5, causticsCoeffDBG);

		vec3 baseColorDBG = baseColor;
		float viewDistanceToDepthDBG = distance(frontPosition_OG, fragmentPosition.xyz);
		float deepCoeffDBG = 1.0 - smoothstep(0.0, 1.0, viewDistanceToDepthDBG);
		float shallowCoeffDBG = 1.0 - smoothstep(0.0, 0.35, viewDistanceToDepthDBG);

		float deepCoeff2DBG = 1.0 - smoothstep(0.0, 0.2, viewDistanceToDepthDBG);
		vec3 frontColorCausticsDBG = mix(frontColor_OG, frontColor_OG + (frontColor_OG*renderer.sun2.w), causticsCoeffDBG *(1.0 -deepCoeff2DBG));

		vec3 shallowColor2 = mix(baseColorDBG, frontColorCausticsDBG, 0.15);
		vec3 deepColor2 = mix(frontColorCausticsDBG*baseColorDBG, shallowColor2, shallowCoeffDBG);
		vec3 deepColor3 =  mix(baseColorDBG, deepColor2,  0.1 + deepCoeffDBG*0.9);
		float viewDepthCoeff2DBG = 1.0 - smoothstep(0.0, 0.026, viewDistanceToDepthDBG);

		if(substage == 0)
		{
			outColor.rgb = vec3(causticsCoeffDBG *(1.0 -deepCoeff2DBG));
		}
		else if(substage == 1)
		{
			outColor.rgb = frontColorCausticsDBG;
		}
		else if(substage >= 2)
		{
			outColor.rgb = deepColor3* iblStrength;
			outColor.rgb = mix(outColor.rgb, frontColor_OG, viewDepthCoeff2DBG);
			outColor.rgb += hdr * vdotn * reflectionStrength * sunStrength* reflCoeff;
		}


		return;
	}

	// REFRACTION
	if(stage == 7)
	{
		if(substage == 0)
		{
			vec2 refracUv = (-waterViewNormal.xz) * 0.5 + 0.5;
			outColor.rgb = vec3(refracUv.x, refracUv.y, 0.0);

		}
		else if(substage == 1)
		{
			outColor.rgb = frontColor.rgb;
		}
		return;
	}

}