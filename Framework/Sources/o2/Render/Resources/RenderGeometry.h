#pragma once

#include "o2/Utils/Math/Vertex.h"
#include "o2/Utils/Types/CommonTypes.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    // ---------------------------------------------------------------------------------------------------
    // Triangles that their owner keeps between frames, drawn by Render::DrawGeometry. Render does not copy
    // them into a batch that the previous frame has recorded from the same geometries of the same versions
    // ---------------------------------------------------------------------------------------------------
    class RenderGeometry: public RefCounterable
    {
    public:
        Vector<Vertex>      vertices;           // Vertices in the space of the camera
        const VertexIndex*  indexes = nullptr;  // Triangles, 3 indexes per triangle; alive while indexesOwner is
        UInt                trianglesCount = 0; // Count of triangles
        Ref<RefCounterable> indexesOwner;       // Keeps the indexes alive

    public:
        // Default constructor
        RenderGeometry();

        // Gives the geometry a new version; must be called after every change of vertices or indexes
        void OnChanged();

        // Returns version of the content: never the same for two contents, of this or of another geometry
        UInt64 GetVersion() const;

        // Returns true when the geometry is drawn into a batch that is not sent to render yet. Such geometry must not
        // be changed: its owner makes another one
        bool IsQueued() const;

    private:
        UInt64 mVersion = 0;     // Version of the content
        int    mQueuedCount = 0; // How many times it is in the batch being filled

        friend class Render;
    };
}
