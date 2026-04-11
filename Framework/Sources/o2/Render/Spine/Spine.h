#pragma once

#include "o2/Assets/Types/Spine/SpineAsset.h"
#include "o2/Assets/Types/Spine/SpineAtlasAsset.h"
#include "o2/Render/IDrawable.h"
#include "o2/Render/TextureRef.h"
#include "o2/Utils/Basic/ICloneable.h"
#include "o2/Utils/Math/Vertex.h"
#include "o2/Utils/Property.h"
#include "o2/Utils/Types/CommonTypes.h"

#include "3rdPartyLibs/Spine/spine-cpp/include/spine/Skeleton.h"
#include "3rdPartyLibs/Spine/spine-cpp/include/spine/AnimationState.h"
#include "3rdPartyLibs/Spine/spine-cpp/include/spine/Animation.h"

namespace o2
{
    // --------------------------------
    // EsotericSoftware Spine animation
    // --------------------------------
    class Spine : public virtual IDrawable, public virtual RefCounterable, public ICloneableRef
    {
    public:
        PROPERTIES(Spine);

    public:
        // ---------------------
        // Spine animation track
        // ---------------------
        class Track: public IAnimation
        {
        public:
            // Default constructor
            Track() = default;

            // Constructor
            Track(const Ref<Spine>& owner, int trackIndex, const String& name);

			// Destructor
			~Track() override;

            // Sets time
            void SetTime(float time) override;

            // Sets animation weight for blending
            void SetWeight(float weight);

            // Returns animation weight
            float GetWeight() const;

            SERIALIZABLE(Track);

        private:
            WeakRef<Spine> mOwner; // Spine reference

            String mAnimationName;   // Animation name
            int    mTrackIndex = -1; // Spine track index

			spine::TrackEntry* mTrackEntry = nullptr; // Spine track entry
			spine::Animation*  mAnimation = nullptr;  // Spine animation

        private:
            // Called for updating animated object, after updating time
			void Evaluate() override;
            
			// Called when animation starts playing
			void OnPlay() override;

			// Called when animation stops playing
			void OnStop() override;

            // Called when animation loop state changed
            void OnLoopChanged() override;
        };

    public:
        Basis transform; // Root transform                                       

    public:
        // Constructor
        explicit Spine(RefCounter* refCounter);

        // Constructor with spine asset
        Spine(RefCounter* refCounter, const AssetRef<SpineAsset>& spine);

        // Copy-constructor
        Spine(RefCounter* refCounter, const Spine& other);

        // Destructor
        ~Spine();

        // Assign operator
        Spine& operator=(const Spine& other);

        // Load spine skeleton
        void Load(const AssetRef<SpineAsset>& spine);

        // Unload spine skeleton
        void Unload();

        // Drawing SkinnableMesh
        void Draw() override;

        // Updates spine animation
        void Update(float dt);

        // Returns whether skeleton and animation state were created successfully
        bool IsLoaded() const;

        // Returns spine animation names
        const Vector<String>& GetAnimationNames() const;

        // Returns spine animation track
        Ref<Track> GetTrack(const String& name);

        CLONEABLE_REF(Spine);

    protected:
        AssetRef<SpineAsset> mSpineAsset; // Spine asset @SERIALIZABLE @EDITOR_PROPERTY

        spine::Skeleton*       mSkeleton = nullptr;       // Spine skeleton
        spine::AnimationState* mAnimationState = nullptr; // Spine animation state

        Vector<String> mAnimationNames; // Animation names

        Vector<Vertex>      mVertices; // Vertices buffer
        Vector<VertexIndex> mIndices;  // Indices buffer

        friend class Render;
    };
}
// --- META ---

CLASS_BASES_META(o2::Spine::Track)
{
    BASE_CLASS(o2::IAnimation);
}
END_META;
CLASS_FIELDS_META(o2::Spine::Track)
{
    FIELD().PRIVATE().NAME(mOwner);
    FIELD().PRIVATE().NAME(mAnimationName);
    FIELD().PRIVATE().DEFAULT_VALUE(-1).NAME(mTrackIndex);
    FIELD().PRIVATE().DEFAULT_VALUE(nullptr).NAME(mTrackEntry);
    FIELD().PRIVATE().DEFAULT_VALUE(nullptr).NAME(mAnimation);
}
END_META;
CLASS_METHODS_META(o2::Spine::Track)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Ref<Spine>&, int, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTime, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetWeight, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetWeight);
    FUNCTION().PRIVATE().SIGNATURE(void, Evaluate);
    FUNCTION().PRIVATE().SIGNATURE(void, OnPlay);
    FUNCTION().PRIVATE().SIGNATURE(void, OnStop);
    FUNCTION().PRIVATE().SIGNATURE(void, OnLoopChanged);
}
END_META;
// --- END META ---
