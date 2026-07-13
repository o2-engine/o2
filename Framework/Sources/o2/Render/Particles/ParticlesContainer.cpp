#include "o2/stdafx.h"
#include "ParticlesContainer.h"

#include "o2/Render/Particles/ParticlesEmitter.h"
#include "o2/Render/Render.h"

namespace o2
{
    namespace
    {
        // Returns billboard axes: world XY in 2D mode, camera facing plane in 3D mode
        void GetBillboardAxes(bool is3D, Vec3F& right, Vec3F& up)
        {
            if (is3D)
            {
                Quat rotation = o2Render.GetCamera().GetRotation();
                right = rotation*Vec3F(1, 0, 0);
                up = rotation*Vec3F(0, 1, 0);
            }
            else
            {
                right = Vec3F(1, 0, 0);
                up = Vec3F(0, 1, 0);
            }
        }

        void SetParticleVertex(Vertex& vertex, const Vec3F& position, ULong color, float u, float v)
        {
            vertex.x = position.x;
            vertex.y = position.y;
            vertex.z = position.z;
            vertex.color = (Color32Bit)color;
            vertex.tu = u;
            vertex.tv = v;
        }
    }

    void SingleSpriteParticlesContainer::SetMaterial(const Ref<Material>& material)
    {
        mParticlesMesh.SetMaterial(material);
    }

    void SingleSpriteParticlesContainer::BuildMesh(const Vector<Particle>& particles, int maxParticles,
                                                   const Vec3F& right, const Vec3F& up)
    {
        if (mParticlesMesh.GetMaxVertexCount() < (UInt)maxParticles * 4)
            mParticlesMesh.Resize(maxParticles * 4, maxParticles * 2);

        mParticlesMesh.vertexCount = 0;
        mParticlesMesh.polyCount = 0;
        int polyIndex = 0;
        VertexIndex* idx = mParticlesMesh.GetIndexes();

        RectF textureSrcRect;
        Vec2F imageSize(10, 10);
        Vec2F invTexSize(1.0f, 1.0f);

        auto imageAsset = source->image;
        if (imageAsset)
        {
            auto textureSource = imageAsset->GetTextureSource();
            mParticlesMesh.SetTexture(textureSource.texture);

            invTexSize = Vec2F(1.0f / textureSource.texture->GetSize().x, 1.0f / textureSource.texture->GetSize().y);
            textureSrcRect = textureSource.sourceRect;
            imageSize = imageAsset->GetSize();
        }
        else
            mParticlesMesh.SetTexture(TextureRef::Null());

        float uvLeft = textureSrcRect.left * invTexSize.x;
        float uvRight = textureSrcRect.right * invTexSize.x;
        float uvUp = 1.0f - textureSrcRect.bottom * invTexSize.y;
        float uvDown = 1.0f - textureSrcRect.top * invTexSize.y;

        for (auto& particle : particles)
        {
            if (!particle.alive)
                continue;

            float sn = Math::Sin(particle.angle), cs = Math::Cos(particle.angle);
            Vec2F hs = imageSize * particle.size * 0.5f;
            Vec3F xv = (right*cs + up*sn)*hs.x;
            Vec3F yv = (right*-sn + up*cs)*hs.y;
            Vec3F o = particle.position;
            ULong colr = particle.color.ABGR();

            Vertex* verts = mParticlesMesh.GetVertices<Vertex>();
            SetParticleVertex(verts[mParticlesMesh.vertexCount++], o - xv + yv, colr, uvLeft, uvUp);
            SetParticleVertex(verts[mParticlesMesh.vertexCount++], o + xv + yv, colr, uvRight, uvUp);
            SetParticleVertex(verts[mParticlesMesh.vertexCount++], o + xv - yv, colr, uvRight, uvDown);
            SetParticleVertex(verts[mParticlesMesh.vertexCount++], o - xv - yv, colr, uvLeft, uvDown);

            idx[polyIndex++] = mParticlesMesh.vertexCount - 4;
            idx[polyIndex++] = mParticlesMesh.vertexCount - 3;
            idx[polyIndex++] = mParticlesMesh.vertexCount - 2;

            idx[polyIndex++] = mParticlesMesh.vertexCount - 4;
            idx[polyIndex++] = mParticlesMesh.vertexCount - 2;
            idx[polyIndex++] = mParticlesMesh.vertexCount - 1;
            mParticlesMesh.polyCount += 2;
        }
    }

    void SingleSpriteParticlesContainer::Update(Vector<Particle>& particles, int maxParticles)
    {
        // 3D billboards depend on the drawing camera, the mesh is rebuilt at Draw
        if (emitter && emitter->Is3D())
            return;

        BuildMesh(particles, maxParticles, Vec3F(1, 0, 0), Vec3F(0, 1, 0));
    }

    void SingleSpriteParticlesContainer::Draw()
    {
        if (emitter && emitter->Is3D())
        {
            Vec3F right, up;
            GetBillboardAxes(true, right, up);
            BuildMesh(emitter->GetParticles(), emitter->GetMaxParticles(), right, up);
        }

        mParticlesMesh.Draw();
    }

    Ref<ParticlesContainer> SingleSpriteParticleSource::CreateContainer()
    {
        auto container = mmake<SingleSpriteParticlesContainer>();
        container->source = Ref(this);
        return container;
    }

    void MultiSpriteParticlesContainer::SetMaterial(const Ref<Material>& material)
    {
        mParticlesMesh.SetMaterial(material);
    }

    void MultiSpriteParticlesContainer::BuildMesh(const Vector<Particle>& particles, int maxParticles,
                                                  const Vec3F& right, const Vec3F& up)
    {
        if (mParticlesMesh.GetMaxVertexCount() < (UInt)maxParticles * 4)
            mParticlesMesh.Resize(maxParticles * 4, maxParticles * 2);

        mParticlesMesh.vertexCount = 0;
        mParticlesMesh.polyCount = 0;
        int polyIndex = 0;
        VertexIndex* idx = mParticlesMesh.GetIndexes();

        Vec2F invTexSize(1.0f, 1.0f);

        auto imageAsset = source->images.IsEmpty() ? nullptr : source->images[0];
        if (imageAsset)
        {
            auto texture = TextureRef(imageAsset->GetAtlasUID(), imageAsset->GetAtlasPage());
            mParticlesMesh.SetTexture(texture);

            invTexSize = Vec2F(1.0f / texture->GetSize().x, 1.0f / texture->GetSize().y);
        }
        else
            mParticlesMesh.SetTexture(TextureRef::Null());

        mImagesCache.Clear();
        for (auto& imageAsset : source->images)
        {
            ImageInfo info;

            info.texSize = imageAsset->GetSize();

            RectF textureSrcRect = imageAsset->GetAtlasRect();
            info.uv.left = textureSrcRect.left * invTexSize.x;
            info.uv.right = textureSrcRect.right * invTexSize.x;
            info.uv.top = 1.0f - textureSrcRect.bottom * invTexSize.y;
            info.uv.bottom = 1.0f - textureSrcRect.top * invTexSize.y;

            mImagesCache.push_back(info);
        }

        if (mImagesCache.IsEmpty())
            return;

        float maxImageIdx = (float)(mImagesCache.Count() - 1);

        for (auto& particle : particles)
        {
            if (!particle.alive)
                continue;

            int imageIdx = Math::RoundToInt((1.0f - particle.timeLeft/particle.lifetime)*maxImageIdx);
            auto& imageInfo = mImagesCache[imageIdx];

            float sn = Math::Sin(particle.angle), cs = Math::Cos(particle.angle);
            Vec2F hs = imageInfo.texSize * particle.size * 0.5f;
            Vec3F xv = (right*cs + up*sn)*hs.x;
            Vec3F yv = (right*-sn + up*cs)*hs.y;
            Vec3F o = particle.position;
            ULong colr = particle.color.ABGR();

            Vertex* verts = mParticlesMesh.GetVertices<Vertex>();
            SetParticleVertex(verts[mParticlesMesh.vertexCount++], o - xv + yv, colr, imageInfo.uv.left, imageInfo.uv.top);
            SetParticleVertex(verts[mParticlesMesh.vertexCount++], o + xv + yv, colr, imageInfo.uv.right, imageInfo.uv.top);
            SetParticleVertex(verts[mParticlesMesh.vertexCount++], o + xv - yv, colr, imageInfo.uv.right, imageInfo.uv.bottom);
            SetParticleVertex(verts[mParticlesMesh.vertexCount++], o - xv - yv, colr, imageInfo.uv.left, imageInfo.uv.bottom);

            idx[polyIndex++] = mParticlesMesh.vertexCount - 4;
            idx[polyIndex++] = mParticlesMesh.vertexCount - 3;
            idx[polyIndex++] = mParticlesMesh.vertexCount - 2;

            idx[polyIndex++] = mParticlesMesh.vertexCount - 4;
            idx[polyIndex++] = mParticlesMesh.vertexCount - 2;
            idx[polyIndex++] = mParticlesMesh.vertexCount - 1;
            mParticlesMesh.polyCount += 2;
        }
    }

    void MultiSpriteParticlesContainer::Update(Vector<Particle>& particles, int maxParticles)
    {
        // 3D billboards depend on the drawing camera, the mesh is rebuilt at Draw
        if (emitter && emitter->Is3D())
            return;

        BuildMesh(particles, maxParticles, Vec3F(1, 0, 0), Vec3F(0, 1, 0));
    }

    void MultiSpriteParticlesContainer::Draw()
    {
        if (emitter && emitter->Is3D())
        {
            Vec3F right, up;
            GetBillboardAxes(true, right, up);
            BuildMesh(emitter->GetParticles(), emitter->GetMaxParticles(), right, up);
        }

        mParticlesMesh.Draw();
    }

    Ref<ParticlesContainer> MultiSpriteParticleSource::CreateContainer()
    {
        auto container = mmake<MultiSpriteParticlesContainer>();
        container->source = Ref(this);
        return container;
    }
}
// --- META ---

DECLARE_CLASS(o2::ParticleSource, o2__ParticleSource);

DECLARE_CLASS(o2::SingleSpriteParticleSource, o2__SingleSpriteParticleSource);

DECLARE_CLASS(o2::MultiSpriteParticleSource, o2__MultiSpriteParticleSource);
// --- END META ---
