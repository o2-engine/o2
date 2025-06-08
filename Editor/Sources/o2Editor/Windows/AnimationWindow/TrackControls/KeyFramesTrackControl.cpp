#include "o2Editor/stdafx.h"
#include "KeyFramesTrackControl.h"

DECLARE_TEMPLATE_CLASS_MANUAL(Editor::KeyFramesTrackControl<AnimationTrack<float>>);
DECLARE_TEMPLATE_CLASS_MANUAL(Editor::KeyFramesTrackControl<AnimationTrack<bool>>);
DECLARE_TEMPLATE_CLASS_MANUAL(Editor::KeyFramesTrackControl<AnimationTrack<Color4>>);
DECLARE_TEMPLATE_CLASS_MANUAL(Editor::KeyFramesTrackControl<AnimationTrack<Vec2F>>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::KeyFramesTrackControl<AnimationTrack<float>>>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::KeyFramesTrackControl<AnimationTrack<bool>>>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::KeyFramesTrackControl<AnimationTrack<Color4>>>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::KeyFramesTrackControl<AnimationTrack<Vec2F>>>);

namespace Editor
{
    void DrawCurveInCoords(const ApproximationValue* points, int pointsCount, const RectF& pointsBounds, const Basis& drawBasis,
                           const Color4& color)
    {
        const int bufferSize = 50;
        static Vertex buffer[bufferSize];

        if (pointsBounds.Height() < 0.001f)
        {
            o2Render.DrawAALine(Vec2F(0.0f, 0.5f)*drawBasis, Vec2F(1.0f, 0.5f)*drawBasis, color);
            return;
        }

        Basis boundsBasis(pointsBounds);
        Basis transform = boundsBasis.Inverted() * drawBasis;

        for (int j = 0; j < pointsCount; j++)
            buffer[j].Set(Vec2F(points[j].position, points[j].value)*transform, color.ABGR(), 0, 0);

        o2Render.DrawAAPolyLine(buffer, pointsCount, 1.0f, LineType::Solid, false);
    }

    void DrawCenterCurveInCoords(const Vec2F* points, int pointsCount, const RectF& pointsBounds, const Basis& drawBasis,
                                 const Color4& color)
    {
        const int bufferSize = 50;
        static Vertex buffer[bufferSize];

        Basis boundsBasis(pointsBounds);
        Basis boundsBasisInv = boundsBasis.Inverted();
        Basis newDrawBasis(drawBasis.origin + drawBasis.yv*0.5f, drawBasis.xv, drawBasis.yv);

        for (int j = 0; j < pointsCount; j++)
        {
            Vec2F p = points[j] * boundsBasisInv;
            p.y -= p.x;
            buffer[j].Set(p * newDrawBasis, color.ABGR(), 0, 0);
        }

        o2Render.DrawAAPolyLine(buffer, pointsCount, 1.0f, LineType::Solid, false);
    }

    template<>
    void KeyFramesTrackControl<AnimationTrack<float>>::SetCurveViewEnabled(bool enabled)
    {
        mAddKeyDotButton->enabled = enabled;
        mAddKeyButton->enabled = !enabled;
    }

    template<>
    void KeyFramesTrackControl<AnimationTrack<float>>::Draw()
    {
        if (!mResEnabledInHierarchy)
            return;

        if (!mHandlesSheet.Lock()->enabled)
            return;

        OnDrawn();

        auto timeline = mTimeline.Lock();
        auto track = mTrack.Lock();

        o2Render.EnableScissorTest(timeline->layout->GetWorldRect());

        for (int i = 1; i < track->GetKeys().Count(); i++)
        {
            auto& key = track->GetKeys()[i];
            auto& prevKey = track->GetKeys()[i - 1];

            Basis drawCoords(RectF(timeline->LocalToWorld(prevKey.position) - 1,
                                   layout->GetWorldTop() - 5,
                                   timeline->LocalToWorld(key.position) - 1,
                                   layout->GetWorldBottom() + 5));

            DrawCurveInCoords(key.GetTopApproximatedPoints(), key.GetApproximatedPointsCount(),
                              key.GetGetApproximatedPointsBounds(), drawCoords, Color4(44, 62, 80));
        }

        for (auto& handle : mHandles)
            handle->handle->Draw();

        o2Render.DisableScissorTest();

        DrawDebugFrame();
    }

    template<>
    void KeyFramesTrackControl<AnimationTrack<Color4>>::Draw()
    {
        if (!mResEnabledInHierarchy)
            return;

        if (!mHandlesSheet.Lock()->enabled)
            return;

        OnDrawn();

        auto timeline = mTimeline.Lock();
        auto track = mTrack.Lock();

        if (!track->GetKeys().IsEmpty())
        {
            static TextureRef chessBackTexture;
            static Sprite chessBackSprite;
            if (!chessBackTexture)
            {
                Color4 color1(1.0f, 1.0f, 1.0f, 1.0f), color2(0.7f, 0.7f, 0.7f, 1.0f);
                Bitmap backLayerBitmap(PixelFormat::R8G8B8A8, Vec2I(16, 16));
                backLayerBitmap.Fill(color1);
                backLayerBitmap.FillRect(0, 8, 8, 0, color2);
                backLayerBitmap.FillRect(8, 16, 16, 8, color2);
                chessBackTexture = TextureRef(backLayerBitmap);
                chessBackSprite = Sprite(chessBackTexture, RectI(0, 0, 16, 16));
                chessBackSprite.mode = SpriteMode::Tiled;
            }

            chessBackSprite.SetRect(RectF(timeline->LocalToWorld(track->GetKeys()[0].position), layout->GetWorldTop() - 5,
                                          timeline->LocalToWorld(track->GetKeys().Last().position), layout->GetWorldBottom() + 4));
            chessBackSprite.Draw();

            static Mesh mesh;
            int verticies = track->GetKeys().Count()*2;
            int polygons = (track->GetKeys().Count() - 1)*2;

            if (mesh.GetMaxVertexCount() < (UInt)verticies)
                mesh.Resize(verticies, polygons);

            mesh.vertexCount = verticies;
            mesh.polyCount = polygons;

            o2Render.EnableScissorTest(timeline->layout->GetWorldRect());

            for (int i = 0; i < track->GetKeys().Count(); i++)
            {
                auto& key = track->GetKeys()[i];
                float keyPos = timeline->LocalToWorld(key.position);
                int nv = i*2;

                mesh.vertices[nv] = Vertex(keyPos, layout->GetWorldTop() - 5, key.value.ABGR(), 0, 0);
                mesh.vertices[nv + 1] = Vertex(keyPos, layout->GetWorldBottom() + 4, key.value.ABGR(), 0, 0);

                if (i > 0)
                {
                    int np = (i - 1)*2;

                    mesh.indexes[np*3] = nv - 2;
                    mesh.indexes[np*3 + 1] = nv;
                    mesh.indexes[np*3 + 2] = nv - 1;
                    mesh.indexes[np*3 + 3] = nv;
                    mesh.indexes[np*3 + 4] = nv + 1;
                    mesh.indexes[np*3 + 5] = nv - 1;
                }
            }

            mesh.Draw();
        }

        for (auto& handle : mHandles)
            handle->handle->Draw();

        o2Render.DisableScissorTest();

        DrawDebugFrame();
    }

    template<>
    void KeyFramesTrackControl<AnimationTrack<bool>>::Draw()
    {
        if (!mResEnabledInHierarchy)
            return;

        if (!mHandlesSheet.Lock()->enabled)
            return;

        OnDrawn();

        auto timeline = mTimeline.Lock();
        auto track = mTrack.Lock();

        if (!track->GetKeys().IsEmpty())
        {

            float lineOffset = 11;

            static Mesh mesh;
            int verticies = (track->GetKeys().Count() - 1)*4;
            int polygons = (track->GetKeys().Count() - 1)*2;

            if (mesh.GetMaxVertexCount() < (UInt)verticies)
                mesh.Resize(verticies, polygons);

            mesh.vertexCount = verticies;
            mesh.polyCount = polygons;

            o2Render.EnableScissorTest(timeline->layout->GetWorldRect());

            Color4 trueColor(44, 62, 80);
            Color4 falseColor(0, 0, 0, 0);

            for (int i = 1; i < track->GetKeys().Count(); i++)
            {
                auto& key = track->GetKeys()[i];
                auto& prevKey = track->GetKeys()[i - 1];

                float keyPos = timeline->LocalToWorld(key.position);
                float prevKeyPos = timeline->LocalToWorld(prevKey.position);

                int nv = (i - 1)*4;
                int np = (i - 1)*2;

                auto color = (prevKey.value ? trueColor : falseColor).ABGR();

                mesh.vertices[nv] = Vertex(prevKeyPos, layout->GetWorldTop() - lineOffset, color, 0, 0);
                mesh.vertices[nv + 1] = Vertex(keyPos, layout->GetWorldTop() - lineOffset, color, 0, 0);
                mesh.vertices[nv + 2] = Vertex(keyPos, layout->GetWorldBottom() + lineOffset, color, 0, 0);
                mesh.vertices[nv + 3] = Vertex(prevKeyPos, layout->GetWorldBottom() + lineOffset, color, 0, 0);

                mesh.indexes[np*3] = nv;
                mesh.indexes[np*3 + 1] = nv + 1;
                mesh.indexes[np*3 + 2] = nv + 2;
                mesh.indexes[np*3 + 3] = nv;
                mesh.indexes[np*3 + 4] = nv + 2;
                mesh.indexes[np*3 + 5] = nv + 3;
            }

            mesh.Draw();
        }

        for (auto& handle : mHandles)
            handle->handle->Draw();

        o2Render.DisableScissorTest();

        DrawDebugFrame();
    }
}
