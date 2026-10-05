#include "o2/stdafx.h"
#include "SkinningMeshComponent.h"

#include "3rdPartyLibs/CDT/include/CDT.h"
#include "o2/Application/Input.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/SkinningMeshBoneComponent.h"
#include "o2/Utils/Editor/Attributes/EditorPropertyAttribute.h"

namespace o2
{
    SkinningMeshComponent::SkinningMeshComponent():
        Component()
    {
        spline->onKeysChanged = THIS_FUNC(UpdateMesh);
        spline->SetClosed(true);
    }

    SkinningMeshComponent::SkinningMeshComponent(const SkinningMeshComponent& other):
        Component(other), mMesh(other.mMesh), spline(other.spline)
    {
        spline->onKeysChanged = THIS_FUNC(UpdateMesh);
        spline->SetClosed(true);
    }

    SkinningMeshComponent::~SkinningMeshComponent()
    {}

    SkinningMeshComponent& SkinningMeshComponent::operator=(const SkinningMeshComponent& other)
    {
        Component::operator=(other);
        spline = other.spline;
        mMesh = other.mMesh;
        return *this;
    }

    void SkinningMeshComponent::OnDraw()
    {
        if (mNeedUpdateMesh)
            UpdateMesh();

        UpdateBonesTransforms();

        mMesh.Reskin();
        mMesh.Draw();

        if (o2Input.IsKeyDown(VK_F3))
            DrawMeshWire();
    }

    void SkinningMeshComponent::OnUpdate(float dt)
    {
        if (mNeedUpdateBones)
            UpdateBones();

        UpdateBonesTransforms();
    }

    void SkinningMeshComponent::UpdateBonesTransforms()
    {
        for (auto& bone : mBonesMapping)
            bone.second->releaseTransform = bone.second->baseTransform.Inverted()*bone.first.Lock()->GetActor()->transform->GetWorldNonSizedBasis();
    }

    void SkinningMeshComponent::UpdateBones()
    {
        mBonesMapping.Clear();

        Function<void(const Ref<Actor>)> searchBones = [&searchBones, this](const Ref<Actor>& actor) {
            if (auto boneComp = actor->GetComponent<SkinningMeshBoneComponent>())
                mBonesMapping.Add({ boneComp, nullptr });

            for (auto& child : actor->GetChildren())
                searchBones(child);
        };

        searchBones(mOwner.Lock());

        mMesh.SetMaxBonesCount(mBonesMapping.Count() + 1);
        mMesh.bonesCount = mBonesMapping.Count() + 1;

        for (int i = 0; i < mMesh.vertexCount; i++)
        {
            memset(mMesh.vertices[i].bones, 0, sizeof(mMesh.vertices[i].bones));
            memset(mMesh.vertices[i].boneWeights, 0, sizeof(mMesh.vertices[i].boneWeights));
        }

        for (int i = 0; i < mBonesMapping.Count(); i++)
        {
            mBonesMapping[i].second = &mMesh.bones[i + 1];
            mBonesMapping[i].second->baseTransform = 
                mBonesMapping[i].first.Lock()->GetActor()->transform->GetWorldNonSizedBasis();

            for (auto& weightPair : mBonesMapping[i].first.Lock()->vertexWeights)
            {
                auto& vertex = mMesh.vertices[weightPair.first];

                int boneId = 0;
                const int maxBoneId = sizeof(vertex.bones)/sizeof(vertex.bones[0]);
                for (; boneId < maxBoneId; boneId++)
                {
                    if (vertex.bones[boneId] == 0)
                        break;
                }

                if (boneId == maxBoneId)
                    o2Debug.LogError("Too much bones affects vertex in skinning mesh, skipping");
                else
                {
                    vertex.bones[boneId] = i + 1;
                    vertex.boneWeights[boneId] = weightPair.second;
                }
            }
        }

        for (int i = 0; i < mMesh.vertexCount; i++)
        {
            auto& v = mMesh.vertices[i];
            float weightsSum = v.boneWeights[0] + v.boneWeights[1] + v.boneWeights[2] + v.boneWeights[3];
            v.boneWeights[0] /= weightsSum;
            v.boneWeights[1] /= weightsSum;
            v.boneWeights[2] /= weightsSum;
            v.boneWeights[3] /= weightsSum;
        }

        mNeedUpdateBones = false;
    }

    void SkinningMeshComponent::OnStart()
    {
        UpdateBones();
    }

    void SkinningMeshComponent::OnTransformUpdated()
    {
        auto newTransform = mOwner.Lock()->transform->GetWorldNonSizedBasis();
        auto delta = newTransform*mTransform.Inverted();
        mTransform = newTransform;

        for (int i = 0; i < mMesh.vertexCount; i++)
        {
            auto v = mMesh.vertices[i];
            Vec2F newPos = v*delta;
            mMesh.vertices[i].Set(newPos, v.z, v.color, v.tu, v.tv);
        }
    }

    void SkinningMeshComponent::UpdateMesh()
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

        mMesh.Resize(triangulation.vertices.size(), triangulation.triangles.size(), 1);

        TextureSource imageSource = mImageAsset ? mImageAsset->GetTextureSource() : TextureSource();
        auto texture = imageSource.texture;
        Vec2F invTexSize(1.0f, 1.0f);
        if (texture)
            invTexSize.Set(1.0f/texture->GetSize().x, 1.0f/texture->GetSize().y);

        RectF imageRect = imageSource.sourceRect;
        RectF imageUV = RectF(imageRect.left*invTexSize.x, 1.0f - imageRect.top*invTexSize.y,
                              imageRect.right*invTexSize.x, 1.0f - imageRect.bottom*invTexSize.y);

        for (int i = 0; i < triangulation.vertices.size(); i++)
        {
            Vec2F p(triangulation.vertices[i].x, triangulation.vertices[i].y);
            Vec2F coef((p.x - mImageMapping.left)/mImageMapping.Width(), (p.y - mImageMapping.bottom)/mImageMapping.Height());
            mMesh.vertices[i].Set(p*mTransform, 1.0f,
                                  mColor.ARGB(),
                                  imageUV.left + coef.x*imageUV.Width(),
                                  imageUV.bottom + coef.y*imageUV.Height());
        }

        for (int i = 0; i < triangulation.triangles.size(); i++)
        {
            mMesh.indexes[i*3] = triangulation.triangles[i].vertices[0];
            mMesh.indexes[i*3 + 1] = triangulation.triangles[i].vertices[1];
            mMesh.indexes[i*3 + 2] = triangulation.triangles[i].vertices[2];
        }

        mMesh.SetTexture(texture);
        mMesh.vertexCount = triangulation.vertices.size();
        mMesh.polyCount = triangulation.triangles.size();
    }

    const SkinningMesh& SkinningMeshComponent::GetMesh() const
    {
        return mMesh;
    }

    void SkinningMeshComponent::SetExtraPoints(const Vector<Vec2F>& points)
    {
        mExtraPoints = points;
        mNeedUpdateMesh = true;
    }

    const Vector<Vec2F>& SkinningMeshComponent::GetExtraPoints() const
    {
        return mExtraPoints;
    }

    void SkinningMeshComponent::SetExtraPoint(int idx, const Vec2F& pos)
    {
        mExtraPoints[idx] = pos;
        mNeedUpdateMesh = true;
    }

    void SkinningMeshComponent::AddExtraPoint(const Vec2F& point)
    {
        mExtraPoints.Add(point);
        mNeedUpdateMesh = true;
    }

    void SkinningMeshComponent::RemoveExtraPoint(int idx)
    {
        mExtraPoints.RemoveAt(idx);
        mNeedUpdateMesh = true;
    }

    void SkinningMeshComponent::SetImage(const AssetRef<ImageAsset>& image)
    {
        mImageAsset = image;
        mNeedUpdateMesh = true;
    }

    const AssetRef<ImageAsset>& SkinningMeshComponent::GetImage() const
    {
        return mImageAsset;
    }

    void SkinningMeshComponent::SetMappingFrame(const RectF& frame)
    {
        mImageMapping = frame;
        mNeedUpdateMesh = true;
    }

    const RectF& SkinningMeshComponent::GetMappingFrame() const
    {
        return mImageMapping;
    }

    void SkinningMeshComponent::SetColor(const Color4& color)
    {
        mColor = color;
        mNeedUpdateMesh = true;
    }

    const Color4& SkinningMeshComponent::GetColor() const
    {
        return mColor;
    }

    void SkinningMeshComponent::NeedUpdateBones(bool force /*= false*/)
    {
        mNeedUpdateBones = true;

        if (force)
            UpdateBones();
    }

    const Vector<Pair<WeakRef<SkinningMeshBoneComponent>, SkinningMesh::Bone*>>& SkinningMeshComponent::GetBones() const
    {
        return mBonesMapping;
    }

    String SkinningMeshComponent::GetName()
    {
        return "Skinning Mesh";
    }

    String SkinningMeshComponent::GetCategory()
    {
        return "Render";
    }

    String SkinningMeshComponent::GetIcon()
    {
        return "ui/UI4_image_component.svg";
    }

    void SkinningMeshComponent::SetOwnerActor(const Ref<Actor>& actor)
    {
        Component::SetOwnerActor(actor);
    }

    void SkinningMeshComponent::OnDeserialized(const DataValue& node)
    {
        Component::OnDeserialized(node);
        UpdateMesh();
    }

    void SkinningMeshComponent::OnDeserializedDelta(const DataValue& node, const IObject& origin)
    {
        Component::OnDeserializedDelta(node, origin);
        UpdateMesh();
    }

    void SkinningMeshComponent::DrawMeshWire() const
    {
        Color4 wireColor(255, 255, 255, 100);
        Vector<Vertex> verticies;
        for (int i = 0; i < mMesh.polyCount; i++)
        {
            auto v = mMesh.vertices[mMesh.indexes[i*3]];
            auto v1 = mMesh.vertices[mMesh.indexes[i*3 + 1]];
            auto v2 = mMesh.vertices[mMesh.indexes[i*3 + 2]];

            verticies.Clear();
            verticies.Add(Vertex(v.x, v.y, 0.0f, wireColor.ABGR(), 0.0f, 0.0f));
            verticies.Add(Vertex(v1.x, v1.y, 0.0f, wireColor.ABGR(), 0.0f, 0.0f));
            verticies.Add(Vertex(v2.x, v2.y, 0.0f, wireColor.ABGR(), 0.0f, 0.0f));
            verticies.Add(Vertex(v.x, v.y, 0.0f, wireColor.ABGR(), 0.0f, 0.0f));
            o2Render.DrawPolyLine(verticies.Data(), verticies.Count());
        }
    }

}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::SkinningMeshComponent>);
// --- META ---

DECLARE_CLASS(o2::SkinningMeshComponent, o2__SkinningMeshComponent);
// --- END META ---
