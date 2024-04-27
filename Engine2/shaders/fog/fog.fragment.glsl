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

	outPosition = fragmentPosition;
	outColor = vec4(frontColor, 1.0);

	vec3 visibility = textureLod(sampler2D(visibilityMask, clampSampler), screenUv, 0).xyz;

	outColor.rgb = fogOfWar(outColor.rgb, frontPosition, visibility, renderer.time.x);

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