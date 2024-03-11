#version 460 core
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
layout(set = 0, binding = 1) uniform sampler clampSampler;



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
    float vis = texture(sampler2D(visibilityMask, clampSampler), inUv).r;
    vec2 worldSize = worldMax - worldMin;
	vec2 position = worldMin + inUv * worldSize;//* zoom;

//	float h = pow(sampleSimplex(position, 0.05), 0.5);
	


	float h = sampleBaseHeight(position);
	//h = h3;
	float m = 0.0;
	float g = 0.0;
	float s = 0.0;
	float o = 0.0;

	if(h > 0.81)
		m = 1.0;
	else if(h > 0.39)
		g = 1.0;
	else if (h > 0.03)
		s = 1.0;
	else 
		o = 1.0;

	float f = sampleBaseHeight((position + vec2(321.4, 2928.0)) * 4);
	if(f > 0.2 && h > 0.6)
		f = 1.0;
	else
		f = 0.0;

	f = f * g;
	g = g - f;

	vec4 m_C = vec4(0.6, 0.6, 0.6, 0.9);
	vec4 g_C = vec4(0.2, 0.2, 0.2, 0.9);
	vec4 s_C = vec4(0.03, 0.03, 0.03, 0.5);
	vec4 o_C = vec4(0.0, 0.0, 0.0, 0.5);
	vec4 f_C = vec4(0.4, 0.4, 0.4, 0.9);

	outColor.rgba = o_C;
	outColor.rgba = mix(outColor.rgba, s_C, s);
	outColor.rgba = mix(outColor.rgba, g_C, g);
	outColor.rgba = mix(outColor.rgba, m_C, m);
	outColor.rgba = mix(outColor.rgba, f_C, f);

    outColor.rgba = mix(outColor.rgba, vec4(0.0, 0.0, 0.0, 0.0), 1.0 - vis);

	float outlineWidth = 0.1;
	float outlineSoftness = 0.04;
	float outlineCenter = 0.3;
	float outlineCoeff = smoothstep(outlineCenter - (outlineWidth / 2.0),  outlineCenter - (outlineWidth / 2.0) + outlineSoftness, vis);
	outlineCoeff = outlineCoeff - smoothstep( outlineCenter + (outlineWidth / 2.0) - outlineSoftness , outlineCenter + (outlineWidth / 2.0), vis);
	outColor.rgba = mix(outColor.rgba, vec4(1.0, 1.0, 1.0, 0.4), outlineCoeff * 0.2);

	outColor.rgba = mix(outColor.rgba, vec4(1.0, 1.0, 1.0, 1.0), lineSegment(position,viewCornerTL, viewCornerTR ));
    outColor.rgba = mix(outColor.rgba, vec4(1.0, 1.0, 1.0, 1.0), lineSegment(position,viewCornerTR, viewCornerBR ));
    outColor.rgba = mix(outColor.rgba, vec4(1.0, 1.0, 1.0, 1.0), lineSegment(position,viewCornerBR, viewCornerBL ));
    outColor.rgba = mix(outColor.rgba, vec4(1.0, 1.0, 1.0, 1.0), lineSegment(position,viewCornerBL, viewCornerTL ));
}