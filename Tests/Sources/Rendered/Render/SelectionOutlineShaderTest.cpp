#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Material.h"
#include "o2/Render/Mesh.h"
#include "o2/Render/Render.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Math/Math.h"

using namespace o2;

// The editor selection outline shader: on a synthetic mask the border pixels get the outline
// color while the silhouette inside and the empty background stay transparent
TEST(SelectionOutlineShader, BorderDetectedOnSyntheticMask)
{
    // 64x64 mask with an opaque square in texels [20, 44)
    Bitmap maskBitmap(PixelFormat::R8G8B8A8, Vec2I(64, 64));
    maskBitmap.Fill(Color4(0, 0, 0, 0));
    maskBitmap.FillRect(20, 44, 44, 20, Color4(255, 255, 255, 255));

    TextureRef maskTexture(maskBitmap);
    maskTexture->SetFilter(Texture::Filter::Nearest);

    auto material = Material::CreateFromBuiltinShaders("SelectionOutline");
    ASSERT_NE(material, nullptr);
    material->AddParam(mmake<ShaderParamVec2>("u_texelSize", Vec2F(1.0f/64.0f, 1.0f/64.0f)));
    ASSERT_TRUE(material->Build());

    Ref<Bitmap> captured;
    o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

    o2Render.Begin();
    o2Render.Clear(Color4::Black());
    o2Render.SetCamera(Camera());

    // Centered quad, 8x upscale of the mask (512 px), green outline color
    const float quadHalfSize = 256.0f;
    ULong green = Color4::Green().ABGR();

    Mesh quad(maskTexture, 4, 2);
    quad.SetMaterial(material);

    Vertex* vertices = quad.GetVertices<Vertex>();
    vertices[0] = Vertex(-quadHalfSize, quadHalfSize, 0.0f, green, 0.0f, 0.0f);
    vertices[1] = Vertex(quadHalfSize, quadHalfSize, 0.0f, green, 1.0f, 0.0f);
    vertices[2] = Vertex(quadHalfSize, -quadHalfSize, 0.0f, green, 1.0f, 1.0f);
    vertices[3] = Vertex(-quadHalfSize, -quadHalfSize, 0.0f, green, 0.0f, 1.0f);

    VertexIndex* indexes = quad.GetIndexes();
    indexes[0] = 0; indexes[1] = 1; indexes[2] = 2;
    indexes[3] = 0; indexes[4] = 2; indexes[5] = 3;

    quad.vertexCount = 4;
    quad.polyCount = 2;
    quad.Draw();

    o2Render.SetCamera(Camera());
    o2Render.End();

    ASSERT_TRUE(captured);

    Vec2I size = captured->GetSize();
    const UInt8* data = captured->GetData();

    auto greenAt = [&](int x, int y) { return (int)data[(y*size.x + x)*4 + 1]; };

    // Square in quad space: texels [20, 44) of 64 upscaled to 512 -> [-96, 96) from the quad center
    int centerX = size.x/2, centerY = size.y/2;
    int squareEdgeOffset = (int)(96.0f/256.0f*quadHalfSize);

    // Center of the square: silhouette inside, no outline
    EXPECT_LT(greenAt(centerX, centerY), 60);

    // Just outside the square edges (along the center row, orientation-agnostic): outline color
    EXPECT_GT(greenAt(centerX + squareEdgeOffset + 5, centerY), 200);
    EXPECT_GT(greenAt(centerX - squareEdgeOffset - 5, centerY), 200);

    // Far outside: nothing
    EXPECT_LT(greenAt(centerX + squareEdgeOffset + 60, centerY), 60);
    EXPECT_LT(greenAt(centerX - squareEdgeOffset - 60, centerY), 60);
}
