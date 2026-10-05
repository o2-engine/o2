#pragma once
#include "o2/Assets/Asset.h"
#include "o2/Assets/AssetRef.h"
#include "o2/Scene/ActorLinkRef.h"

namespace o2
{
    FORWARD_CLASS_REF(Actor);

    // -----------
    // Actor asset
    // -----------
    class ActorAsset: public AssetWithDefaultMeta<ActorAsset>
    {
    public:
        // Default constructor
        ActorAsset();

        // Constructor
        ActorAsset(const Ref<Actor>& actor);

        // Copy-constructor
        ActorAsset(const ActorAsset& other);

        // Destructor
        ~ActorAsset();

        // Check equals operator
        ActorAsset& operator=(const ActorAsset& asset);

        // Instantiates actor toscene @SCRIPTABLE
        Ref<Actor> Instantiate() const;

        // Returns actor
        const Ref<Actor>& GetActor() const;

        // Sets actor
        void SetActor(const Ref<Actor>& actor);

        // Returns extensions string
        static Vector<String> GetFileExtensions();

        // Returns editor icon
        static String GetEditorIcon() { return "ui/UI4_actor_icon.svg"; }

        // Returns editor sorting weight
        static int GetEditorSorting() { return 99; }

        // Is this asset type is available to create from editor's assets window
        static bool IsAvailableToCreateFromEditor() { return true; }

        SERIALIZABLE(ActorAsset);
        CLONEABLE_REF(ActorAsset);

    protected:
        Ref<Actor> mActor; // Asset data 

    protected:
        // Itis called when UID has changed; updates actor asset id
        void OnUIDChanged(const UID& oldUID) override;

        // Beginning serialization callback
        void OnSerialize(DataValue& node) const override;

        // Completion deserialization callback
        void OnDeserialized(const DataValue& node) override;

        friend class Assets;
    };
}
// --- META ---

CLASS_BASES_META(o2::ActorAsset)
{
    BASE_CLASS(o2::AssetWithDefaultMeta<ActorAsset>);
}
END_META;
CLASS_FIELDS_META(o2::ActorAsset)
{
    FIELD().PROTECTED().NAME(mActor);
}
END_META;
CLASS_METHODS_META(o2::ActorAsset)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Ref<Actor>&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const ActorAsset&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Ref<Actor>, Instantiate);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Actor>&, GetActor);
    FUNCTION().PUBLIC().SIGNATURE(void, SetActor, const Ref<Actor>&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vector<String>, GetFileExtensions);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetEditorIcon);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(int, GetEditorSorting);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableToCreateFromEditor);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUIDChanged, const UID&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerialize, DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
}
END_META;
// --- END META ---
