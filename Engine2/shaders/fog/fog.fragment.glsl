#include <shaders/header.glsl>

// Fragment attributes

in vec4 fragmentPosition;

// Out color
out vec4 outColor;
out vec4 outPosition;

#include <shaders/fog/fog.common.glsl>

void main()
{
	vec2 screenUv = getScreenPixelUV(gl_FragCoord.xy);
	vec3 frontColor = getFrontColor(screenUv);
	vec3 frontPosition = getFrontPosition(screenUv);

	float fogHeight = pow(1.0 -sampleFogHeight(frontPosition.xz,  renderer.time.x), 3.0);
	vec4 fragPos = vec4(frontPosition.xyz, 1.0);
	fragPos.y -= fogHeight;



	outPosition = fragmentPosition + vec4(0.0, -fogHeight, 0.0, 0.0);
	outColor = vec4(frontColor, 1.0);

    


	float depth = clamp(distance(frontPosition,fragmentPosition.xyz)/1.0, 0.0, 1.0); 
	depth = pow(depth, 0.25);

	vec3 visibility = textureLod(sampler2D(visibilityMask, clampSampler), screenUv, 0).xyz;

	outColor.rgb = fogOfWar(outColor.rgb,fragPos.xyz, visibility, renderer.time.x, depth, fogHeight);

	//outColor.rgb = vec3(pow(fogHeight,3.0));

	//outColor.rgb = vec3(renderer.time.x);

	// depth-related artifacts
	//outColor.rgb = vec3(voronoi2d(frontPosition.xz));
	//outColor.rgb = vec3(sampleHeight2(frontPosition.xz));
	//outColor.rgb = vec3(sampleFogHeight(frontPosition.xz, 0.0));
	//outColor.rgb = sampleFogNormal(frontPosition.xz, 0.0) *0.5 + 0.5; 

	// float h = sampleFogHeight(frontPosition.xz, 0.0);
	// vec3 n =  sampleFogNormal(frontPosition.xz, 0.0); 
	// vec3 u = undiscovered(h, n, frontColor, frontPosition, visibility);
	// vec3 o = outOfSight(h, n, frontColor, frontPosition, visibility);
	// vec3 fow = mix(u, o, visibility.x);
	// outColor.rgb = fow;

}

// vec3 fogOfWar(vec3 color, vec3 position, vec3 vis, float time)