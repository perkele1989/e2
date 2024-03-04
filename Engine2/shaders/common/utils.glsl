const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 equirectangularUv(vec3 direction)
{
    vec2 uv = vec2(atan(direction.z, direction.x), asin(-direction.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}