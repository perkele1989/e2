#include <shaders/header.glsl>

// Fragment attributes

#if !defined(Renderer_Shadow)
in vec4 fragmentPosition;

#if defined(Vertex_TexCoords01)
in vec4 fragmentUv01;
#endif

#if defined(Vertex_TexCoords23)
in vec4 fragmentUv23;
#endif 

#if defined(Vertex_Normals)
in vec3 fragmentNormal;
in vec4 fragmentTangent;
#endif

#if defined(Vertex_Color)
in vec4 fragmentColor;
#endif

// Out color
out vec4 outColor;
out vec4 outPosition;

#endif

#include <shaders/common/renderersets.glsl>

%CustomDescriptorSets%

void main()
{
#if !defined(Renderer_Shadow)
	outPosition = fragmentPosition;
	outColor = vec4(1.0, 1.0, 1.0, 1.0);

#if defined(Vertex_TexCoords01)
	vec4 uv = fragmentUv01.xyzw;
#else 
	vec4 uv = vec4(fragmentPosition.xz, 0.0, 0.0);
#endif 

    vec4 color = material.color.rgba;
	float ss = sampleSimplex(fragmentPosition.xz*1.0 + renderer.time.x * 1.0);

    vec3 greenGlow = vec3(0.739474, 1.0, 0.102378);
    vec3 redGlow = vec3(1.0, 0.15539474, 0.05102378);
    redGlow = mix(redGlow, greenGlow, 0.25);

    float x = uv.x;
    float y = uv.y;
    float dist = uv.z;

    float offSpeed = 9.0;
    float signalSpeed = 9.0;
    float onSpeed = 15.0;

    // outColor = vec4(color.r, color.g, 0.0, 1.0);
    // return;
    float speed = mix(mix(offSpeed, signalSpeed, color.g), onSpeed, color.r);

    float ySin = sin((dist*50.0) - renderer.time.x * speed) * 0.5 + 0.5;
    float ySin2 = ySin* sin((dist*30.0) - renderer.time.x * speed*1.15) * 0.5 + 0.5;
    float xCenter = smoothstep(ySin2, 1.0, 1.0 - abs(x));
    float xCenter2 = smoothstep(0.0, ySin, 1.0 - abs(x));


    // online color 
    vec4 onlineColor = vec4(greenGlow*(1.0 + ySin2), xCenter2);

    // offline color 
    vec4 offlineColor = vec4(0.0, 0.0, 0.0, xCenter*0.75);

    // signal color 
    vec4 signalColor = vec4(redGlow*(1.0 + ySin2), xCenter2);
    // signalColor.rgb = mix(offlineColor.rgb, signalColor.rgb, signalColor.a); 
    // signalColor.a = clamp(offlineColor.a + signalColor.a, 0.0, 1.0);

    

    outColor = mix(mix(offlineColor, signalColor, color.g), onlineColor, color.r);
    //outColor.a = 0.5 + color.r * 0.5;


#endif
}