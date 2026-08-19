#include "o2Editor/stdafx.h"
#include "VideoViewer.h"

#include "o2Editor/Properties/Properties.h"

namespace Editor
{
    void VideoViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        o2EditorProperties.BuildObjectProperties(mSpoiler, &TypeOf(Video), mPropertiesContext, "",
                                                 mOnPropertyChangeCompleted, mOnPropertyChanged);
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TObjectPropertiesViewer<o2::Video>);
// --- META ---

DECLARE_CLASS(Editor::VideoViewer, Editor__VideoViewer);
// --- END META ---
