#include "o2/stdafx.h"
#include "ParticlesContainer.h"

namespace o2
{
    void SingleSpriteParticlesContainer::SetMaterial(const Ref<Material>& material)
    {
        mParticlesMesh.SetMaterial(material);
    }

    void SingleSpriteParticlesContainer::Update(Vector<Particle>& particles, int maxParticles)
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
            Vec2F xv(cs * hs.x, sn * hs.x);
            Vec2F yv(-sn * hs.y, cs * hs.y);
            Vec2F o(particle.position);
            ULong colr = particle.color.ABGR();

            Vertex* verts = mParticlesMesh.GetVertices<Vertex>();
            verts[mParticlesMesh.vertexCount++].Set(o - xv + yv, colr, uvLeft, uvUp);
            verts[mParticlesMesh.vertexCount++].Set(o + xv + yv, colr, uvRight, uvUp);
            verts[mParticlesMesh.vertexCount++].Set(o + xv - yv, colr, uvRight, uvDown);
            verts[mParticlesMesh.vertexCount++].Set(o - xv - yv, colr, uvLeft, uvDown);

            idx[polyIndex++] = mParticlesMesh.vertexCount - 4;
            idx[polyIndex++] = mParticlesMesh.vertexCount - 3;
            idx[polyIndex++] = mParticlesMesh.vertexCount - 2;

            idx[polyIndex++] = mParticlesMesh.vertexCount - 4;
            idx[polyIndex++] = mParticlesMesh.vertexCount - 2;
            idx[polyIndex++] = mParticlesMesh.vertexCount - 1;
            mParticlesMesh.polyCount += 2;
        }
    }

    void SingleSpriteParticlesContainer::Draw()
    {
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

    void MultiSpriteParticlesContainer::Update(Vector<Particle>& particles, int maxParticles)
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

        float maxImageIdx = (float)(mImagesCache.Count() - 1);

        for (auto& particle : particles)
        {
            if (!particle.alive)
                continue;

            int imageIdx = Math::RoundToInt((1.0f - particle.timeLeft/particle.lifetime)*maxImageIdx);
            auto& imageInfo = mImagesCache[imageIdx];

            float sn = Math::Sin(particle.angle), cs = Math::Cos(particle.angle);
            Vec2F hs = imageInfo.texSize * particle.size * 0.5f;
            Vec2F xv(cs * hs.x, sn * hs.x);
            Vec2F yv(-sn * hs.y, cs * hs.y);
            Vec2F o(particle.position);
            ULong colr = particle.color.ABGR();

            Vertex* verts = mParticlesMesh.GetVertices<Vertex>();
            verts[mParticlesMesh.vertexCount++].Set(o - xv + yv, colr, imageInfo.uv.left, imageInfo.uv.top);
            verts[mParticlesMesh.vertexCount++].Set(o + xv + yv, colr, imageInfo.uv.right, imageInfo.uv.top);
            verts[mParticlesMesh.vertexCount++].Set(o + xv - yv, colr, imageInfo.uv.right, imageInfo.uv.bottom);
            verts[mParticlesMesh.vertexCount++].Set(o - xv - yv, colr, imageInfo.uv.left, imageInfo.uv.bottom);

            idx[polyIndex++] = mParticlesMesh.vertexCount - 4;
            idx[polyIndex++] = mParticlesMesh.vertexCount - 3;
            idx[polyIndex++] = mParticlesMesh.vertexCount - 2;

            idx[polyIndex++] = mParticlesMesh.vertexCount - 4;
            idx[polyIndex++] = mParticlesMesh.vertexCount - 2;
            idx[polyIndex++] = mParticlesMesh.vertexCount - 1;
            mParticlesMesh.polyCount += 2;
        }
    }

    void MultiSpriteParticlesContainer::Draw()
    {
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
