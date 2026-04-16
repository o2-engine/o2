#pragma once

#include "o2/Assets/Asset.h"
#include "o2/Utils/Serialization/DataValue.h"
#if IS_SCRIPTING_SUPPORTED
#include "o2/Scripts/ScriptEngine.h"
#endif

namespace o2
{
    // -----------------
    // Java Script asset
    // -----------------
    class JavaScriptAsset: public AssetWithDefaultMeta<JavaScriptAsset>
    {
    public:
        // Default constructor
        JavaScriptAsset();

        // Copy-constructor
        JavaScriptAsset(const JavaScriptAsset& asset);

        // Check equals operator
        JavaScriptAsset& operator=(const JavaScriptAsset& asset);

        // Parse script and return parse result
        ScriptParseResult Parse() const;

        // Runs script and returns result
        ScriptValue Run() const;

        // Returns extensions string
        static Vector<String> GetFileExtensions();

        // Returns editor icon
        static String GetEditorIcon();

        // Returns editor sorting weight
        static int GetEditorSorting();

        // Is this asset type is available to create from editor's assets window
        static bool IsAvailableToCreateFromEditor() { return true; }

        SERIALIZABLE(JavaScriptAsset);
        CLONEABLE_REF(JavaScriptAsset);

    protected:
        // Loads data
        void LoadData(const String& path) override;

        // Saves data
        void SaveData(const String& path) const override;

        friend class Assets;
    };
}
// --- META ---

CLASS_BASES_META(o2::JavaScriptAsset)
{
    BASE_CLASS(o2::AssetWithDefaultMeta<JavaScriptAsset>);
}
END_META;
CLASS_FIELDS_META(o2::JavaScriptAsset)
{
}
END_META;
CLASS_METHODS_META(o2::JavaScriptAsset)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const JavaScriptAsset&);
    FUNCTION().PUBLIC().SIGNATURE(ScriptParseResult, Parse);
    FUNCTION().PUBLIC().SIGNATURE(ScriptValue, Run);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vector<String>, GetFileExtensions);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetEditorIcon);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(int, GetEditorSorting);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableToCreateFromEditor);
    FUNCTION().PROTECTED().SIGNATURE(void, LoadData, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, SaveData, const String&);
}
END_META;
// --- END META ---
