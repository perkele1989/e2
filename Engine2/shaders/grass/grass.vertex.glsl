#include <shaders/header.glsl>

// Vertex attributes 
in vec4 vertexPosition;

#if defined(Vertex_Normals)
in vec4 vertexNormal;
in vec4 vertexTangent;
#endif

#if defined(Vertex_TexCoords01)
in vec4 vertexUv01;
#endif 

#if defined(Vertex_TexCoords23)
in vec4 vertexUv23;
#endif 

#if defined(Vertex_Color)
in vec4 vertexColor;
#endif

// Fragment attributes

#if !defined(Renderer_Shadow)
out vec4 fragmentPosition;
out vec4 rootPosition;

#if defined(Vertex_TexCoords01)
out vec4 fragmentUv01;
#endif

#if defined(Vertex_TexCoords23)
out vec4 fragmentUv23;
#endif 

#if defined(Vertex_Normals)
out vec3 fragmentNormal;
out vec4 fragmentTangent;
#endif

#if defined(Vertex_Color)
out vec4 fragmentColor;
#endif
#endif

#include <shaders/common/renderersets.glsl>

%CustomDescriptorSets%


/*
			float grassCoeff = sampleSimplex((rootPosition.xz + vec2(32.14f, 29.28f)) * 7.5f);
			grassCoeff = 1.0 - smoothstep(0.4, 0.6, grassCoeff);

*/
void main()
{
	vec4 meshVertex = vec4(vertexPosition.xyz, 1.0);
	vec4 meshRoot = vec4(vertexUv01.x, 0.0, vertexUv01.y, 1.0);
	vec4 worldRoot = mesh.modelMatrix * meshRoot;
	
#if !defined(Renderer_Shadow)
	rootPosition = worldRoot;
#endif

	vec2 areaResolution = material.areaParams.xx;
	vec2 areaSize = material.areaParams.yy;
	vec2 areaCenter = material.areaParams.zw;
	vec2 areaTopLeft = areaCenter - (areaSize * 0.5);
	vec2 areaUv = (worldRoot.xz - areaTopLeft) / areaSize; 

	float areaHeight = 1.0;
	vec2 areaOffset = vec2(15.0, 15.0);
	float areaOffsetLength = 15.0;
	if(areaUv.x >= 0.0 && areaUv.x <= 1.0 && areaUv.y >= 0.0 && areaUv.y <= 1.0)
	{
		vec4 areaMap = texture(sampler2D(customTexture_cutMask, clampSampler), areaUv).rgba;
		areaHeight = areaMap.r;
		areaOffset = areaMap.gb;
		areaOffsetLength = length(areaOffset);
	}
	
	float grassCoeff = sampleSimplex((worldRoot.xz + vec2(32.16, 64.32)) * 0.135);
	grassCoeff = pow(smoothstep(0.35, 0.40, grassCoeff), 4.2);

	float randomCoeff = sampleSimplex((worldRoot.xz + vec2(32.16, 64.32)) * 1.135);

	meshVertex.x = mix(meshRoot.x, meshVertex.x, 0.85);
	meshVertex.y = mix(meshRoot.y, meshVertex.y, 0.85 + 0.15 * randomCoeff);
	meshVertex.xyz = mix(meshRoot.xyz, meshVertex.xyz, grassCoeff);

	meshVertex.xyz = mix(meshRoot.xyz, meshVertex.xyz, areaHeight *0.5 + 0.5);
	meshVertex.y *= areaHeight*0.5 + 0.5;

#if !defined(Renderer_Shadow)
	vec4 animatedVertexNormal = vec4(vertexNormal.xyz, 0.0);
	vec4 animatedVertexTangent = vec4(vertexTangent.xyz, 0.0);
	float tangentSign = vertexTangent.w;
#endif

	vec4 worldVertex = mesh.modelMatrix * meshVertex;
	

#if !defined(Renderer_Shadow)
	vec4 time = renderer.time;
#else 
	vec4 time = shadowTime;
#endif

	float ss = sampleSimplex(worldRoot.xz * 0.5) * 0.5 + 0.5;

	float heightCoeff = smoothstep(-0.185, 0.0, worldVertex.y);

	worldVertex.x += cos(time.x*1.3 + ss*20.0) * mix(0.065 * ss * 0.45, 0.0, heightCoeff);
	worldVertex.z += sin(time.x*0.7 + ss*40.0) * mix(0.073 * ss * 0.45, 0.0, heightCoeff);

#if !defined(Renderer_Shadow)
	vec2 ltp = playerPosition - worldRoot.xz;
	float ltpDistance = length(ltp);
	ltp = normalize(ltp);

	ltp = normalize(-areaOffset);
	ltpDistance = areaOffsetLength;

	float ss2 = sampleSimplex(worldRoot.xz * 10.5) * 0.75 + 0.25;

	float heightCoeff2 = smoothstep(0.05, 0.1, -worldVertex.y);
	float ltpCoeff = mix(0.0, 0.0755, 1.0 - clamp(ltpDistance*3.0, 0.0, 1.0));
	worldVertex.xz += -ltp * ltpCoeff * heightCoeff2 * ss2;

	float ltpCoeff2 = 1.0 - clamp(ltpDistance*8.0, 0.0, 1.0);
	worldVertex.y = mix(worldVertex.y, worldVertex.y*0.5, ltpCoeff2 * heightCoeff2);

	gl_Position = renderer.projectionMatrix * renderer.viewMatrix * worldVertex;

#else 
	gl_Position = shadowViewProjection * worldVertex;
#endif

#if !defined(Renderer_Shadow)
	fragmentPosition = worldVertex;
#if defined(Vertex_Normals)

	fragmentNormal = normalize(mesh.modelMatrix * normalize(animatedVertexNormal)).xyz;
	fragmentTangent.xyz =  normalize(mesh.modelMatrix * normalize(animatedVertexTangent)).xyz;
    fragmentTangent.w = tangentSign;
	//fragmentBitangent = normalize(cross(fragmentNormal.xyz, fragmentTangent.xyz));
#endif

#if defined(Vertex_TexCoords01)
	fragmentUv01 = vertexUv01;
#endif

#if defined(Vertex_TexCoords23)
	fragmentUv23 = vertexUv23;
#endif

#if defined(Vertex_Color)
	fragmentColor = vertexColor;
#endif
#endif
}