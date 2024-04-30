


#include <shaders/common/renderersets.glsl>

// Begin Set2: Material
layout(set = MaterialSetIndex, binding = 0) uniform MaterialData
{
    vec4 albedo;
} material;

layout(set = MaterialSetIndex, binding = 1) uniform texture2D visibilityMask;
//layout(set = MaterialSetIndex, binding = 3) uniform texture2D visibilityMask;
// End Set2



float sampleHeight2(vec2 position)
{
    return pow(voronoi2d(position), 2.0);
}

float sampleFogHeight(vec2 position, float time)
{

    float timeCoeff = 0.7;
    float speed1 = 0.3*timeCoeff;
    vec2 dir1 = normalize(vec2(0.2, 0.4));
    vec2 pos1 = position + (dir1 * speed1 * time);
    float height1 = sampleHeight2(pos1 * 0.2) ;


    float speed2 = 0.4*timeCoeff;
    vec2 dir2 = normalize(vec2(-0.22, -0.3));
    vec2 pos2 = position + (dir2 * speed2 * time);
    float height2 = sampleHeight2(pos2*0.5) ;


    return mix(height1, height2, 0.2);
}


vec3 sampleFogNormal(vec2 position, float time)
{
    float eps = 0.4;
    float eps2 = eps * 2;
    vec3 off = vec3(1.0, 1.0, 0.0)* eps;
    float hL = sampleFogHeight(position.xy - off.xz, time);
    float hR = sampleFogHeight(position.xy + off.xz, time);
    float hD = sampleFogHeight(position.xy - off.zy, time);
    float hU = sampleFogHeight(position.xy + off.zy, time);

    return normalize(vec3(hR - hL, -eps2 , hU - hD));
}

vec3 undiscovered(float f,vec3 n, vec3 color, vec3 position, float depth)
{
    float fogSaturation = 0.75;
    float fogBrightness = 0.65;
    vec3 fogColor = vec3(250, 187, 107) / 255.0;
    float fogDustiness = 0.4;
    
    vec3 fogColorDesaturate = vec3(dot(fogColor, vec3(1.0/3.0)));
    fogColor = mix(fogColorDesaturate, fogColor, fogSaturation) * fogBrightness;

    vec3 irr_back = getIrradiance(-n);
    vec3 irr_front = getIrradiance(n);

    float ratio = 0.5;
    float ratio_half = ratio / 2.0;

    vec3 back_actual = irr_back * (1.0 - ratio_half) + irr_front * ratio_half;
    vec3 front_actual = irr_front * (1.0 - ratio_half) + irr_back * ratio_half;

    vec3 irradiance =  mix(back_actual, front_actual, pow(f, 2.0));

    vec3 fogDiffuse =  mix(irradiance, irradiance * fogColor, fogDustiness);

	vec3 lightVector = normalize(renderer.sun1.xyz);
    vec3 ndotl = vec3(clamp(dot(n, -lightVector), 0.0, 1.0)) * renderer.sun2.xyz * renderer.sun2.w;

    vec3 fogResult = fogDiffuse * 0.5 + (fogColor * ndotl *0.5);

    float heightCoeff = smoothstep(0.0, 1.0, depth);
    vec3 undis = mix(color, fogResult, heightCoeff);

    return undis;
}

/*vec3 outOfSight(float f, vec3 n, vec3 color, vec3 position, vec3 vis, float depth) 
{

    vec3 tint = vec3(250, 187, 107) / 255.0 * 0.75;
    float y = -position.y + f;
    vec3 gg = (1.0 - f) * tint * 0.15;

    float heightCoeff = clamp(smoothstep(0.4, 0.6, y), 0.0, 1.0);
    vec3 undis = mix(gg, color, heightCoeff );
    
	vec3 l = normalize(renderer.sun1.xyz);
    vec3 ndotl = vec3(clamp(dot(n, -l), 0.0, 1.0));
    undis += ndotl * tint * renderer.sun2.xyz * renderer.sun2.w * 0.2;

    return mix( mix(undis, color, 0.2), color, vis.y*1.0);
}*/

vec3 fogOfWar(vec3 color, vec3 position, vec3 vis, float time, float depth, float fogHeight)
{
    vec3 fn = sampleFogNormal(position.xz, time);
	
    //vec3 oos = outOfSight(fogHeight, fn, color, position, vis, depth);
    vec3 und = undiscovered(fogHeight, fn, color, position, mix(depth, depth *0.65, vis.x));

    //return mix(und, oos, vis.x);
    return mix(und, color, vis.y);
}