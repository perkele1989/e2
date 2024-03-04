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
