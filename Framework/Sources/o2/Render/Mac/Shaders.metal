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
	matrix_float4x4 m = uniforms.projectionMatrix*uniforms.modelViewMatrix;
	
	out.position = m*float4(v.x, v.y, v.z, 1.0);
	
	out.color = v.color;
	out.textureCoordinate = vector_float2(v.tu, v.tv);
	
	return out;
}

fragment float4 fragmentShader(RasterizerData in [[stage_in]],
							   texture2d<half> colorTexture [[ texture(0) ]])
{
	constexpr sampler textureSampler (mag_filter::linear,
									  min_filter::linear);
	
	const half4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate);
	return float4(colorSample)*in.color;
}

