#pragma once

#include "o2/Animation/AnimationClip.h"
#include "o2/Assets/Asset.h"
#include "o2/Assets/AssetRef.h"
#include "o2/Utils/Editor/Attributes/ExpandedByDefaultAttribute.h"

namespace o2
{
    // ---------------
    // Animation asset
    // ---------------
    class AnimationAsset: public AssetWithDefaultMeta<AnimationAsset>
    {
    public:
        Ref<AnimationClip> animation; // Asset data @SERIALIZABLE @EXPANDED_BY_DEFAULT

    public:
        // Default constructor
        AnimationAsset();

        // Copy-constructor
        AnimationAsset(const AnimationAsset& asset);

        // Constructor with animation clip
        explicit AnimationAsset(const Ref<AnimationClip>& clip);

        // Check equals operator
        AnimationAsset& operator=(const AnimationAsset& asset);

        // Returns extensions string
        static Vector<String> GetFileExtensions();

        // Returns editor icon
        static String GetEditorIcon() { return "ui/UI4_anim_file_icon.svg"; }

        // Returns editor sorting weight
        static int GetEditorSorting() { return 98; }

        // Is this asset type is available to create from editor's assets window
        static bool IsAvailableToCreateFromEditor() { return true; }

        // Is asset reference available to contain instance inside
        static bool IsReferenceCanOwnInstance() { return true; }

        SERIALIZABLE(AnimationAsset);
        CLONEABLE_REF(AnimationAsset);

        friend class Assets;
    };
}
// --- META ---

CLASS_BASES_META(o2::AnimationAsset)
{
    BASE_CLASS(o2::AssetWithDefaultMeta<AnimationAsset>);
}
END_META;
CLASS_FIELDS_META(o2::AnimationAsset)
{
    FIELD().PUBLIC().EXPANDED_BY_DEFAULT_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(animation);
}
END_META;
CLASS_METHODS_META(o2::AnimationAsset)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const AnimationAsset&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const Ref<AnimationClip>&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vector<String>, GetFileExtensions);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetEditorIcon);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(int, GetEditorSorting);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableToCreateFromEditor);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsReferenceCanOwnInstance);
}
END_META;
// --- END META ---
