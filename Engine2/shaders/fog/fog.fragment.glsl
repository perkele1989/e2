#include <shaders/header.glsl>

// Fragment attributes

in vec4 fragmentPosition;

// Out color
out vec4 outColor;
out vec4 outPosition;

#include <shaders/fog/fog.common.glsl>

void main()
{
	outPosition = fragmentPosition;
	outColor = vec4(0.0, 0.0, 0.0, 1.0);

	vec3 frontBuffer = textureLod(sampler2D(frontBufferColor, clampSampler), gl_FragCoord.xy / vec2(resolution.x, resolution.y), 0).xyz;
	outColor.rgb = frontBuffer;

	vec3 frontPosition = textureLod(sampler2D(frontBufferPosition, clampSampler), gl_FragCoord.xy / vec2(resolution.x, resolution.y), 0).xyz;


	vec3 visibility = textureLod(sampler2D(visibilityMask, clampSampler), gl_FragCoord.xy / vec2(resolution.x, resolution.y), 0).xyz;

	
    //vec3 visibility = textureLod(sampler2D(visibilityMask, clampSampler), gl_FragCoord.xy / vec2(resolution.x, resolution.y), 0).xyz;
    //outColor.rgb = fogOfWar(outColor.rgb, fragmentPosition.xyz, visibility, renderer.time.x);
	outColor.rgb = fogOfWar(outColor.rgb, frontPosition, visibility, renderer.time.x) ;

}

// vec3 fogOfWar(vec3 color, vec3 position, vec3 vis, float time)