#include "o2/stdafx.h"
#include "MeshComponent.h"

#include "o2/Scene/Actor.h"
#include "o2/Utils/Editor/Attributes/EditorPropertyAttribute.h"
#include "3rdPartyLibs/CDT/include/CDT.h"
#include "o2/Application/Input.h"

namespace o2
{
    MeshComponent::MeshComponent()
    {
        spline->onKeysChanged = THIS_FUNC(UpdateMesh);
        spline->SetClosed(true);
    }

    MeshComponent::MeshComponent(const MeshComponent& other):
        Component(other), mMesh(other.mMesh), spline(other.spline)
    {
        spline->onKeysChanged = THIS_FUNC(UpdateMesh);
        spline->SetClosed(true);
    }

    MeshComponent::~MeshComponent()
    {}

    MeshComponent& MeshComponent::operator=(const MeshComponent& other)
    {
        Component::operator=(other);
        spline = other.spline;
        mMesh = other.mMesh;
        return *this;
    }

    void MeshComponent::OnDraw()
    {
        if (mNeedUpdateMesh)
            UpdateMesh();

        mMesh.Draw();

        if (o2Input.IsKeyDown(VK_F3))
            o2Render.DrawMeshWire(&mMesh, Color4(0, 0, 0, 100));
    }

    const Mesh& MeshComponent::GetMesh() const
    {
        return mMesh;
    }

    void MeshComponent::SetExtraPoints(const Vector<Vec2F>& points)
    {
        mExtraPoints = points;
        mNeedUpdateMesh = true;
    }

    const Vector<Vec2F>& MeshComponent::GetExtraPoints() const
    {
        return mExtraPoints;
    }

    void MeshComponent::SetExtraPoint(int idx, const Vec2F& pos)
    {
        mExtraPoints[idx] = pos;
        mNeedUpdateMesh = true; 
    }

    void MeshComponent::AddExtraPoint(const Vec2F& point)
    {
        mExtraPoints.Add(point);
        mNeedUpdateMesh = true;
    }

    void MeshComponent::RemoveExtraPoint(int idx)
    {
        mExtraPoints.RemoveAt(idx);
        mNeedUpdateMesh = true;
    }

    void MeshComponent::SetImage(const AssetRef<ImageAsset>& image)
    {
        mImageAsset = image;
        mNeedUpdateMesh = true;
    }

    const AssetRef<ImageAsset>& MeshComponent::GetImage() const
    {
        return mImageAsset;
    }

    void MeshComponent::SetMappingFrame(const RectF& frame)
    {
        mImageMapping = frame;
        mNeedUpdateMesh = true;
    }

    const RectF& MeshComponent::GetMappingFrame() const
    {
        return mImageMapping;
    }

    void MeshComponent::SetColor(const Color4& color)
    {
        mColor = color;
        mNeedUpdateMesh = true;
    }

    const Color4& MeshComponent::GetColor() const
    {
        return mColor;
    }

    String MeshComponent::GetName()
    {
        return "Mesh";
    }

    String MeshComponent::GetCategory()
    {
        return "Render";
    }

    String MeshComponent::GetIcon()
    {
        return "ui/UI4_image_component.png";
    }

    void MeshComponent::OnTransformUpdated()
    {
        auto newTransform = mOwner.Lock()->transform->GetWorldNonSizedBasis();
        auto delta = newTransform*mTransform.Inverted();
        mTransform = newTransform;

        Vertex* verts = mMesh.GetVertices<Vertex>();
        for (int i = 0; i < mMesh.vertexCount; i++)
        {
            auto v = verts[i];
            Vec2F newPos = v*delta;
            verts[i].Set(newPos, v.z, v.color, v.tu, v.tv);
        }
    }

    void MeshComponent::UpdateMesh()
    {
        mNeedUpdateMesh = false;

        if (spline->GetKeys().Count() < 3)
            return;

        std::vector<CDT::V2d<float>> verticies;
        std::vector<CDT::Edge> edges;

        auto pushVertex = [&](const Vec2F& p) {
            verticies.push_back(CDT::V2d<float>::make(p.x, p.y));

            if (verticies.size() > 1)
                edges.push_back(CDT::Edge(verticies.size() - 2, verticies.size() - 1));
        };

        int count = spline->GetKeys().Count();
        for (int i = 0; i < count; i++)
        {
            auto key = spline->GetKey(i);
            auto prevKey = spline->GetKey((i - 1 + count)%count);

            const float noSupportsThreshold = 0.01f;
            if (!(key.prevSupport.Length() < noSupportsThreshold && prevKey.nextSupport.Length() < noSupportsThreshold))
            {
                for (int j = 1; j < key.GetApproximatedPointsCount() - 1; j++)
                    pushVertex(key.GetApproximatedPointsLeft()[j].value);
            }

            pushVertex(key.value);
        }

        edges.push_back(CDT::Edge(verticies.size() - 1, 0));

        for (auto& p : mExtraPoints)
            verticies.push_back(CDT::V2d<float>::make(p.x, p.y));

        CDT::Triangulation<float> triangulation(CDT::VertexInsertionOrder::AsProvided);
        triangulation.insertVertices(verticies);
        triangulation.insertEdges(edges);
        triangulation.eraseOuterTriangles();

        mMesh.Resize(triangulation.vertices.size(), triangulation.triangles.size());

        TextureSource imageSource = mImageAsset ? mImageAsset->GetTextureSource() : TextureSource();
        auto texture = imageSource.texture;
        Vec2F invTexSize(1.0f, 1.0f);
        if (texture)
            invTexSize.Set(1.0f/texture->GetSize().x, 1.0f/texture->GetSize().y);

        RectF imageRect = imageSource.sourceRect;
        RectF imageUV = RectF(imageRect.left*invTexSize.x, 1.0f - imageRect.top*invTexSize.y,
                              imageRect.right*invTexSize.x, 1.0f - imageRect.bottom*invTexSize.y);

        Vertex* verts = mMesh.GetVertices<Vertex>();
        for (int i = 0; i < triangulation.vertices.size(); i++)
        {
            Vec2F p(triangulation.vertices[i].x, triangulation.vertices[i].y);
            Vec2F coef((p.x - mImageMapping.left)/mImageMapping.Width(), (p.y - mImageMapping.bottom)/mImageMapping.Height());
            verts[i].Set(p*mTransform, 1.0f,
                         mColor.ARGB(),
                         imageUV.left + coef.x*imageUV.Width(),
                         imageUV.bottom + coef.y*imageUV.Height());
        }

        VertexIndex* idx = mMesh.GetIndexes();
        for (int i = 0; i < triangulation.triangles.size(); i++)
        {
            idx[i*3] = triangulation.triangles[i].vertices[0];
            idx[i*3 + 1] = triangulation.triangles[i].vertices[1];
            idx[i*3 + 2] = triangulation.triangles[i].vertices[2];
        }

        mMesh.SetTexture(texture);
        mMesh.vertexCount = triangulation.vertices.size();
        mMesh.polyCount = triangulation.triangles.size();
    }

    void MeshComponent::SetOwnerActor(const Ref<Actor>& actor)
    {
        Component::SetOwnerActor(actor);
    }

    void MeshComponent::OnDeserialized(const DataValue& node)
    {
        Component::OnDeserialized(node);
        UpdateMesh();
    }

    void MeshComponent::OnDeserializedDelta(const DataValue& node, const IObject& origin)
    {
        Component::OnDeserializedDelta(node, origin);
        UpdateMesh();
    }

}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::MeshComponent>);
// --- META ---

DECLARE_CLASS(o2::MeshComponent, o2__MeshComponent);
// --- END META ---
