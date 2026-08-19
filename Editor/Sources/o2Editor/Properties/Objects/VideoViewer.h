#pragma once

#include "o2/Render/Video.h"
#include "o2Editor/Properties/IObjectPropertiesViewer.h"

using namespace o2;

namespace Editor
{
    // -------------------------------------------------------------------
    // Editor video viewer. Builds all fields automatically via reflection
    // -------------------------------------------------------------------
    class VideoViewer : public TObjectPropertiesViewer<Video>
    {
    public:
        IOBJECT(VideoViewer);

    protected:
        // Called when the viewer is refreshed, builds properties, and places them in mPropertiesContext
        void RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;
    };
}
// --- META ---

CLASS_BASES_META(Editor::VideoViewer)
{
    BASE_CLASS(Editor::TObjectPropertiesViewer<Video>);
}
END_META;
CLASS_FIELDS_META(Editor::VideoViewer)
{
}
END_META;
CLASS_METHODS_META(Editor::VideoViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;

    FUNCTION().PROTECTED().SIGNATURE(void, RebuildProperties, _tmp1);
}
END_META;
// --- END META ---
