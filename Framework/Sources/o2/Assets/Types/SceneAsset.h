#pragma once

#include "o2/Assets/Asset.h"
#include "o2/Assets/AssetRef.h"

namespace o2
{
    // ----------- 
    // Scene asset 
    // ----------- 
    class SceneAsset: public AssetWithDefaultMeta<SceneAsset>
    {
    public:
        // Default constructor
        SceneAsset();

        // Copy-constructor
        SceneAsset(const SceneAsset& asset);

        // Destructor
        ~SceneAsset();

        // Check equals operator
        SceneAsset& operator=(const SceneAsset& asset);

        // Loads the scene
        void Load() const;

        // Returns extensions string
        static Vector<String> GetFileExtensions();

        // Returns editor sorting weight
        static int GetEditorSorting() { return 96; }

        // Returns editor icon
        static String GetEditorIcon() { return "ui/UI4_big_scene_icon.png"; }

        SERIALIZABLE(SceneAsset);
        CLONEABLE_REF(SceneAsset);

    protected:
        // Loads asset data, using DataValue and serialization
        void LoadData(const String& path) override;

        // Saves asset data, using DataValue and serialization
        void SaveData(const String& path) const override;

        friend class Assets;
    };
}
// --- META ---

CLASS_BASES_META(o2::SceneAsset)
{
    BASE_CLASS(o2::AssetWithDefaultMeta<SceneAsset>);
}
END_META;
CLASS_FIELDS_META(o2::SceneAsset)
{
}
END_META;
CLASS_METHODS_META(o2::SceneAsset)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const SceneAsset&);
    FUNCTION().PUBLIC().SIGNATURE(void, Load);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vector<String>, GetFileExtensions);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(int, GetEditorSorting);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetEditorIcon);
    FUNCTION().PROTECTED().SIGNATURE(void, LoadData, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, SaveData, const String&);
}
END_META;
// --- END META ---
