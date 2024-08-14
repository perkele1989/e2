#include <shaders/header.glsl>
#define EPSILON 1e-5

in vec2 inUv;
out vec4 outColor;

// Push constants
layout(push_constant) uniform ConstantData
{
	vec2 viewCornerTL;
	vec2 viewCornerTR;
	vec2 viewCornerBL;
	vec2 viewCornerBR;

    vec2 worldMin;
    vec2 worldMax;

	uvec2 resolution;
};

layout(set = 0, binding = 0) uniform texture2D visibilityMask;
layout(set = 0, binding = 1) uniform texture2D unitsTexture;
layout(set = 0, binding = 2) uniform texture2D waterTexture;
layout(set = 0, binding = 3) uniform texture2D forestTexture;
layout(set = 0, binding = 4) uniform texture2D mountainTexture;
layout(set = 0, binding = 5) uniform sampler clampSampler;
layout(set = 0, binding = 6) uniform sampler repeatSampler;



float lineSegment(vec2 p, vec2 a, vec2 b)
{
    float renderSize = max(resolution.x, resolution.y);
    float worldSize = max(worldMax.x - worldMin.x, worldMax.y - worldMin.y);

    float renderThickness = 1.5;

    float worldThickness = (renderThickness / renderSize) * worldSize;

    vec2 pa = p - a;
    vec2 ba = b - a;

    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    // ????????
    float idk = length(pa - ba*h);

    return 1.0 - clamp(smoothstep(0.0, worldThickness, idk), 0.0, 1.0);
}

#include <shaders/common/utils.glsl>

void main()
{
	vec4 bg = vec4(0.11372549019, 0.14509803921, 0.19215686274, 0.8);
	bg.rgb = pow(bg.rgb, vec3(2.2));

    float vis = texture(sampler2D(visibilityMask, clampSampler), inUv).r;
	vec4 units = texture(sampler2D(unitsTexture, clampSampler), inUv).rgba;
    vec2 worldSize = worldMax - worldMin;
	vec2 position = worldMin + inUv * worldSize;//* zoom;

//	float h = pow(sampleSimplex(position, 0.05), 0.5);
	
	vec4 water = texture(sampler2D(waterTexture, repeatSampler), position/200.0);
	float w_a = water.a;
	water.a = 1.0;

	vec4 forest = texture(sampler2D(forestTexture, repeatSampler), position/50.0);
	float f_a = forest.a;
	forest.a = 1.0;
	
	vec4 mountain = texture(sampler2D(mountainTexture, repeatSampler), position/50.0);
	float m_a = mountain.a;
	mountain.a = 1.0;

	float h = sampleBaseHeight(position * 1.05);
	//h = h3;
	float m = 0.0;
	float g = 0.0;
	float s = 0.0;
	float o = 0.0;

	if(h > 0.90)
		m = 1.0;
	else if(h > 0.75)
		g = 1.0;
	else if (h > 0.57)
		s = 1.0;
	else 
		o = 1.0;

	float f = sampleSimplex((position + vec2(32.14, 29.28)) * 4.0);
	if(f > 0.4 && h > 0.6)
		f = 1.0;
	else
		f = 0.0;

	f = f * g;
	g = g - f;

	vec4 m_C = mix(bg, mountain, m_a);//vec4(0.6, 0.6, 0.6, 0.9);
	vec4 g_C = bg;//vec4(0.2, 0.2, 0.2, 0.9);
	vec4 o_C = mix(vec4(bg.rgb, 0.7), water, w_a);//vec4(0.0, 0.0, 0.0, 0.5);
	vec4 f_C =  mix(bg, forest, f_a);//vec4(0.4, 0.4, 0.4, 0.9);


	float wc = h;
	float woutlineWidth = 0.02;
	float woutlineSoftness = 0.0005;
	float woutlineCenter = 0.75;
	float woutlineCoeff = smoothstep(woutlineCenter - (woutlineWidth / 2.0),  woutlineCenter - (woutlineWidth / 2.0) + woutlineSoftness, wc);
	float bby = woutlineCoeff;
	woutlineCoeff = woutlineCoeff - smoothstep( woutlineCenter + (woutlineWidth / 2.0) - woutlineSoftness , woutlineCenter + (woutlineWidth / 2.0), wc);

	outColor.rgba = bg;

	// terrain color
	outColor.rgba = mix(o_C, outColor.rgba, bby);
	//outColor.rgba = mix(outColor.rgba, g_C, g);
	outColor.rgba = mix(outColor.rgba, m_C, m);
	//outColor.rgba = mix(outColor.rgba, f_C, f);


	outColor.rgba = mix(outColor.rgba, vec4(1.0, 1.0, 1.0, 1.0), woutlineCoeff*0.5);

	// units
	outColor.rgba = mix(outColor.rgba, units.rgba, units.a);

	// cull visibility 
    outColor.rgba = mix(outColor.rgba, vec4(bg.rgb, 0.95), 1.0 - vis);

	// visibility outline
	// float outlineWidth = 0.1;
	// float outlineSoftness = 0.04;
	// float outlineCenter = 0.3;
	// float outlineCoeff = smoothstep(outlineCenter - (outlineWidth / 2.0),  outlineCenter - (outlineWidth / 2.0) + outlineSoftness, vis);
	// outlineCoeff = outlineCoeff - smoothstep( outlineCenter + (outlineWidth / 2.0) - outlineSoftness , outlineCenter + (outlineWidth / 2.0), vis);
	// outColor.rgba = mix(outColor.rgba, vec4(1.0, 1.0, 1.0, 0.4), outlineCoeff * 0.2);

	// frustum
	float lineStrength = 0.15 + smoothstep(0.5, 0.51, vis) * 0.85;
	outColor.rgba = mix(outColor.rgba, vec4(1.0, 1.0, 1.0, lineStrength), lineSegment(position,viewCornerTL, viewCornerTR ));
    outColor.rgba = mix(outColor.rgba, vec4(1.0, 1.0, 1.0, lineStrength), lineSegment(position,viewCornerTR, viewCornerBR ));
    outColor.rgba = mix(outColor.rgba, vec4(1.0, 1.0, 1.0, lineStrength), lineSegment(position,viewCornerBR, viewCornerBL ));
    outColor.rgba = mix(outColor.rgba, vec4(1.0, 1.0, 1.0, lineStrength), lineSegment(position,viewCornerBL, viewCornerTL ));
	outColor.a *= 0.9;
}