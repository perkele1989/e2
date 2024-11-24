#version 460 core

in vec4 vertexPosition;
out vec2 fragmentUv;

layout(push_constant) uniform ConstantData
{
    vec2 resolution;
    vec2 areaSize;
    vec2 areaCenter;

    vec2 position;
    float radius;
};

void main()
{
    vec2 areaCenterPixels = (areaCenter / areaSize) * resolution;
    vec2 topLeftPixels = areaCenterPixels - (resolution / 2.0);

    vec2 radiusPixels = (radius / areaSize) * resolution;
    
    vec2 positionPixels = (position / areaSize) * resolution;
    positionPixels -= topLeftPixels;
    positionPixels -= radiusPixels;

    vec2 quadPosition = positionPixels;
    vec2 quadSize = vec2(radiusPixels*2.0);

	fragmentUv = vertexPosition.xy;
	vec4 vertPos = vertexPosition;
	vertPos.xy = (vertPos.xy * quadSize) + (quadPosition);
	vertPos.xy = vertPos.xy * (1.0 / resolution);
	vertPos.xy = vertPos.xy * 2.0 - 1.0; // -1 -> +1
	vertPos.z = 0.0;
	vertPos.w = 1.0;

	gl_Position = vertPos;
}