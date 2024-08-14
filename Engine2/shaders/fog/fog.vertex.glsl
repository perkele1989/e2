#include <shaders/header.glsl>

// Vertex attributes 
in vec4 vertexPosition;

// Fragment attributes

out vec4 fragmentPosition;

#include <shaders/fog/fog.common.glsl>

void main()
{
	vec4 worldPos = mesh.modelMatrix * vertexPosition;
	float f = 1.0 - sampleFogHeight(worldPos.xz, renderer.time.x);

	vec4 vertexPos = vertexPosition;
	//vertexPos.y -= (f * 1.0) - 1.0;

	fragmentPosition = mesh.modelMatrix * vertexPos;


	fragmentPosition.y -= f;

	gl_Position = renderer.projectionMatrix * renderer.viewMatrix * mesh.modelMatrix * vertexPos;

	
	

	
}