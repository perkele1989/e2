

#define EPSILON 1e-5

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 equirectangularUv(vec3 direction)
{
    //direction = normalize(direction);
    vec2 uv = vec2(atan(direction.z, direction.x), asin(-direction.y));
    uv *= invAtan;
    uv += 0.5;

    uv.y = 1.0 - uv.y;


    //uv = clamp(uv, vec2(0.01), vec2(0.99));


    return uv;
}

const mat2 myt = mat2(.12121212, .13131313, -.13131313, .12121212);
const vec2 mys = vec2(1e4, 1e6);

vec2 rhash(vec2 uv) {
    uv *= myt;
    uv *= mys;
    return fract(fract(uv / mys) * uv);
}

vec3 hash(vec3 p) {
    return fract(sin(vec3(dot(p, vec3(1.0, 57.0, 113.0)),
                        dot(p, vec3(57.0, 113.0, 1.0)),
                        dot(p, vec3(113.0, 1.0, 57.0)))) *
                43758.5453);
}

float voronoi2d(const in vec2 point)
{
    vec2 p = floor(point);
    vec2 f = fract(point);
    float res = 0.0;
    for (int j = -1; j <= 1; j++) {
        for (int i = -1; i <= 1; i++) {
            vec2 b = vec2(i, j);
            vec2 r = vec2(b) - f + rhash(p + b);
            res += 1. / pow(dot(r, r), 8.);
        }
    }
    return pow(1. / res, 0.0625);
}


vec3 permute(vec3 x)
{
	return mod(((x*34.0)+1.0)*x, 289.0);
}

float simplex(vec2 v){
		const vec4 C = vec4(0.211324865405187, 0.366025403784439, -0.577350269189626, 0.024390243902439);
		vec2 i  = floor(v + dot(v, C.yy) );
		vec2 x0 = v -   i + dot(i, C.xx);
		vec2 i1;
		i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
		vec4 x12 = x0.xyxy + C.xxzz;
		x12.xy -= i1;
		i = mod(i, 289.0);
		vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
		+ i.x + vec3(0.0, i1.x, 1.0 ));
		vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy),
		dot(x12.zw,x12.zw)), 0.0);
		m = m*m ;
		m = m*m ;
		vec3 x = 2.0 * fract(p * C.www) - 1.0;
		vec3 h = abs(x) - 0.5;
		vec3 ox = floor(x + 0.5);
		vec3 a0 = x - ox;
		m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );
		vec3 g;
		g.x  = a0.x  * x0.x  + h.x  * x0.y;
		g.yz = a0.yz * x12.xz + h.yz * x12.yw;
		return 130.0 * dot(m, g);
}

float sampleSimplex(vec2 position)
{
	return simplex(position) * 0.5 + 0.5;
}



// vec3 shiftHue(vec3 col, float shift)
// {
//     vec3 P = vec3(0.55735)*dot(vec3(0.55735), col);
    
//     vec3 U = col-P;
    
//     vec3 V = cross(vec3(0.55735),U);    

//     col = U*cos(shift*6.2832) + V*sin(shift*6.2832) + P;
    
//     return col;
// }

vec3 shiftHue(vec3 color, float hue) {
    const vec3 k = vec3(0.57735, 0.57735, 0.57735);
    float cosAngle = cos(hue);
    return clamp(vec3(color * cosAngle + cross(k, color) * sin(hue) + k * dot(k, color) * (1.0 - cosAngle)), vec3(0.0), vec3(1.0));
}


float sampleBaseHeight(vec2 position)
{
    float baseHeight = sampleSimplex((position + vec2(32.16, 64.32)) * 0.0135);
    return baseHeight;
/*
	float h1p = 0.42;
	float scale1 = 0.058;
	float h1 = pow(sampleSimplex(position, scale1), h1p);

	float semiStart = 0.31;
	float semiSize = 0.47;
	float h2p = 0.013;
	float h2 = smoothstep(semiStart, semiStart + semiSize, sampleSimplex(position, h2p));

	float semiStart2 = 0.65; 
	float semiSize2 = 0.1; 
	float h3p = (0.75 * 20) / 5000;
	float h3 = 1.0 - smoothstep(semiStart2, semiStart2 + semiSize2, sampleSimplex(position, h3p));
	return h1 * h2 * h3;*/
}



vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

float desaturate(vec3 color)
{
    float bw = (min(color.r, min(color.g, color.b)) + max(color.r, max(color.g, color.b))) * 0.5;
    return bw;
}

vec3 desaturate_blend(vec3 color, float alpha)
{
    return mix(color, vec3(desaturate(color)), alpha);
}



vec3 heightblend(vec3 input1, float height1, vec3 input2, float height2)
{
    float height_start = max(height1, height2) - 0.05;
    float level1 = max(height1 - height_start, 0);
    float level2 = max(height2 - height_start, 0);
    return ((input1 * level1) + (input2 * level2)) / (level1 + level2);
}

vec3 heightlerp(vec3 input1, float height1, vec3 input2, float height2, float t)
{
    t = clamp(t, 0, 1);
    return heightblend(input1, height1 * (1 - t), input2, height2 * t);
}


vec4 blur9(texture2D sourceTexture, sampler sourceSampler, vec2 uv, vec2 direction)
{
    vec2 resolution = textureSize(sampler2D(sourceTexture, sourceSampler), 0);
    vec4 color = vec4(0.0);
    vec2 off1 = vec2(1.3846153846) * direction;
    vec2 off2 = vec2(3.2307692308) * direction;
    color += texture(sampler2D(sourceTexture, sourceSampler), uv) * 0.2270270270;
    color += texture(sampler2D(sourceTexture, sourceSampler), uv + (off1 / resolution)) * 0.3162162162;
    color += texture(sampler2D(sourceTexture, sourceSampler), uv - (off1 / resolution)) * 0.3162162162;
    color += texture(sampler2D(sourceTexture, sourceSampler), uv + (off2 / resolution)) * 0.0702702703;
    color += texture(sampler2D(sourceTexture, sourceSampler), uv - (off2 / resolution)) * 0.0702702703;
    return color;
}




float beckmannDistribution(float x, float roughness)
{
    roughness = max(0.0001, roughness);
    float NdotH = max(x, 0.0001);
    float cos2Alpha = NdotH * NdotH;
    float tan2Alpha = (cos2Alpha - 1.0) / cos2Alpha;
    float roughness2 = roughness * roughness;
    float denom = 3.141592653589793 * roughness2 * cos2Alpha * cos2Alpha;
    return exp(tan2Alpha / roughness2) / denom;
}

vec3 cookTorranceSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float roughness, vec3 F0)
{

  //Half angle vector
  vec3 H = normalize(lightDirection + viewDirection);

  float VdotN = max(dot(viewDirection, surfaceNormal), 0.0001);
  float LdotN = max(dot(lightDirection, surfaceNormal), 0.0001);

  //Geometric term
  float NdotH = max(dot(surfaceNormal, H), 0.0001);
  float VdotH = max(dot(viewDirection, H), 0.0001);
  float LdotH = max(dot(lightDirection, H), 0.0001);
  float G1 = (2.0 * NdotH * VdotN) / VdotH;
  float G2 = (2.0 * NdotH * LdotN) / VdotH;
  float G = min(1.0, min(G1, G2));

  //Distribution term
  float D = beckmannDistribution(NdotH, roughness);

  //Fresnel term
  vec3 F = (F0 + (1.0 - F0)) * pow(1.0 - VdotH, 5.0);

  //Multiply terms and done
  return  (F * G * D / max(4.0  * VdotN, 0.0001));
}



float hex(in vec2 p)
{
    const vec2 s = vec2(1, 1.7320508);
    p = abs(p);
    
    return max(dot(p, s*.5), p.x); // Hexagon.
}

vec4 getHex(vec2 p)
{
    const vec2 s = vec2(1, 1.7320508);
    p *=  1.0 / sqrt(3.0);
    
    vec4 hC = floor(vec4(p, p - vec2(.5, 1))/s.xyxy) + .5;
    
    vec4 h = vec4(p - hC.xy*s, p - (hC.zw + .5)*s);
    
    return dot(h.xy, h.xy) < dot(h.zw, h.zw) 
        ? vec4(h.xy, hC.xy) 
        : vec4(h.zw, hC.zw + .5);
}