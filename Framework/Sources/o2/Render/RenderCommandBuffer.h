#pragma once

#include "o2/Render/TextureRef.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Math/Rect.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/CommonTypes.h"
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    FORWARD_CLASS_REF(Material);

    // ------------------------------------------------------------------------------------------------
    // A single recorded draw batch: the geometry bytes plus a full snapshot of the GPU state needed to
    // submit it. Recorded on the main thread during the frame and replayed by the render thread. The
    // geometry is copied into the command so the main thread's staging buffer is free to be reused,
    // and the state snapshot means the render thread never reads the live (concurrently mutated) Render
    // members
    // ------------------------------------------------------------------------------------------------
    struct RenderDrawCommand
    {
        Vector<UInt8>       vertexData;             // Copied vertex bytes for this batch
        Vector<VertexIndex> indexData;              // Copied index data for this batch
        UInt                vertexCount = 0;        // Number of vertices
        UInt                indexCount = 0;         // Number of indices
        UInt                trianglesCount = 0;     // Number of triangles

        int                 primitiveType = 0;      // PrimitiveType as int (avoids including the enum)
        int                 vertexStride = 0;       // Bytes per vertex of the batch vertex type
        bool                skinnedVertexType = false; // Whether the batch used the skinned vertex layout

        TextureRef          drawTexture;            // Primary texture
        Ref<Material>       material;               // Material used

        float               mvpMatrix[16] = { 0 };  // Model-view-projection matrix snapshot

        bool                scissorEnabled = false; // Scissor test enabled
        RectI               scissorRect;            // Scissor rectangle

        bool                depthTestEnabled = false; // Depth test enabled
        bool                depthWriteEnabled = true; // Depth write enabled

        TextureRef          renderTarget;           // Current render target (null = back buffer)
        Vector<TextureRef>  extraRenderTargets;     // Extra MRT color targets

        Vec2I               resolution;             // Current back buffer / target resolution

        bool                needClear = false;      // Whether the color buffer must be cleared before this batch
        Color4              clearColor;             // Clear color
        bool                needDepthClear = false; // Whether the depth buffer must be cleared before this batch

        // Drops the held asset references, keeping the geometry storage for the next frame
        void ReleaseReferences()
        {
            drawTexture = TextureRef();
            material = nullptr;
            renderTarget = TextureRef();
            extraRenderTargets.Clear();
        }
    };

    // ------------------------------------------------------------------------------------------------
    // An ordered list of draw commands for one frame. The main thread appends commands while recording;
    // the render thread iterates them to submit the frame. Reset (which drops the held texture/material
    // references) always happens on the main thread, so those references are never ref-counted from the
    // render thread.
    // Commands are pooled: reset keeps them and their geometry storage allocated, so a steady frame
    // records into the same buffers instead of re-allocating a few hundred kilobytes every frame
    // ------------------------------------------------------------------------------------------------
    class RenderCommandBuffer
    {
    public:
        // Appends a command and returns a reference to fill in; reuses a pooled one when available
        RenderDrawCommand& Emplace()
        {
            if (mCount == mCommands.Count())
                mCommands.Add(RenderDrawCommand());

            return mCommands[mCount++];
        }

        // Returns the recorded command by index
        const RenderDrawCommand& Get(int idx) const { return mCommands[idx]; }

        // Returns number of recorded commands
        int Count() const { return mCount; }

        // Returns true if nothing was recorded
        bool IsEmpty() const { return mCount == 0; }

        // Drops the recorded commands, keeping their geometry storage pooled (releases the held
        // texture/material references — call only on the main thread)
        void Reset()
        {
            for (int i = 0; i < mCount; i++)
                mCommands[i].ReleaseReferences();

            mCount = 0;
        }

    protected:
        Vector<RenderDrawCommand> mCommands; // Recorded and pooled commands
        int                       mCount = 0; // Number of commands recorded for the current frame
    };
}
