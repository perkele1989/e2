
in vec3 fragmentPosition;

#if defined(LW_HasNormal)
in vec3 fragmentNormal;
#endif 

#if defined(LW_HasTangent)
in vec3 fragmentTangent;
#endif 

#if defined(LW_HasUv0)
in vec2 fragmentUv0;
#endif


out vec4 output;