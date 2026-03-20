#pragma once

#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/CommonTypes.h"

#include <cstddef>

namespace o2
{
	// Vertex parameter flags - each bit represents a vertex attribute
	namespace VertexParam
	{
		const UInt None = 0;
		const UInt Position = 1 << 0; // float x, y, z (12 bytes)
		const UInt Color = 1 << 1; // Color32Bit (4 bytes)
		const UInt TexCoord0 = 1 << 2; // float tu, tv (8 bytes)
		const UInt TexCoord1 = 1 << 3; // float tu2, tv2 (8 bytes)
		const UInt TexCoord2 = 1 << 4; // float tu3, tv3 (8 bytes)
		const UInt Normal = 1 << 5; // float nx, ny, nz (12 bytes)

		inline size_t ParamSize(UInt flag)
		{
			switch (flag)
			{
				case Position:  return sizeof(float) * 3;
				case Color:     return sizeof(UInt);
				case TexCoord0: return sizeof(float) * 2;
				case TexCoord1: return sizeof(float) * 2;
				case TexCoord2: return sizeof(float) * 2;
				case Normal:    return sizeof(float) * 3;
				default:        return 0;
			}
		}
	}

	// Function type for getting parameter offset within a vertex struct
	using VertexParamOffsetFunc = size_t(*)(UInt param);

	// ---------------------------------------------------------------------------------
	// Vertex type descriptor built from parameter flags, stride, and an offset function
	// ---------------------------------------------------------------------------------
	struct VertexType
	{
		UInt                  flags = 0;
		size_t                stride = 0;
		VertexParamOffsetFunc paramOffsetFunc = nullptr;

	public:
		// Default constructor for an empty vertex type
		VertexType() = default;

		// Constructor to initialize the vertex type with specific flags, stride, and offset function
		VertexType(UInt flags, size_t stride, VertexParamOffsetFunc offsetFunc);

		// Returns the total size of the vertex in bytes, which is the stride
		size_t GetStride() const;

		// Returns byte offset to the specified parameter, delegating to the vertex struct's offset function
		size_t GetParamOffset(UInt param) const;

		// Counts how many texture coordinate sets are present based on the flags
		int GetTexCoordsCount() const;

		// Checks if a specific parameter is included in the vertex type by testing the corresponding bit in the flags
		bool HasParam(UInt param) const;

		// Equality operators to compare vertex types based on their flags, stride, and offset function
		bool operator==(const VertexType& other) const;

		// Inequality operator defined in terms of the equality operator
		bool operator!=(const VertexType& other) const;
	};

	// --------------------------------------------------------------------
	// Basic vertex structure with position, color, and texture coordinates
	// --------------------------------------------------------------------
	struct Vertex
	{
		float x, y, z;
		Color32Bit color;
		float tu, tv;
		float nx, ny, nz;

	public:
		Vertex();
		Vertex(float vx, float vy);
		Vertex(float vx, float vy, float vz);
		Vertex(float vx, float vy, float vz, Color32Bit vcolor, float vtu, float vtv);
		Vertex(float vx, float vy, Color32Bit vcolor, float vtu, float vtv);
		Vertex(const Vec2F& pos, Color32Bit vcolor, float vtu, float vtv);

		bool operator==(const Vertex& other) const;
		Vertex operator=(const Vec2F& vec);

		operator Vec2F() const;

		void Set(const Vec2F& pos, Color32Bit ccolor, float u, float v);
		void Set(float px, float py, Color32Bit ccolor, float u, float v);
		void Set(const Vec2F& pos, float cz, Color32Bit ccolor, float u, float v);

		void SetPosition(const Vec2F& pos);
		void SetPosition(float px, float py, float pz = 0);

		void SetUV(float u, float v);
		void SetUV(const Vec2F& uv);

		void SetNormal(float nnx, float nny, float nnz);
		void SetNormal(const Vec2F& tangent);

		static size_t ParamOffset(UInt param);
		static VertexType Type();
	};

	// --------------------------------------------------------------
	// Extended vertex structure with two sets of texture coordinates
	// --------------------------------------------------------------
	struct Vertex2Tex
	{
		float x, y, z;
		Color32Bit color;
		float tu, tv;
		float tu2, tv2;
		float nx, ny, nz;

	public:
		Vertex2Tex();

		static size_t ParamOffset(UInt param);
		static VertexType Type();
	};

	// --------------------------------------------------------------------------------------
	// Extended vertex structure with three sets of texture coordinates (for multi-texturing)
	// --------------------------------------------------------------------------------------
	struct Vertex3Tex
	{
		float x, y, z;
		Color32Bit color;
		float tu, tv;
		float tu2, tv2;
		float tu3, tv3;
		float nx, ny, nz;

	public:
		Vertex3Tex();

		static size_t ParamOffset(UInt param);
		static VertexType Type();
	};
}
