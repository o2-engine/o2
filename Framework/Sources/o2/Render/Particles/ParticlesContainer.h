#pragma once

#include "o2/Assets/AssetRef.h"
#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Render/Material.h"
#include "o2/Render/Mesh.h"
#include "o2/Render/Particles/Particle.h"
#include "o2/Utils/Basic/ICloneable.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    class ParticlesEmitter;

    // ------------------------------------------------------------------------------------------------------
    // Basic particles container interface. Keeps list of particles, pooling, synchronizing particle graphics
    // ------------------------------------------------------------------------------------------------------
    class ParticlesContainer : public RefCounterable
    {
    public:
        ParticlesEmitter* emitter = nullptr; // Emitter that uses this source

    public:
        virtual void OnParticleEmitted(Particle& particle) {}
        virtual void OnParticleDied(Particle& particle) {}
        virtual void SetMaterial(const Ref<Material>& material) {}

        virtual void Update(Vector<Particle>& particles, int maxParticles) = 0;
        virtual void Draw() = 0;
    };

    // --------------------------------------
    // Particles emitter shape base interface
    // --------------------------------------
    class ParticleSource: public ISerializable, public RefCounterable, public ICloneableRef
    {
    public:
        virtual ~ParticleSource() {}

        virtual Ref<ParticlesContainer> CreateContainer() { return nullptr; }

        SERIALIZABLE(ParticleSource);
        CLONEABLE_REF(ParticleSource);
    };

    // ---------------------------------------------------------------------------
    // Single sprite particle source. Generates particles with single sprite image
    // ---------------------------------------------------------------------------
    class SingleSpriteParticleSource : public ParticleSource
    {
    public:
        AssetRef<ImageAsset> image; // Particle sprite image @SERIALIZABLE

    public:
        Ref<ParticlesContainer> CreateContainer() override;

        SERIALIZABLE(SingleSpriteParticleSource);
        CLONEABLE_REF(SingleSpriteParticleSource);
    };

    // ----------------------------------------------------------------------------
    // Single sprite particles container. Contains mesh and rebuilds it every frame
    // ----------------------------------------------------------------------------
    class SingleSpriteParticlesContainer : public ParticlesContainer
    {
    public:
        Ref<SingleSpriteParticleSource> source; // Source of particles

    public:
        void SetMaterial(const Ref<Material>& material) override;
        void Update(Vector<Particle>& particles, int maxParticles) override;
        void Draw() override;

    private:
        Mesh mParticlesMesh; // Particles mesh
    };

    // ---------------------------------------------------------------------------------------
    // Multi sprite particle source. Generates particles woth changing image sprites over time
    // ---------------------------------------------------------------------------------------
    class MultiSpriteParticleSource : public ParticleSource
    {
    public:
        Vector<AssetRef<ImageAsset>> images; // Particle sprite images @SERIALIZABLE

    public:
        Ref<ParticlesContainer> CreateContainer() override;

        SERIALIZABLE(MultiSpriteParticleSource);
        CLONEABLE_REF(MultiSpriteParticleSource);
    };

    // ----------------------------------------------------------------------------
    // Multi sprite particles container. Contains mesh and rebuilds it every frame
    // ----------------------------------------------------------------------------
    class MultiSpriteParticlesContainer : public ParticlesContainer
    {
    public:
        Ref<MultiSpriteParticleSource> source; // Source of particles

    public:
        void SetMaterial(const Ref<Material>& material) override;
        void Update(Vector<Particle>& particles, int maxParticles) override;
        void Draw() override;

    private:
        struct ImageInfo
        {
            RectF uv;
            Vec2F texSize;
        };

        Mesh mParticlesMesh; // Particles mesh

        Vector<ImageInfo> mImagesCache; // Cached images info
    };
}
// --- META ---

CLASS_BASES_META(o2::ParticleSource)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::ParticleSource)
{
}
END_META;
CLASS_METHODS_META(o2::ParticleSource)
{

    FUNCTION().PUBLIC().SIGNATURE(Ref<ParticlesContainer>, CreateContainer);
}
END_META;

CLASS_BASES_META(o2::SingleSpriteParticleSource)
{
    BASE_CLASS(o2::ParticleSource);
}
END_META;
CLASS_FIELDS_META(o2::SingleSpriteParticleSource)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(image);
}
END_META;
CLASS_METHODS_META(o2::SingleSpriteParticleSource)
{

    FUNCTION().PUBLIC().SIGNATURE(Ref<ParticlesContainer>, CreateContainer);
}
END_META;

CLASS_BASES_META(o2::MultiSpriteParticleSource)
{
    BASE_CLASS(o2::ParticleSource);
}
END_META;
CLASS_FIELDS_META(o2::MultiSpriteParticleSource)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(images);
}
END_META;
CLASS_METHODS_META(o2::MultiSpriteParticleSource)
{

    FUNCTION().PUBLIC().SIGNATURE(Ref<ParticlesContainer>, CreateContainer);
}
END_META;
// --- END META ---
