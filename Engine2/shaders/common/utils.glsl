const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 equirectangularUv(vec3 direction)
{
    vec2 uv = vec2(atan(direction.z, direction.x), asin(-direction.y));
    uv *= invAtan;
    uv += 0.5;

    uv.y = 1.0 - uv.y;

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

float voronoi2d(const in vec2 point) {
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

float sampleSimplex(vec2 position, float scale)
{
	return simplex(position*scale) * 0.5 + 0.5;
}

float sampleBaseHeight(vec2 position)
{
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
	return h1 * h2 * h3;
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

    return normalize(vec3(hR - hL, -eps2 * 0.2, hU - hD));
}

vec3 undiscovered(vec3 color, vec3 position, vec3 vis, float time)
{
    vec3 tint = vec3(250, 187, 107) / 255.0;
    float f = sampleFogHeight(position.xz, time);
    float y = -position.y + f - 0.5;
    //vec3 gg = mix(vec3(0.15), vec3(0.0), f);
    vec3 gg = mix(vec3(0.25), vec3(0.05), f);
    gg = mix(gg, gg*tint, 1.0) * 0.5;

    float h = 1.0 -smoothstep(0.4, 1.5, y);
    float h2 = 1.0 - smoothstep(-0.1, 0.0, y);
    vec3 grb = color * 0.95;
    vec3 gra = mix(grb, vec3(desaturate(grb)), 0.7);
    
    vec3 gr = mix(gra,gg, 0.99);
    gr = mix(gr, gg, h2);
    vec3 undis = mix(color, gr, h * 0.99);
    
    vec3 n = sampleFogNormal(position.xz, time);
	vec3 l = normalize(vec3(-1.0, -1.0, -1.0));
    vec3 ndotl = vec3(clamp(pow(dot(n, l), 1.2), 0.0, 1.0));


    //return undis;

    undis += ndotl * tint * 0.06;
    


    return mix(undis, color, vis.y);
}

vec3 outOfSight(vec3 color, vec3 position, vec3 vis, float time) 
{
    vec3 tint = vec3(250, 187, 107) / 255.0;
    float f = sampleFogHeight(position.xz, time);
    float y = -position.y + f;
    //vec3 gg = mix(vec3(0.15), vec3(0.0), f);
    vec3 gg = mix(vec3(0.25), vec3(0.05), f);
    gg = mix(gg, gg*tint, 1.0) * 0.5;

    float h = 1.0 -smoothstep(0.4, 1.5, y);
    float h2 = 1.0 - smoothstep(-0.1, 0.1, y);
    vec3 grb = color * 0.95;
    vec3 gra = mix(grb, vec3(desaturate(grb)), 0.7);
    
    vec3 gr = mix(gra,gg, 0.858);
    gr = mix(gr, gg, h2*0.9);
    vec3 undis = mix(color, gr, h * 0.99);
    
    vec3 n = sampleFogNormal(position.xz, time);
	vec3 l = normalize(vec3(-1.0, -1.0, -1.0));
    vec3 ndotl = vec3(clamp(pow(dot(n, l), 2.2), 0.0, 1.0));
    //return ndotl;

    undis += ndotl * tint * 0.06;

    return mix(undis, color, vis.y*0.8);
}

vec3 fogOfWar(vec3 color, vec3 position, vec3 vis, float time)
{
    //vis.xy = pow(vis.xy, vec2(1.0));
    //vis.x = smoothstep(0.2, 0.8, vis.x);
    //vis.y = smoothstep(0.2, 0.8, vis.y);
    //return vec3(vis.x, vis.y,0.0);

    //vis.x = smoothstep(0.0, 0.5, vis.x);
    //vis.y = smoothstep(0.0, 0.5, vis.y);
	

    vec3 oos = outOfSight(color, position, vis, time);
    vec3 und = undiscovered(color, position, vis, time);
    vec3 fow = mix(und, oos, vis.x);
    //vec3 fow = heightlerp(und, 0.4, oos, sampleFogHeight(position.xz, time),vis.x);
    //fow = mix(fow, oos, 1.0);
    return fow;
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