#pragma once

#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/Ref.h"

using namespace o2;

namespace o2
{
    class IObject;
}

namespace Editor
{
    FORWARD_CLASS_REF(IPropertiesViewer);
    FORWARD_CLASS_REF(DefaultPropertiesViewer);

    // ---------------------------------------------------------------------------------
    // Owns the available property viewers and picks the one matching the edited targets
    // ---------------------------------------------------------------------------------
    class PropertiesViewerSelector
    {
    public:
        // Builds one viewer per IPropertiesViewer type plus the default fallback viewer
        void Initialize();

        // Returns the viewer for the targets: the one whose viewing type the first target is based on,
        // or the default viewer when there are no targets or none of the viewers match
        Ref<IPropertiesViewer> Select(const Vector<IObject*>& targets) const;

        // Returns the viewer from viewers whose viewing type the first target is based on, or null when
        // there are no targets or none of them match
        static Ref<IPropertiesViewer> SelectFrom(const Vector<IObject*>& targets,
                                                 const Vector<Ref<IPropertiesViewer>>& viewers);

    private:
        Vector<Ref<IPropertiesViewer>> mViewers;       // All available object-type viewers
        Ref<DefaultPropertiesViewer>   mDefaultViewer; // Default fallback viewer
    };
}
