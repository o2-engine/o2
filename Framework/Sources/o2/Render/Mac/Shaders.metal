#include <metal_stdlib>

using namespace metal;

#include "ShaderTypes.h"

struct RasterizerData
{
	float4 position [[position]];
	float4 color;
	float2 textureCoordinate;
};

vertex RasterizerData vertexShader(uint vertexID [[vertex_id]], constant MetalVertex2 *vertices [[buffer(0)]], constant Uniforms &uniforms [[buffer(1)]])
{
	RasterizerData out;
	
	MetalVertex2 v = vertices[vertexID];
	
	out.position = uniforms.mvpMatrix*float4(v.x, v.y, v.z, 1);
    out.position.z = 0.5;
	out.color = v.color;
	out.textureCoordinate = vector_float2(v.tu, v.tv);
	
	return out;
}

fragment float4 fragmentShader(RasterizerData in [[stage_in]],
							   texture2d<half> colorTexture [[ texture(0) ]],
							   sampler textureSampler [[ sampler(0) ]])
{
	const half4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate);
    return float4(colorSample)*in.color;
}

