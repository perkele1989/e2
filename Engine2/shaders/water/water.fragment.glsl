#include <shaders/header.glsl>
// Fragment attributes

in vec4 fragmentPosition;
in vec4 fragmentPosition2;

in vec3 fragmentNormal;

// Out color
out vec4 outColor;
out vec4 outPosition;

#include <shaders/water/water.common.glsl>

void main()
{
	vec2 screenUv = getScreenPixelUV(gl_FragCoord.xy);
	vec3 frontColor = getFrontColor(screenUv);
	vec3 frontPosition = getFrontPosition(screenUv);

	vec4 vis = texture(sampler2D(visibilityMask, clampSampler), screenUv).rgba;

	vec4 outline = texture(sampler2D(outlineTexture, clampSampler),  screenUv).rgba;

    float time = renderer.time.x;

	outPosition = fragmentPosition;
	outColor = vec4(1.0, 1.0, 1.0, 1.0);

	float h = sampleWaterHeight(fragmentPosition.xz, time);
	//float d = sampleWaterDepth(fragmentPosition.xz);
	//float h =0.2;
	//float d = 0.2;

	h *= mix(0.7, 1.0, vis.w);

	vec3 n = sampleWaterNormal(fragmentPosition.xz, time);
	//n.y *= mix(8.0, 1.0, vis.w);
	//n = normalize(n);



	//vec3 n = vec3(0.0, -1.0, 0.0);
	float sunShadow = calculateSunShadow(fragmentPosition);
	float sunDelta =  sunShadow * renderer.sun2.w;
	vec3 l = normalize(renderer.sun1.xyz);
	vec3 ndotl = vec3(clamp(dot(n, -l), 0.0, 1.0));

	//ndotl = vec3(getSunDelta(fragmentPosition.xyz, n));

	vec3 v = normalize(fragmentPosition.xyz - (inverse(renderer.viewMatrix) * vec4(0.0, 0.0, 0.0, 1.0)).xyz);


	vec3 lightWater = vec3(28.0, 200.0, 255.0) / 255.0;

	vec3 darkWater = vec3(0.0, 60.0, 107.0) / 255.0;
	float shP = mix(1.7, 2.4, vis.w);
	vec3 shoreDark  = pow(darkWater, vec3(shP));

	//shoreDark *= mix(1.0, 0.5, 1.0 - vis.w);
	lightWater = mix(lightWater*1.5, darkWater*2, vis.w);
	
	vec3 r = reflect(v, n);

	vec3 hdr = texture(sampler2D(reflectionHdr, equirectSampler), equirectangularUv(r)).rgb;


	float viewDepthScalar = mix(0.5, 1.5, 1.0 - vis.w);

	float viewDistanceToDepth = distance(frontPosition, fragmentPosition.xyz);
	float viewDepthCoeff = 1.0 - smoothstep(0.0, 0.2, viewDistanceToDepth);
	float viewDepthCoeff2 = 1.0 - smoothstep(0.0, viewDepthScalar, viewDistanceToDepth);

    
	float timeSin = (sin(time) * 0.5 + 0.5);
	float timeSin2 = 1.0 - (sin( (time + timeSin * 0.3) * 1.2) * 0.5 + 0.5);

	float viewDepthCoeffFoam = 1.0 - smoothstep(0.0,  (0.15 + timeSin2 * 0.2 ) * pow((simplex(fragmentPosition.xz * 4.0 + vec2(cos(time * 0.25), sin(time*0.2))) * 0.5 + 0.5), 1.0), viewDistanceToDepth);
	//float viewDepthCoeffFoam = 1.0;

	vec3 baseColor = mix(shoreDark, lightWater,  pow(h, 1.4) * (ndotl * 0.5 + 0.5) * sunShadow );
	

	vec3 dimBaseColor = mix(baseColor * frontColor, frontColor, 0.2);
	baseColor = mix(baseColor, dimBaseColor, viewDepthCoeff2*0.75);

	vec3 foamColor = vec3(0.867, 0.89, 0.9);
	baseColor = mix(baseColor, foamColor, (pow(viewDepthCoeffFoam, 0.25)*0.75));


	//baseColor = shoreDark + lightWater * pow(h, 1.4);

	//baseColor = shoreDark  * (ndotl);


	float ibl = renderer.ibl1.x * 3.0;
	

	//sun += sun * 0.5 + ibl * 0.5;
	//float ibl = 

	// basecolor
	outColor.rgb = (baseColor * ibl );

	float vdotn = pow(clamp(-dot(v, n), 0.0, 1.0), 4.0);

    float reflCoeff = smoothstep(0.0, 0.35, h);

	// fresnel reflection
	outColor.rgb += hdr * vdotn * 0.15 * reflCoeff * (sunDelta * 0.333);


	float reflCoeff2 = mix(1.0, reflCoeff, vis.w);
	outColor.rgb += hdr *  0.04 * reflCoeff * (sunDelta * 0.333);


	// debug normal 
	//outColor.rgb = clamp(vec3(n.x, n.z, -n.y) * 0.5 + 0.5, vec3(0.0), vec3(1.0));


	outColor.rgb = mix(outColor.rgb, outline.rgb, outline.a*0.25);


	outColor.rgb = clamp(outColor.rgb, vec3(0.0), vec3(1.0));
	outColor.a = 1.0;

}