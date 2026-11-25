#pragma once

#include <unordered_map>
#include <vector>

#include "o2/Utils/Types/CommonTypes.h"

namespace o2
{
    // --------------------------------------------------------------------------------------------------
    // GPU copies of the batches of retained geometries that come unchanged frame by frame, for the OpenGL
    // render backends. Included after the OpenGL header of the platform; used with the render context bound
    // --------------------------------------------------------------------------------------------------
    class RetainedBatchesGL
    {
    public:
        // Returns is there the copy of the batch data
        bool Has(UInt64 dataId) const
        {
            return mBatches.find(dataId) != mBatches.end();
        }

        // Copies the batch to its own buffers, the indexes get relative to the base vertex; leaves the buffers bound
        void Create(UInt64 dataId, const void* vertices, size_t verticesSize, const VertexIndex* indexes, UInt indexesCount,
                    VertexIndex baseVertex)
        {
            mIndexes.resize(indexesCount);
            for (UInt i = 0; i < indexesCount; i++)
                mIndexes[i] = indexes[i] - baseVertex;

            Batch batch;
            batch.usedFrame = mFrame;

            glGenBuffers(1, &batch.vertexBuffer);
            glBindBuffer(GL_ARRAY_BUFFER, batch.vertexBuffer);
            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)verticesSize, vertices, GL_STATIC_DRAW);

            glGenBuffers(1, &batch.indexBuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.indexBuffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(indexesCount*sizeof(VertexIndex)), mIndexes.data(),
                         GL_STATIC_DRAW);

            mBatches[dataId] = batch;
        }

        // Binds the buffers of the batch copy; false when there is no such
        bool Bind(UInt64 dataId)
        {
            auto found = mBatches.find(dataId);
            if (found == mBatches.end())
                return false;

            found->second.usedFrame = mFrame;
            glBindBuffer(GL_ARRAY_BUFFER, found->second.vertexBuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, found->second.indexBuffer);

            return true;
        }

        // Deletes the copies the last frame has not drawn, or all of them
        void ReleaseUnused(bool all)
        {
            for (auto it = mBatches.begin(); it != mBatches.end();)
            {
                if (all || it->second.usedFrame != mFrame)
                {
                    glDeleteBuffers(1, &it->second.vertexBuffer);
                    glDeleteBuffers(1, &it->second.indexBuffer);
                    it = mBatches.erase(it);
                }
                else
                    ++it;
            }

            mFrame++;
        }

    private:
        struct Batch
        {
            GLuint vertexBuffer = 0;
            GLuint indexBuffer = 0;
            UInt64 usedFrame = 0;
        };

        std::unordered_map<UInt64, Batch> mBatches; // Copies by the batch data id
        std::vector<VertexIndex>          mIndexes; // Indexes of a batch being copied
        UInt64                            mFrame = 0;
    };
}
