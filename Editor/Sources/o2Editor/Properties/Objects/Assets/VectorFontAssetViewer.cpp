#include "o2Editor/stdafx.h"
#include "VectorFontAssetViewer.h"

#include "o2/Assets/Types/VectorFontAsset.h"
#include "o2Editor/UI/Preview/FontSampleView.h"

namespace Editor
{
    const Type* VectorFontAssetViewer::GetViewingObjectType() const
    {
        return GetViewingObjectTypeStatic();
    }

    const Type* VectorFontAssetViewer::GetViewingObjectTypeStatic()
    {
        return &TypeOf(VectorFontAsset);
    }
}
// --- META ---

DECLARE_CLASS(Editor::VectorFontAssetViewer, Editor__VectorFontAssetViewer);
// --- END META ---
