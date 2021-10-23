#pragma once
#include <simd/simd.h>

struct Uniforms
{
	matrix_float4x4 projectionMatrix;
	matrix_float4x4 modelViewMatrix;
};

struct MetalVertex2
{
	float x, y, z;
	vector_float4 color;
	float tu, tv;
};
