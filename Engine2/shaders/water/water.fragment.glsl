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



	// float prevTime = (renderer.time.x - renderer.time.w) * 0.67 * 2.0;
	// float prevHeight = sampleWaterHeight(fragmentPosition.xz, prevTime);
	// vec3 prevNormal = sampleWaterNormal(fragmentPosition.xz, prevTime);
	

	// vec3 posNow = vec3(fragmentPosition.x, -waterHeight, fragmentPosition.z);
	// vec3 posThen = vec3(fragmentPosition.x, -prevHeight, fragmentPosition.z);
	// float distanceTravelled = distance(waterNormal.xyz, prevNormal.xyz);
	// float velocity = distanceTravelled / renderer.time.w;

	// outColor.rgb = vec3(distanceTravelled);
	// return;



	vec3 waterViewNormal = (renderer.viewMatrix * vec4(waterNormal, 0.0)).xyz;

	vec3 frontPosition_OG = getFrontPosition(screenUv);
	vec3 frontColor_OG = getFrontColor(screenUv);
	float depth_OG = distance(frontPosition_OG, fragmentPosition.xyz);

	float depthScale_OG = smoothstep(0.0, 2.0,  depth_OG);
	vec2 pixelScale = vec2(1.0) / resolution;
	vec2 off_OG = -waterViewNormal.xz * depthScale_OG * pixelScale *15.0;

	vec3 frontColor = getFrontColor(screenUv + off_OG);
	vec3 frontPosition = getFrontPosition(screenUv + off_OG);

	float refractFixCoeff = 1.0 - smoothstep(fragmentPosition.y + 0.075, fragmentPosition.y + 0.15, frontPosition.y);

	// outColor = vec4(refractFixCoeff,refractFixCoeff,refractFixCoeff,1.0);
	// return;

	frontColor = mix(frontColor, frontColor_OG, refractFixCoeff);
	frontPosition = mix(frontPosition, frontPosition_OG, refractFixCoeff);
	

	vec4 visMask = textureLod(sampler2D(visibilityMask, clampSampler), screenUv , 0).xyzw;
	float deepMask = visMask.w;


		


	//vec3 n = vec3(0.0, -1.0, 0.0);

	vec3 lightToSurface = normalize(renderer.sun1.xyz);
	float ndotl = clamp(dot(waterNormal, -lightToSurface), 0.0, 1.0);
	
	vec3 viewToSurface = normalize(fragmentPosition.xyz - (inverse(renderer.viewMatrix) * vec4(0.0, 0.0, 0.0, 1.0)).xyz);
	//viewToSurface.y *= 0.25;
	//viewToSurface = normalize(viewToSurface);

	// vec3 lightWaterCol = vec3(28.0, 200.0, 255.0) / 255.0;
	// vec3 darkWaterCol = vec3(0.0, 60.0, 107.0) / 255.0;

	vec3 lightWaterCol = pow(vec3(0, 111, 214) / 255.0, vec3(2.2));
	vec3 darkWaterCol = pow(vec3(0, 42, 105) / 255.0, vec3(2.2));


	vec3 lightWaterCol2 = lightWaterCol*0.87;
	vec3 darkWaterCol2 = darkWaterCol * 0.4;

	// vec3 lightWaterCol2 = pow(vec3(0, 111, 214) / 255.0, vec3(2.2));
	// vec3 darkWaterCol2 = pow(vec3(0, 42, 105) / 255.0, vec3(3.2));

	lightWaterCol = mix(lightWaterCol, lightWaterCol2, deepMask);
	darkWaterCol = mix(darkWaterCol, darkWaterCol2, deepMask);

	//vec3 darkWater = pow(darkWaterCol, vec3(mix(1.1, 1.25, deepMask)));
	//vec3 shoreDark  = pow(darkWater, vec3(2.4));
	//vec3 lightWater = darkWater*5.0;
	
	vec3 r = reflect(viewToSurface, waterNormal);

    
	float baseHeightCoeff = waterHeight;
	//float baseHeightCoeff = clamp(dot(waterNormal, vec3(0.0, -2.0, 0.0)), 0.0, 1.0) * waterHeight;


	float baseSunCoeff = ndotl *0.5 + 0.5;
	vec3 baseColor = mix(darkWaterCol, lightWaterCol, baseHeightCoeff*baseSunCoeff * sunShadow ) * iblStrength;

	float desaturation = 0.2;
	float gamma = 1.2;
	float hue = -0.2;
	baseColor = mix(baseColor, vec3(desaturate(baseColor)), desaturation);
	baseColor = pow(baseColor, vec3(gamma));
	baseColor = shiftHue(baseColor, hue);

	//   outColor.rgb = baseColor * iblStrength;
	//   return;



	float causticsCoeff = sampleWaterHeight(frontPosition_OG.xz, time);
	causticsCoeff = smoothstep(0.0, 1.0, causticsCoeff);
	//causticsCoeff = pow(causticsCoeff, 1.0);
	//outColor.rgb = vec3(causticsCoeff);
	//return;
	//causticsCoeff = clamp( smoothstep(0.0, 1.0, causticsCoeff), 0.0, 1.0);

	float viewDistanceToDepth = distance(frontPosition, fragmentPosition.xyz);
	float viewDistanceToDepth_OG = distance(frontPosition_OG, fragmentPosition.xyz);
	float causticsDepthCoeff = smoothstep(0.0, 0.5, viewDistanceToDepth_OG);
	vec3 frontColorCaustics = mix(frontColor_OG,(frontColor_OG*renderer.sun2.w*2.0), causticsCoeff*causticsDepthCoeff );

	// outColor.rgb = vec3(causticsCoeff*causticsDepthCoeff);
	// return;


	outColor.rgb = mix(frontColorCaustics*baseColor, baseColor, 0.4 + 0.4*deepMask);
	
	float shoreFade = 1.0 - smoothstep(0.0, 1.1, pow(viewDistanceToDepth, 1.0/3.0));
	outColor.rgb = mix(outColor.rgb,frontColor, shoreFade);

	// fresnel reflection
	float clouds = getCloudShadows(fragmentPosition.xyz);
	float vdotn = clamp(pow(dot(viewToSurface, waterNormal), 4.0), 0.0, 1.0);
	float reflectionDepthSoft = smoothstep(0.1, 1.0, waterHeight);
	float reflectionStrength = 0.75;
	float reflectionCoeff =  vdotn * reflectionStrength *reflectionDepthSoft;
	vec3 hdr = textureLod(sampler2D(reflectionHdr, equirectSampler), equirectangularUv(r), 0.0).rgb;
	outColor.rgb += hdr * sunStrength * reflectionCoeff;// *clouds;


	float shoreFade2 = 1.0 - smoothstep(0.0, 0.4, viewDistanceToDepth);
	outColor.rgb = mix(outColor.rgb, frontColor, shoreFade2);


	// FOAM 
	float foamLengthMin = 0.3;
	float foamLengthMax = 1.0;
	float foamLengthVar = sin(time) * 0.5 + 0.5;
	foamLengthVar *= simplex(fragmentPosition.xz*0.5 + vec2(0.3, 0.15) * renderer.time.x) * 0.5 + 0.5;
	float foamLengthX = mix(foamLengthMin, foamLengthMax, foamLengthVar);
	float foamLength = foamLengthX* (simplex(fragmentPosition.xz*2.0 +  vec2(0.2, 0.4) * 0.1 * renderer.time.x) * 0.5 + 0.5);
	//foamLength = mix(0.3*foamLength, 1.0*foamLength, );

	float foamCoeff = 1.0 - smoothstep(0.1, 0.1 + (foamLength) , viewDistanceToDepth);


	float foamSub = smoothstep(0.00, 0.05, viewDistanceToDepth);
	foamCoeff *= foamSub;

	vec3 foamAlbedo = pow(vec3(0.4, 0.4, 0.4), vec3(0.35));
	vec3 foamDirect = getSunColor(fragmentPosition.xyz, vec3(0.0, -1.0, 0.0), foamAlbedo, 0.8, 0.0, viewToSurface);
	vec3 foamIndirect = getIrradiance(vec3(0.0, -1.0, 0.0)) * foamAlbedo;
	vec3 foamColor =  foamDirect + foamIndirect;
	vec3 foamAlbedo2 = mix(foamColor, outColor.rgb*foamColor, 0.75);
	
	outColor.rgb = mix(outColor.rgb, foamAlbedo2, foamCoeff*0.45);


	vec4 hx = getHex(fragmentPosition.xz);
	float hd = hex(hx.xy);
	float hcf = pow(smoothstep(0.475, 0.5, hd), 0.25);
	outColor.rgb = mix(outColor.rgb, outColor.rgb *1.75, hcf * float(gridParams.x));


	outColor.a = 1.0;




}