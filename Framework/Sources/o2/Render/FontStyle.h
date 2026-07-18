#pragma once

#include "o2/Render/VectorFont.h"
#include "o2/Utils/Editor/Attributes/EditorPropertyAttribute.h"
#include "o2/Utils/Editor/Attributes/ExpandedByDefaultAttribute.h"
#include "o2/Utils/Editor/Attributes/InvokeOnChangeAttribute.h"
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    // ----------------------------------------------------------------------------
    // Font style. Holds a set of vector font effects, applied to rendering glyphs.
    // One font can be drawn with many styles, all styles share the font atlas
    // ----------------------------------------------------------------------------
    class FontStyle: virtual public ISerializable, public RefCounterable, virtual public ICloneableRef
    {
    public:
        Function<void()> onChanged; // Called when effects list was changed

    public:
        // Default constructor
        FontStyle();

        // Copy-constructor. Clones effects
        FontStyle(const FontStyle& other);

        // Assign operator. Clones effects
        FontStyle& operator=(const FontStyle& other);

        // Adds effect
        Ref<VectorFont::Effect> AddEffect(const Ref<VectorFont::Effect>& effect);

        // Adds effect
        template<typename _eff_type, typename ... _args>
        Ref<_eff_type> AddEffect(_args ... args);

        // Removes effect
        void RemoveEffect(const Ref<VectorFont::Effect>& effect);

        // Removes all effects
        void RemoveAllEffects();

        // Sets effects list
        void SetEffects(const Vector<Ref<VectorFont::Effect>>& effects);

        // Returns effects list
        const Vector<Ref<VectorFont::Effect>>& GetEffects() const;

        // Returns key based on effects content. Styles with equal effects share the key
        UInt64 GetCacheKey() const;

        SERIALIZABLE(FontStyle);
        CLONEABLE_REF(FontStyle);

    protected:
        Vector<Ref<VectorFont::Effect>> mEffects; // Font effects list @SERIALIZABLE @EDITOR_PROPERTY @EXPANDED_BY_DEFAULT @INVOKE_ON_CHANGE(OnChanged)

        int mVersion = 0; // Effects list change counter

        mutable UInt64 mCacheKey = 0;         // Cached content key
        mutable int    mCacheKeyVersion = -1; // mVersion at which cache key was computed

    protected:
        // Called when effects list was changed, invokes onChanged
        void OnChanged();

        // Completion deserialization callback, invalidates cache key
        void OnDeserialized(const DataValue& node) override;
    };

    template<typename _eff_type, typename ... _args>
    Ref<_eff_type> FontStyle::AddEffect(_args ... args)
    {
        return DynamicCast<_eff_type>(AddEffect(mmake<_eff_type>(args ...)));
    }
}
// --- META ---

CLASS_BASES_META(o2::FontStyle)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::FontStyle)
{
    FIELD().PUBLIC().NAME(onChanged);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().EXPANDED_BY_DEFAULT_ATTRIBUTE().INVOKE_ON_CHANGE_ATTRIBUTE(OnChanged).SERIALIZABLE_ATTRIBUTE().NAME(mEffects);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mVersion);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mCacheKey);
    FIELD().PROTECTED().DEFAULT_VALUE(-1).NAME(mCacheKeyVersion);
}
END_META;
CLASS_METHODS_META(o2::FontStyle)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const FontStyle&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<VectorFont::Effect>, AddEffect, const Ref<VectorFont::Effect>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveEffect, const Ref<VectorFont::Effect>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveAllEffects);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEffects, const Vector<Ref<VectorFont::Effect>>&);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<VectorFont::Effect>>&, GetEffects);
    FUNCTION().PUBLIC().SIGNATURE(UInt64, GetCacheKey);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
}
END_META;
// --- END META ---
