#include "o2/stdafx.h"
#include "Geometry.h"

#include "o2/Utils/Math/Intersection.h"

namespace o2
{
	void Geometry::CreatePolyLineMesh(const Vertex* points, int pointsCount,
									  Vertex*& verticies, UInt& vertexCount, UInt& vertexSize,
									  UInt16*& indexes, UInt& polyCount, UInt& polySize,
									  float width, float texBorderTop, float texBorderBottom, const Vec2F& texSize,
									  const Vec2F& invCameraScale /*= Vec2F(1, 1)*/)
	{
		int newVertexCount = pointsCount*4;
		int newPolyCount = (pointsCount - 1)*6;

		if (newVertexCount > (int)vertexSize)
		{
			if (verticies)
				delete[] verticies;

			verticies = new Vertex[newVertexCount];
			vertexSize = newVertexCount;
		}

		if (newPolyCount > (int)polySize)
		{
			if (indexes)
				delete[] indexes;

			indexes = new UInt16[newPolyCount*3];
			polySize = newPolyCount;
		}

		float halfWidth = width*0.5f;
		float halfWidhtBorderTop = halfWidth + texBorderTop;
		float halfWidhtBorderBottom = halfWidth + texBorderBottom;

		Vec2F invTexSize(1.0f/texSize.x, 1.0f/texSize.y);

		float upTexV = 1.0f - texBorderTop*invTexSize.y;
		float downTexV = texBorderBottom*invTexSize.y;

		Vec2F offs = Vec2F(0.5f, 0.5f)*invCameraScale;

		int vertex = 0; int poly = 0;
		float u = 0;

		for (int i = 0; i < pointsCount; i++)
		{
			Vec2F point = (Vec2F)points[i] + offs;
			ULong color = points[i].color;
			ULong zeroAlphaColor = color << 8 >> 8;

			if (i == 0)
			{
				Vec2F dir = ((Vec2F)points[i + 1] + offs - point).Normalized();
				Vec2F norm = dir.Perpendicular();

				Vec2F upBorder = point + norm*invCameraScale*halfWidhtBorderBottom;
				Vec2F up = point + norm*invCameraScale*halfWidth;
				Vec2F down = point - norm*invCameraScale*halfWidth;
				Vec2F downBorder = point - norm*invCameraScale*halfWidhtBorderTop;

				verticies[vertex++].Set(up, color, 0, upTexV);
				verticies[vertex++].Set(upBorder, zeroAlphaColor, 0, 1);
				verticies[vertex++].Set(down, color, 0, downTexV);
				verticies[vertex++].Set(downBorder, zeroAlphaColor, 0, 0);
			}
			else if (i == pointsCount - 1)
			{
				Vec2F dir = point - points[i - 1] - offs;
				float segLength = dir.Length();
				dir /= segLength;
				Vec2F norm = dir.Perpendicular();

				Vec2F upBorder = point + norm*invCameraScale*halfWidhtBorderBottom;
				Vec2F up = point + norm*invCameraScale*halfWidth;
				Vec2F down = point - norm*invCameraScale*halfWidth;
				Vec2F downBorder = point - norm*invCameraScale*halfWidhtBorderTop;

				if (i%2 == 0)
					u -= segLength/invCameraScale.x*invTexSize.x;
				else
					u += segLength/invCameraScale.x*invTexSize.x;

				verticies[vertex++].Set(up, color, u, upTexV);
				verticies[vertex++].Set(upBorder, zeroAlphaColor, u, 1);
				verticies[vertex++].Set(down, color, u, downTexV);
				verticies[vertex++].Set(downBorder, zeroAlphaColor, u, 0);
			}
			else
			{
				Vec2F prev = (Vec2F)points[i - 1] + offs;
				Vec2F next = (Vec2F)points[i + 1] + offs;

				Vec2F prevDir = point - prev;
				float segLength = prevDir.Length();
				prevDir /= segLength;

				Vec2F nextDir = (point - next).Normalized();

				Vec2F prevNorm = prevDir.Perpendicular().Inverted();
				Vec2F nextNorm = nextDir.Perpendicular();

				Vec2F upBorder = Intersection::LinesNoChek(point - prevNorm*halfWidhtBorderTop, prevDir,
														   point - nextNorm*halfWidhtBorderTop, nextDir);


				Vec2F up = Intersection::LinesNoChek(point - prevNorm*halfWidth, prevDir,
													 point - nextNorm*halfWidth, nextDir);

				Vec2F down = Intersection::LinesNoChek(point + prevNorm*halfWidth, prevDir,
													   point + nextNorm*halfWidth, nextDir);

				Vec2F downBorder = Intersection::LinesNoChek(point + prevNorm*halfWidhtBorderBottom, prevDir,
															 point + nextNorm*halfWidhtBorderBottom, nextDir);

				up = (up - point)*invCameraScale + point;
				down = (down - point)*invCameraScale + point;
				upBorder = (upBorder - point)*invCameraScale + point;
				downBorder = (downBorder - point)*invCameraScale + point;

				float segSign = i%2 == 0 ? -1.0f : 1.0f;

				u += segLength/invCameraScale.x*invTexSize.x*segSign;

				verticies[vertex++].Set(up, color, u + prevDir.Dot(up - point)*invTexSize.x*segSign, upTexV);
				verticies[vertex++].Set(upBorder, zeroAlphaColor, u + prevDir.Dot(upBorder - point)*invTexSize.x*segSign, 1);
				verticies[vertex++].Set(down, color, u + prevDir.Dot(down - point)*invTexSize.x*segSign, downTexV);
				verticies[vertex++].Set(downBorder, zeroAlphaColor, u + prevDir.Dot(downBorder - point)*invTexSize.x*segSign, 0.0f);
			}

#define POLYGON(A, B, C) \
    indexes[poly] = vertex - A - 1; indexes[poly + 1] = vertex - B - 1; indexes[poly + 2] = vertex - C - 1; poly += 3;

			if (i > 0)
			{
				POLYGON(6, 2, 7);
				POLYGON(7, 2, 3);
				POLYGON(5, 7, 3);
				POLYGON(5, 3, 1);
				POLYGON(4, 5, 1);
				POLYGON(4, 1, 0);
			}
		}

		vertexCount = vertex;
		polyCount = poly/3;
	}
}
