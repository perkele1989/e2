#include <shaders/header.glsl>
// Fragment attributes

in vec4 fragmentPosition;

in vec3 fragmentNormal;

// Out color
out vec4 outColor;
out vec4 outPosition;

#include <shaders/water/water.common.glsl>

void main()
{

    float time = renderer.time.x *0.67*2.0;

	float iblStrength = renderer.ibl1.x;
	float sunShadow = calculateSunShadow(fragmentPosition);
	float sunStrength =  sunShadow * renderer.sun2.w;

	vec2 screenUv = getScreenPixelUV(gl_FragCoord.xy);

	outPosition = fragmentPosition;
	outColor = vec4(0.0, 0.0, 0.0, 1.0);

	float waterHeight = sampleWaterHeight(fragmentPosition.xz, time);
	vec3 waterNormal = sampleWaterNormal(fragmentPosition.xz, time);
	vec3 waterViewNormal = (renderer.viewMatrix * vec4(waterNormal, 0.0)).xyz;

	vec3 frontPosition_OG = getFrontPosition(screenUv);
	vec3 frontColor_OG = getFrontColor(screenUv);
	float depth_OG = distance(frontPosition_OG, fragmentPosition.xyz);

	float depthScale_OG = smoothstep(0.0, 2.0,  depth_OG);
	vec2 pixelScale = vec2(1.0) / resolution;
	vec2 off_OG = -waterViewNormal.xz * depthScale_OG * pixelScale *5.0;

	vec3 frontColor = getFrontColor(screenUv + off_OG);
	vec3 frontPosition = getFrontPosition(screenUv + off_OG);

	float refractFixCoeff = 1.0 - smoothstep(fragmentPosition.y + 0.075, fragmentPosition.y + 0.15, frontPosition.y);

	frontColor = mix(frontColor, frontColor_OG, refractFixCoeff);
	frontPosition = mix(frontPosition, frontPosition_OG, refractFixCoeff);
	

	vec4 visMask = textureLod(sampler2D(visibilityMask, clampSampler), screenUv , 0).xyzw;
	float deepMask = visMask.w;


		

	float causticsCoeff = sampleWaterHeight(frontPosition.xz, time);
	causticsCoeff = clamp( smoothstep(0.0, 0.5, causticsCoeff), 0.0, 1.0);


	//vec3 n = vec3(0.0, -1.0, 0.0);

	vec3 lightToSurface = normalize(renderer.sun1.xyz);
	vec3 ndotl = vec3(clamp(dot(waterNormal, -lightToSurface), 0.0, 1.0));
	
	vec3 viewToSurface = normalize(fragmentPosition.xyz - (inverse(renderer.viewMatrix) * vec4(0.0, 0.0, 0.0, 1.0)).xyz);
	//viewToSurface.y *= 0.25;
	//viewToSurface = normalize(viewToSurface);

	vec3 lightWater = vec3(28.0, 200.0, 255.0) / 255.0;

	vec3 darkWater = vec3(0.0, 60.0, 107.0) / 255.0;

	darkWater = pow(darkWater, vec3(mix(1.1, 1.25, deepMask)));
	vec3 shoreDark  = pow(darkWater, vec3(2.4));
	lightWater = darkWater*5.0;
	
	vec3 r = reflect(viewToSurface, waterNormal);

	vec3 hdr = textureLod(sampler2D(reflectionHdr, equirectSampler), equirectangularUv(r), 0.0).rgb;
	//vec3 hdr = textureLod(sampler2D(radianceCube, equirectSampler), equirectangularUv(r), 4.0).rgb;



    
	float baseHeightCoeff = waterHeight *0.5;
	vec3 baseSunCoeff = (ndotl * 0.5 + 0.5) * 0.2;
	vec3 baseColor = mix(shoreDark, lightWater,  baseHeightCoeff * baseSunCoeff * sunShadow );
	



	//outColor.rgb = vec3(visMask.a);
	//return;


	float viewDistanceToDepth = distance(frontPosition, fragmentPosition.xyz);
	float causticsDepthCoeff = smoothstep(0.0, 1.5, viewDistanceToDepth);
	vec3 frontColorCaustics = mix(frontColor, frontColor + (frontColor*renderer.sun2.w), causticsCoeff*causticsDepthCoeff );

	//outColor.rgb = frontColorCaustics;
	//return;

	// apply deep color as we approach shooba

	vec3 floorColor =  mix(frontColorCaustics*baseColor, baseColor, 0.75);
	//floorColor = frontColorCaustics*baseColor;

	outColor.rgb = mix(floorColor, baseColor, deepMask*0.25) * iblStrength;
	


	

	//outColor.rgb = mix(outColor.rgb, baseColor * iblStrength, 1.0);

	float shoreFadeFade = mix(1.5, 1.0, deepMask);
	float shoreFade = 1.0 - smoothstep(0.0, shoreFadeFade, viewDistanceToDepth);
	outColor.rgb = mix(outColor.rgb, mix(frontColor, outColor.rgb, 0.7), shoreFade);





	// fresnel reflection
	float vdotn = clamp(pow(dot(viewToSurface, waterNormal), 4.0), 0.0, 1.0);
	float reflectionDepthSoft = smoothstep(0.2, 0.9, waterHeight);
	float reflectionStrength = 1.0;
	outColor.rgb += hdr * vdotn * reflectionStrength * reflectionDepthSoft * sunStrength;

	float shoreFade2 = 1.0 - smoothstep(0.0, 0.4, viewDistanceToDepth);
	outColor.rgb = mix(outColor.rgb, frontColor, shoreFade2);


	// FOAM 
	float foamLength = simplex(fragmentPosition.xz*2.0) * 0.5 + 0.5;
	foamLength = mix(0.5*foamLength, 1.25*foamLength, (sin(time) * 0.5 + 0.5));

	float foamCoeff = 1.0 - smoothstep(0.1, 0.1 + (foamLength) , viewDistanceToDepth);


	float foamSub = smoothstep(0.00, 0.05, viewDistanceToDepth);
	foamCoeff *= foamSub;



	//vec3 getSunColor(vec3 fragPos, vec3 fragNormal, vec3 fragAlbedo, float roughness, float metalness, vec3 viewVector)
	vec3 foamAlbedo = pow(vec3(0.4, 0.35, 0.23), vec3(0.35));
	vec3 foamDirect = getSunColor(fragmentPosition.xyz, vec3(0.0, -1.0, 0.0), foamAlbedo, 0.8, 0.0, viewToSurface);
	vec3 foamIndirect = getIrradiance(vec3(0.0, -1.0, 0.0)) * foamAlbedo;
	vec3 foamColor =  foamDirect + foamIndirect;
	outColor.rgb = mix(outColor.rgb, mix(foamColor, outColor.rgb*foamColor, 0.75), foamCoeff);


	vec4 hx = getHex(fragmentPosition.xz);
	float hd = hex(hx.xy);
	float hcf = pow(smoothstep(0.475, 0.5, hd), 0.25);
	outColor.rgb = mix(outColor.rgb, outColor.rgb *0.5, hcf*0.22 * float(gridParams.x));


	outColor.a = 1.0;


}