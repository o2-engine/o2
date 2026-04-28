#include "o2/stdafx.h"
#include "SplineMeshCollider.h"

#include "o2/Render/Render.h"
#include "o2/Scene/Actor.h"

namespace o2
{
    SplineMeshCollider::SplineMeshCollider() = default;

    SplineMeshCollider::SplineMeshCollider(const SplineMeshCollider& other):
        SplineCollider(other),
        mImage(other.mImage), mWidth(other.mWidth), mOffset(other.mOffset), mColor(other.mColor)
    {
        mNeedUpdateMesh = true;
    }

    SplineMeshCollider::~SplineMeshCollider() = default;

    SplineMeshCollider& SplineMeshCollider::operator=(const SplineMeshCollider& other)
    {
        SplineCollider::operator=(other);
        mImage = other.mImage;
        mWidth = other.mWidth;
        mOffset = other.mOffset;
        mColor = other.mColor;
        mNeedUpdateMesh = true;
        return *this;
    }

    void SplineMeshCollider::SetImage(const AssetRef<ImageAsset>& image)
    {
        mImage = image;
        mNeedUpdateMesh = true;
    }

    const AssetRef<ImageAsset>& SplineMeshCollider::GetImage() const
    {
        return mImage;
    }

    void SplineMeshCollider::SetWidth(float w)
    {
        mWidth = w;
        mNeedUpdateMesh = true;
    }

    float SplineMeshCollider::GetWidth() const
    {
        return mWidth;
    }

    void SplineMeshCollider::SetOffset(float o)
    {
        mOffset = o;
        mNeedUpdateMesh = true;
    }

    float SplineMeshCollider::GetOffset() const
    {
        return mOffset;
    }

    void SplineMeshCollider::SetColor(const Color4& c)
    {
        mColor = c;
        mNeedUpdateMesh = true;
    }

    const Color4& SplineMeshCollider::GetColor() const
    {
        return mColor;
    }

    String SplineMeshCollider::GetName()
    {
        return "Spline mesh collider";
    }

    String SplineMeshCollider::GetCategory()
    {
        return "Physics";
    }

    bool SplineMeshCollider::IsAvailableFromCreateMenu()
    {
        return true;
    }

    void SplineMeshCollider::OnSplineChanged()
    {
        SplineCollider::OnSplineChanged();
        mNeedUpdateMesh = true;
    }

    void SplineMeshCollider::OnTransformUpdated()
    {
        SplineCollider::OnTransformUpdated();
        mNeedUpdateMesh = true;
    }

    void SplineMeshCollider::OnAddToScene()
    {
        SplineCollider::OnAddToScene();
        mNeedUpdateMesh = true;
    }

    void SplineMeshCollider::OnDraw()
    {
        if (mNeedUpdateMesh)
            UpdateMesh();

        if (mMesh.polyCount > 0)
            mMesh.Draw();
    }

    void SplineMeshCollider::UpdateMesh()
    {
        mNeedUpdateMesh = false;

        mMesh.vertexCount = 0;
        mMesh.polyCount = 0;

        if (!spline)
            return;

        const auto& keys = spline->GetKeys();
        if (keys.Count() < 2)
            return;

        // 1) Sample spline into a polyline (local space). Each Spline::Key K stores
        //    in mLeftApproxValues the precomputed bezier approximation for the
        //    segment K-1 -> K (20 points, index 0 == start point, last == end point).
        //    For closed splines keys[0] additionally holds the closing segment from
        //    keys[N-1] -> keys[0].
        Vector<Vec2F> path;
        auto appendKeySegment = [&](int keyIndex, bool skipFirst) {
            const ApproximationVec2F* approx = keys[keyIndex].GetApproximatedPointsLeft();
            int count = keys[keyIndex].GetApproximatedPointsCount();
            for (int j = skipFirst ? 1 : 0; j < count; j++)
                path.Add(approx[j].value);
        };

        for (int i = 1; i < keys.Count(); i++)
            appendKeySegment(i, /*skipFirst*/ i > 1);

        if (mIsLoop)
            appendKeySegment(0, /*skipFirst*/ true);

        if (path.Count() < 2)
            return;

        // 2) Determine sprite size and atlas UV rect. When the image lives inside an
        //    atlas, the source rect points into a sub-region of the texture — UVs
        //    must be mapped accordingly and cannot wrap freely past [u0..u1].
        TextureSource imageSource = mImage ? mImage->GetTextureSource() : TextureSource();
        auto texture = imageSource.texture;

        Vec2F texSize(1.0f, 1.0f);
        if (texture)
            texSize = texture->GetSize();

        RectF imageRect = imageSource.sourceRect;
        Vec2F spriteSize(Math::Max(imageRect.Width(), 1.0f), Math::Max(imageRect.Height(), 1.0f));

        RectF imageUV;
        if (texture)
        {
            imageUV = RectF(imageRect.left / texSize.x, 1.0f - imageRect.top / texSize.y,
                            imageRect.right / texSize.x, 1.0f - imageRect.bottom / texSize.y);
        }
        else
        {
            imageUV = RectF(0.0f, 0.0f, 1.0f, 1.0f);
        }

        // World length per one tile, preserving sprite aspect ratio relative to width.
        float tileLength = mWidth * (spriteSize.x / spriteSize.y);
        if (tileLength < 0.001f)
            tileLength = 1.0f;

        // 3) Build "stations" along the path. Each station carries a position +
        //    perpendicular normal + atlas U coordinate. Whenever the cumulative
        //    distance crosses an integer tile boundary we insert TWO coincident
        //    stations: one closing the previous tile (uAtlas = imageUV.right) and
        //    one starting the new tile (uAtlas = imageUV.left, isQuadStart=true).
        //    The flag tells the index pass to skip the zero-area quad spanning the
        //    seam, which is what gives the atlas-safe split.
        struct Station
        {
            Vec2F pos;
            Vec2F normal;
            float uAtlas;
            bool  isQuadStart; // skip quad to previous station — this is a tile seam start
        };

        auto computeNormal = [&](int idx)
        {
            Vec2F tangent;
            int last = path.Count() - 1;
            if (idx == 0)
                tangent = path[1] - path[0];
            else if (idx == last)
                tangent = path[last] - path[last - 1];
            else
                tangent = path[idx + 1] - path[idx - 1];

            if (tangent.Length() > 0.0001f)
                tangent.Normalize();

            return Vec2F(-tangent.y, tangent.x);
        };

        Vector<Station> stations;
        stations.Reserve(path.Count() * 2);

        Vec2F prevNormal = computeNormal(0);
        stations.Add({ path[0], prevNormal, imageUV.left, false });

        float accumLen = 0.0f;
        for (int i = 1; i < path.Count(); i++)
        {
            Vec2F segVec = path[i] - path[i - 1];
            float segLen = segVec.Length();
            if (segLen < 0.0001f)
                continue;

            Vec2F currNormal = computeNormal(i);

            float u0 = accumLen / tileLength;
            float u1 = (accumLen + segLen) / tileLength;

            int firstBoundary = (int)floorf(u0) + 1;
            int lastBoundary = (int)floorf(u1);

            for (int k = firstBoundary; k <= lastBoundary; k++)
            {
                // Skip boundaries that coincide with the segment endpoints — they
                // are degenerate and would produce zero-thickness slivers.
                float fk = (float)k;
                if (fk <= u0 + 0.0001f || fk >= u1 - 0.0001f)
                    continue;

                float t = (fk - u0) / (u1 - u0);
                Vec2F seamPos = path[i - 1] + segVec * t;
                Vec2F seamNormal = prevNormal * (1.0f - t) + currNormal * t;
                if (seamNormal.Length() > 0.0001f)
                    seamNormal.Normalize();

                stations.Add({ seamPos, seamNormal, imageUV.right, false });
                stations.Add({ seamPos, seamNormal, imageUV.left,  true  });
            }

            float frac = u1 - floorf(u1);
            float uAtlas = imageUV.left + frac * imageUV.Width();
            stations.Add({ path[i], currNormal, uAtlas, false });

            accumLen += segLen;
            prevNormal = currNormal;
        }

        // 4) Allocate mesh. Two vertices per station (top, bottom), two triangles
        //    per quad — minus the seam quads we skip.
        int stationCount = stations.Count();
        if (stationCount < 2)
            return;

        int quadCount = 0;
        for (int i = 1; i < stationCount; i++)
        {
            if (!stations[i].isQuadStart)
                quadCount++;
        }
        if (quadCount == 0)
            return;

        int vertexCount = stationCount * 2;
        int polyCount = quadCount * 2;
        mMesh.Resize(vertexCount, polyCount);

        Basis transform = mOwner.Lock()->transform->GetWorldNonSizedBasis();
        UInt32 colorARGB = mColor.ARGB();
        float halfW = mWidth * 0.5f;

        Vertex* verts = mMesh.GetVertices<Vertex>();
        for (int i = 0; i < stationCount; i++)
        {
            Vec2F top = stations[i].pos + stations[i].normal * (mOffset + halfW);
            Vec2F bot = stations[i].pos + stations[i].normal * (mOffset - halfW);
            top = top * transform;
            bot = bot * transform;
            verts[i * 2 + 0].Set(top, 1.0f, colorARGB, stations[i].uAtlas, imageUV.top);
            verts[i * 2 + 1].Set(bot, 1.0f, colorARGB, stations[i].uAtlas, imageUV.bottom);
        }

        VertexIndex* idx = mMesh.GetIndexes();
        int triIdx = 0;
        for (int i = 1; i < stationCount; i++)
        {
            if (stations[i].isQuadStart)
                continue;

            int t0 = (i - 1) * 2 + 0;
            int b0 = (i - 1) * 2 + 1;
            int t1 = i * 2 + 0;
            int b1 = i * 2 + 1;

            idx[triIdx * 3 + 0] = t0;
            idx[triIdx * 3 + 1] = b0;
            idx[triIdx * 3 + 2] = t1;
            triIdx++;
            idx[triIdx * 3 + 0] = b0;
            idx[triIdx * 3 + 1] = b1;
            idx[triIdx * 3 + 2] = t1;
            triIdx++;
        }

        mMesh.SetTexture(texture);
        mMesh.vertexCount = vertexCount;
        mMesh.polyCount = polyCount;
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::SplineMeshCollider>);
// --- META ---

DECLARE_CLASS(o2::SplineMeshCollider, o2__SplineMeshCollider);
// --- END META ---
