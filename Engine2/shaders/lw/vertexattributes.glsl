
in vec3 position;

#if defined(LW_HasNormal)
in vec3 normal;
#endif

#if defined(LW_HasTangent)
in vec3 tangent;
#endif

#if defined(LW_HasUv0)
in vec2 uv0;
#endif

#if defined(LW_HasWeights)
in vec4 boneWeights;
in uvec4 boneIds;
#endif


out vec3 fragmentPosition;

#if defined(LW_HasNormal)
out vec3 fragmentNormal;
#endif 

#if defined(LW_HasTangent)
out vec3 fragmentTangent;
#endif 

#if defined(LW_HasUv0)
out vec2 fragmentUv0;
#endif

void applyVertexAttributes(mat4 mvpMatrix, mat4 mvMatrix, mat4 nMatrix)
{
    // @todo skinning ,instancing etc

    gl_Position = mvpMatrix * position;
    fragmentPosition = mvMatrix * position;

    #if defined(LW_HasUv0)
    fragmentUv0 = uv0;
    #endif 

    #if defined(LW_HasNormal)
    fragmentNormal = nMatrix * normal;
    #endif

    if defined(LW_HasTangent)
    fragmentTangent = nMatrix * tangent;
    #endif
}