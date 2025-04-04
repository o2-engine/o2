#pragma once

#include "o2/Assets/Asset.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Animation/AnimationStateGraph.h"

namespace o2
{
    // -----------
    // Atlas asset
    // -----------
    class AnimationStateGraphAsset: public AssetWithDefaultMeta<AnimationStateGraphAsset>
    {
    public:
        PROPERTIES(AnimationStateGraphAsset);

    public:
        // Default constructor
        AnimationStateGraphAsset();

        // Copy-constructor
        AnimationStateGraphAsset(const AnimationStateGraphAsset& asset);

        // Destructor
        ~AnimationStateGraphAsset() override;

        // Check equals operator
        AnimationStateGraphAsset& operator=(const AnimationStateGraphAsset& asset);

        // Sets initial state
        void SetInitialState(const String& name);

        // Returns initial state
        const String& GetInitialState() const;

        // Adds state to graph
        void AddState(const Ref<AnimationGraphState>& state);

        // Adds state to graph
        Ref<AnimationGraphState> AddState(const String& name, const Vector<String>& animations);

        // Removes state from graph
        void RemoveState(const Ref<AnimationGraphState>& state);

        // Removes state from graph
        void RemoveState(const String& name);

        // Removes all states from graph
        void RemoveAllStates();

        // Returns state by name
        Ref<AnimationGraphState> GetState(const String& name);

        // Returns state by UID
        Ref<AnimationGraphState> GetState(const UID& uid);

        // Returns all states
        const Vector<Ref<AnimationGraphState>>& GetStates() const;

        // Calculates path between two states
        Vector<Ref<AnimationGraphTransition>> CalculatePath(const Ref<AnimationGraphState>& from, const Ref<AnimationGraphState>& to);

        // Returns extensions string
        static Vector<String> GetFileExtensions();

        // Returns editor sorting weight
        static int GetEditorSorting() { return 97; }

        // Returns editor icon
        static String GetEditorIcon() { return "ui/UI4_graph_icon.png"; }

        // Is this asset type is available to create from editor's assets window
        static bool IsAvailableToCreateFromEditor() { return true; }

        // Is asset reference available to contain instance inside
        static bool IsReferenceCanOwnInstance() { return true; }

        SERIALIZABLE(AnimationStateGraphAsset);
        CLONEABLE_REF(AnimationStateGraphAsset);

    protected:
		String mInitialState; // Initial state name @SERIALIZABLE @EDITOR_PROPERTY()

        Vector<Ref<AnimationGraphState>> mStates; // Animation graph states @SERIALIZABLE @EDITOR_PROPERTY()

    protected:
        // Completion deserialization callback
        void OnDeserialized(const DataValue& node) override;
    };
}
// --- META ---

CLASS_BASES_META(o2::AnimationStateGraphAsset)
{
    BASE_CLASS(o2::AssetWithDefaultMeta<AnimationStateGraphAsset>);
}
END_META;
CLASS_FIELDS_META(o2::AnimationStateGraphAsset)
{
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(mInitialState);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(mStates);
}
END_META;
CLASS_METHODS_META(o2::AnimationStateGraphAsset)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const AnimationStateGraphAsset&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetInitialState, const String&);
    FUNCTION().PUBLIC().SIGNATURE(const String&, GetInitialState);
    FUNCTION().PUBLIC().SIGNATURE(void, AddState, const Ref<AnimationGraphState>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<AnimationGraphState>, AddState, const String&, const Vector<String>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveState, const Ref<AnimationGraphState>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveState, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveAllStates);
    FUNCTION().PUBLIC().SIGNATURE(Ref<AnimationGraphState>, GetState, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<AnimationGraphState>, GetState, const UID&);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<AnimationGraphState>>&, GetStates);
    FUNCTION().PUBLIC().SIGNATURE(Vector<Ref<AnimationGraphTransition>>, CalculatePath, const Ref<AnimationGraphState>&, const Ref<AnimationGraphState>&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vector<String>, GetFileExtensions);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(int, GetEditorSorting);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetEditorIcon);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableToCreateFromEditor);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsReferenceCanOwnInstance);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
}
END_META;
// --- END META ---
