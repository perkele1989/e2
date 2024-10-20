#include <shaders/header.glsl>
// Fragment attributes

in vec4 fragmentPosition;

// Out color
out vec4 outColor;
out vec4 outPosition;

#include <shaders/sky/sky.common.glsl>

void main()
{
    vec3 viewToSurface = normalize(fragmentPosition.xyz - (inverse(renderer.viewMatrix) * vec4(0.0, 0.0, 0.0, 1.0)).xyz);
    outColor.rgb = textureLod(sampler2D(irradianceCube, equirectSampler), equirectangularUv(viewToSurface), 0.0).rgb * renderer.ibl1.x * 0.5 *  vec3(0.1, 0.4, 1.0);


    outColor.a = 1.0;

}