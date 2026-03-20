#include "o2/stdafx.h"
#include "Vertex.h"

namespace o2
{
	Vertex::Vertex():
		x(0), y(0), color(0), tu(0), tv(0), z(1), nx(1), ny(0), nz(0)
	{}

	Vertex::Vertex(float vx, float vy):
		x(vx), y(vy), z(1), color(0), tu(0), tv(0), nx(1), ny(0), nz(0)
	{}

	Vertex::Vertex(float vx, float vy, float vz):
		x(vx), y(vy), z(vz), color(0), tu(0), tv(0), nx(1), ny(0), nz(0)
	{}

	Vertex::Vertex(float vx, float vy, float vz, Color32Bit vcolor, float vtu, float vtv):
		x(vx), y(vy), z(vz), color(vcolor), tu(vtu), tv(vtv), nx(1), ny(0), nz(0)
	{}

	Vertex::Vertex(float vx, float vy, Color32Bit vcolor, float vtu, float vtv):
		x(vx), y(vy), z(1), color(vcolor), tu(vtu), tv(vtv), nx(1), ny(0), nz(0)
	{}

	Vertex::Vertex(const Vec2F& pos, Color32Bit vcolor, float vtu, float vtv):
		x(pos.x), y(pos.y), z(1), color(vcolor), tu(vtu), tv(vtv), nx(1), ny(0), nz(0)
	{}

	void Vertex::Set(const Vec2F& pos, Color32Bit ccolor, float u, float v)
	{
		x = pos.x; y = pos.y;
		color = ccolor;
		tu = u; tv = v;
	}

	void Vertex::Set(float px, float py, Color32Bit ccolor, float u, float v)
	{
		x = px; y = py;
		color = ccolor;
		tu = u; tv = v;
	}

	void Vertex::Set(const Vec2F& pos, float cz, Color32Bit ccolor, float u, float v)
	{
		x = pos.x; y = pos.y;
		z = cz;
		color = ccolor;
		tu = u; tv = v;
	}

	void Vertex::SetPosition(const Vec2F& pos)
	{
		x = pos.x;
		y = pos.y;
	}

	void Vertex::SetPosition(float px, float py, float pz /*= 0*/)
	{
		x = px; y = py; z = pz;
	}

	void Vertex::SetUV(float u, float v)
	{
		tu = u; tv = v;
	}

	void Vertex::SetUV(const Vec2F& uv)
	{
		tu = uv.x; tv = uv.y;
	}

	bool Vertex::operator==(const Vertex& other) const
	{
		return Math::Equals(x, other.x) && Math::Equals(y, other.y) && color == other.color &&
			Math::Equals(tu, other.tu) && Math::Equals(tv, other.tv);
	}

	Vertex Vertex::operator=(const Vec2F& vec)
	{
		x = vec.x; y = vec.y;
		return *this;
	}

	Vertex::operator Vec2F() const
	{
		return Vec2F(x, y);
	}

	void Vertex::SetNormal(float nnx, float nny, float nnz)
	{
		nx = nnx; ny = nny; nz = nnz;
	}

	void Vertex::SetNormal(const Vec2F& tangent)
	{
		nx = tangent.x; ny = tangent.y; nz = 0;
	}

	size_t Vertex::ParamOffset(UInt param)
	{
		switch (param)
		{
			case VertexParam::Position:  return offsetof(Vertex, x);
			case VertexParam::Color:     return offsetof(Vertex, color);
			case VertexParam::TexCoord0: return offsetof(Vertex, tu);
			case VertexParam::Normal:    return offsetof(Vertex, nx);
			default: return 0;
		}
	}

	VertexType Vertex::Type()
	{
		return VertexType(VertexParam::Position | VertexParam::Color | VertexParam::TexCoord0 | VertexParam::Normal,
						  sizeof(Vertex), &Vertex::ParamOffset);
	}

	bool VertexType::operator!=(const VertexType& other) const
	{
		return !(*this == other);
	}

	bool VertexType::operator==(const VertexType& other) const
	{
		return flags == other.flags && stride == other.stride && paramOffsetFunc == other.paramOffsetFunc;
	}

	VertexType::VertexType(UInt flags, size_t stride, VertexParamOffsetFunc offsetFunc):
		flags(flags), stride(stride), paramOffsetFunc(offsetFunc)
	{}

	size_t VertexType::GetStride() const
	{
		return stride;
	}

	size_t VertexType::GetParamOffset(UInt param) const
	{
		return paramOffsetFunc ? paramOffsetFunc(param) : 0;
	}

	int VertexType::GetTexCoordsCount() const
	{
		int count = 0;
		if (flags & VertexParam::TexCoord0)
			count++;

		if (flags & VertexParam::TexCoord1)
			count++;

		if (flags & VertexParam::TexCoord2)
			count++;

		return count;
	}

	bool VertexType::HasParam(UInt param) const
	{
		return (flags & param) != 0;
	}

	size_t Vertex2Tex::ParamOffset(UInt param)
	{
		switch (param)
		{
			case VertexParam::Position:  return offsetof(Vertex2Tex, x);
			case VertexParam::Color:     return offsetof(Vertex2Tex, color);
			case VertexParam::TexCoord0: return offsetof(Vertex2Tex, tu);
			case VertexParam::TexCoord1: return offsetof(Vertex2Tex, tu2);
			case VertexParam::Normal:    return offsetof(Vertex2Tex, nx);
			default: return 0;
		}
	}

	VertexType Vertex2Tex::Type()
	{
		return VertexType(
			VertexParam::Position | VertexParam::Color | VertexParam::TexCoord0 | VertexParam::TexCoord1 | VertexParam::Normal,
			sizeof(Vertex2Tex), &Vertex2Tex::ParamOffset);
	}

	Vertex2Tex::Vertex2Tex():
		x(0), y(0), z(1), color(0), tu(0), tv(0), tu2(0), tv2(0), nx(1), ny(0), nz(0)
	{}

	size_t Vertex3Tex::ParamOffset(UInt param)
	{
		switch (param)
		{
			case VertexParam::Position:  return offsetof(Vertex3Tex, x);
			case VertexParam::Color:     return offsetof(Vertex3Tex, color);
			case VertexParam::TexCoord0: return offsetof(Vertex3Tex, tu);
			case VertexParam::TexCoord1: return offsetof(Vertex3Tex, tu2);
			case VertexParam::TexCoord2: return offsetof(Vertex3Tex, tu3);
			case VertexParam::Normal:    return offsetof(Vertex3Tex, nx);
			default: return 0;
		}
	}

	VertexType Vertex3Tex::Type()
	{
		return VertexType(VertexParam::Position | VertexParam::Color | VertexParam::TexCoord0 |
						  VertexParam::TexCoord1 | VertexParam::TexCoord2 | VertexParam::Normal,
						  sizeof(Vertex3Tex), &Vertex3Tex::ParamOffset);
	}

	Vertex3Tex::Vertex3Tex():
		x(0), y(0), z(1), color(0), tu(0), tv(0), tu2(0), tv2(0), tu3(0), tv3(0), nx(1), ny(0), nz(0)
	{}
}
