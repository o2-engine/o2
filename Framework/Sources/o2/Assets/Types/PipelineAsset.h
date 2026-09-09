#pragma once

#include "o2/Assets/Asset.h"
#include "o2/Assets/AssetRef.h"
#include "o2/Utils/Serialization/DataValue.h"

namespace o2
{
    // -----------------------------------------------------------------------------
    // Pipeline asset: the node graph document of the editor's Pipeline window, kept
    // as plain data so the runtime carries no pipeline code
    // -----------------------------------------------------------------------------
    class PipelineAsset: public AssetWithDefaultMeta<PipelineAsset>
    {
    public:
        DataDocument document; // Graph document as the editor saved it

    public:
        // Default constructor
        PipelineAsset();

        // Copy-constructor
        PipelineAsset(const PipelineAsset& other);

        // Assign operator, copies the document
        PipelineAsset& operator=(const PipelineAsset& other);

        // Returns extensions string
        static Vector<String> GetFileExtensions();

        // Returns editor sorting weight
        static int GetEditorSorting() { return 96; }

        // Returns editor icon
        static String GetEditorIcon() { return "ui/UI4_graph_icon.png"; }

        // Is this asset type is available to create from editor's assets window
        static bool IsAvailableToCreateFromEditor() { return true; }

        // Is asset reference available to contain instance inside
        static bool IsReferenceCanOwnInstance() { return false; }

        SERIALIZABLE(PipelineAsset);
        CLONEABLE_REF(PipelineAsset);

    protected:
        // Loads the document from the file
        void LoadData(const String& path) override;

        // Saves the document into the file
        void SaveData(const String& path) const override;

        friend class Assets;
    };
}
// --- META ---

CLASS_BASES_META(o2::PipelineAsset)
{
    BASE_CLASS(o2::AssetWithDefaultMeta<PipelineAsset>);
}
END_META;
CLASS_FIELDS_META(o2::PipelineAsset)
{
    FIELD().PUBLIC().NAME(document);
}
END_META;
CLASS_METHODS_META(o2::PipelineAsset)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const PipelineAsset&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vector<String>, GetFileExtensions);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(int, GetEditorSorting);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetEditorIcon);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableToCreateFromEditor);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsReferenceCanOwnInstance);
    FUNCTION().PROTECTED().SIGNATURE(void, LoadData, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, SaveData, const String&);
}
END_META;
// --- END META ---
