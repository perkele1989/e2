#version 460 core

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
    vec3 visibility = textureLod(sampler2D(visibilityMask, frontBufferSampler), gl_FragCoord.xy / vec2(resolution.x, resolution.y), 0).xyz;
    //float time = mix(0.0, renderer.time.x, visibility.x);
    float time = renderer.time.x;

	outPosition = fragmentPosition;
	outColor = vec4(1.0, 1.0, 1.0, 1.0);

	float h = sampleWaterHeight(fragmentPosition.xz, time);
	float d = sampleWaterDepth(fragmentPosition.xz);

	vec3 n = sampleWaterNormal(fragmentPosition.xz, time);
	
	vec3 l = normalize(vec3(-1.0, -1.0, -1.0));
	vec3 ndotl = vec3(clamp(dot(n, l), 0.0, 1.0));
	//vec3 softl = vec3(dot(n,l) *0.5 + 0.5);

	vec3 v = normalize(fragmentPosition.xyz - (inverse(renderer.viewMatrix) * vec4(0.0, 0.0, 0.0, 1.0)).xyz);


	vec3 darkWater = vec3(0.0, 80.0, 107.0) / 255.0;
	vec3 lightWater = vec3(28.0, 255.0, 255.0) / 255.0;
	vec3 superLightWater = vec3(1.0, 1.0 ,1.0);//pow(lightWater, vec3(110.1));

	vec3 shoreDark  = pow(darkWater, vec3(1.2));
	vec3 oceanDark  = pow(darkWater, vec3(2.4));

	//vec3 finalDark = mix(shoreDark, oceanDark, d);
    vec3 finalDark = oceanDark;
	//vec3 finalLight = mix(pow(lightWater, vec3(0.4)), lightWater, d);
    vec3 finalLight = lightWater;

	//vec3 finalLight = lightWater;

	vec3 r = reflect(v, n);

	vec3 hdr = texture(sampler2D(reflectionHdr, genericSampler), equirectangularUv(r)).rgb;


	vec3 frontBuffer = texture(sampler2D(frontBufferColor, frontBufferSampler), gl_FragCoord.xy / vec2(resolution.x, resolution.y)).rgb;
	vec3 frontPosition = texture(sampler2D(frontBufferPosition, frontBufferSampler), gl_FragCoord.xy / vec2(resolution.x, resolution.y)).xyz;

	float viewDistanceToDepth = distance(frontPosition, fragmentPosition.xyz);
	float viewDepthCoeff = 1.0 - smoothstep(0.0, 0.2, viewDistanceToDepth);
	float viewDepthCoeff2 = 1.0 - smoothstep(0.0, 1.0, viewDistanceToDepth);

    
	float timeSin = (sin(time) * 0.5 + 0.5);
	float timeSin2 = 1.0 - (sin( (time + timeSin * 0.3) * 1.2) * 0.5 + 0.5);

	float viewDepthCoeffFoam = 1.0 - smoothstep(0.0,  (0.33 + timeSin2 * 0.2 ) * pow((simplex(fragmentPosition.xz * 4.0 + vec2(cos(time * 0.25), sin(time*0.2))) * 0.5 + 0.5), 1.0), viewDistanceToDepth);

	vec3 baseColor = mix(finalDark, finalLight,  pow(h, 1.4) * (ndotl * 0.5 + 0.5) );
	vec3 dimBaseColor = mix(baseColor * frontBuffer, frontBuffer, 0.05);
	baseColor = mix(baseColor, dimBaseColor, viewDepthCoeff2);
	//baseColor = mix(baseColor, frontBuffer, viewDepthCoeff);
	
    baseColor = mix(baseColor, mix(baseColor, frontBuffer, 0.3), pow(1.0-d, 0.4));

	vec3 foamColor = vec3(0.867, 0.89, 0.9);
	baseColor = mix(baseColor, foamColor, (pow(viewDepthCoeffFoam, 0.25)*0.75));

	outColor.rgb =vec3(0.0, 0.0, 0.0);

	// basecolor
	outColor.rgb +=  baseColor;

	float vdotn = pow(clamp(-dot(v, n), 0.0, 1.0), 4.0);

	// fresnel reflection
	outColor.rgb += hdr * vdotn * 0.15;

	// reflection 
	outColor.rgb += hdr * 0.02;

    float va = smoothstep(0.00, 0.5, visibility.x);
    float vb = smoothstep(1.0 - 0.5, 1.0 , visibility.x);
    float vc = va - vb;

    //outColor.rgb = vec3(vc);
    //outColor.rgb = mix(outColor.rgb, outColor.rgb * 0.05, vc);
    
    outColor.rgb = fogOfWar(outColor.rgb, fragmentPosition.xyz, visibility, renderer.time.x);
    //outColor.rgb = mix()

	// debug normal 
	//outColor.rgb = clamp(vec3(n.x, n.z, -n.y) * 0.5 + 0.5, vec3(0.0), vec3(1.0));

	//outColor.rgb = vec3(viewDepthCoeff);

	outColor.rgb = clamp(outColor.rgb, vec3(0.0), vec3(1.0));
	outColor.a = 1.0; //clamp(pow( smoothstep(0.4, 0.5, d), 16.0), 0.0, 1.0);// * 0.2 + 0.8;

}