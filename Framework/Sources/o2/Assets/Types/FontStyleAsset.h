#pragma once

#include "o2/Assets/Asset.h"
#include "o2/Assets/AssetRef.h"
#include "o2/Render/FontStyle.h"

namespace o2
{
    // ---------------------------------------------------------------------
    // Font style asset. Inherits FontStyle and adds asset serialization.
    // The asset itself IS the font style. Reference can own inline instance
    // ---------------------------------------------------------------------
    class FontStyleAsset: public AssetWithDefaultMeta<FontStyleAsset>, public FontStyle
    {
    public:
        REF_COUNTERABLE_IMPL(Asset, FontStyle);

        template<typename _cast_type>
        Ref<_cast_type> CloneAsRef() const
        {
            return this->AssetWithDefaultMeta<FontStyleAsset>::template CloneAsRef<_cast_type>();
        }

    public:
        // Default constructor
        FontStyleAsset();

        // Copy-constructor
        FontStyleAsset(const FontStyleAsset& asset);

        // Assign operator
        FontStyleAsset& operator=(const FontStyleAsset& asset);

        // Returns supported file extensions (.fntstyle)
        static Vector<String> GetFileExtensions();

        // Returns editor sorting weight
        static int GetEditorSorting() { return 91; }

        // Returns editor icon
        static String GetEditorIcon() { return "ui/UI4_big_font_style_icon.png"; }

        // Returns true because font styles can be created from the editor
        static bool IsAvailableToCreateFromEditor() { return true; }

        // Is asset reference available to contain instance inside
        static bool IsReferenceCanOwnInstance() { return true; }

        // Resolves RefCounterable ambiguity for CloneRef (uses Asset base)
        static Ref<RefCounterable> CastToRefCounterable(const Ref<FontStyleAsset>& ref);

        ASSET_TYPE(FontStyleAsset, Meta);

    protected:
        // Disambiguate On* callbacks from diamond ISerializable inheritance
        void OnSerialize(DataValue& node) const override {}
        void OnDeserialized(const DataValue& node) override;
        void OnSerializeDelta(DataValue& node, const IObject& origin) const override {}
        void OnDeserializedDelta(const DataValue& node, const IObject& origin) override;

    public:
#if IS_SCRIPTING_SUPPORTED
        // Disambiguate GetScriptValue from diamond IObject inheritance
        ScriptValue GetScriptValue() const override { return this->Asset::GetScriptValue(); }
#endif

        friend class Assets;
    };
}
// --- META ---

CLASS_BASES_META(o2::FontStyleAsset)
{
    BASE_CLASS(o2::AssetWithDefaultMeta<FontStyleAsset>);
    BASE_CLASS(o2::FontStyle);
}
END_META;
CLASS_FIELDS_META(o2::FontStyleAsset)
{
}
END_META;
CLASS_METHODS_META(o2::FontStyleAsset)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const FontStyleAsset&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vector<String>, GetFileExtensions);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(int, GetEditorSorting);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetEditorIcon);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableToCreateFromEditor);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsReferenceCanOwnInstance);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<FontStyleAsset>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerialize, DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerializeDelta, DataValue&, const IObject&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserializedDelta, const DataValue&, const IObject&);
#if  IS_SCRIPTING_SUPPORTED
    FUNCTION().PUBLIC().SIGNATURE(ScriptValue, GetScriptValue);
#endif
}
END_META;
// --- END META ---
