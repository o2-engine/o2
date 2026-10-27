#pragma once

#include "o2/Render/TextureRef.h"
#include "o2/Utils/Types/Containers/Map.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2Editor/Pipeline/PipelineValue.h"

#include <atomic>
#include <memory>

using namespace o2;

namespace Editor
{
    // ---------------------------------------------------------------------------------------------------------------
    // Decodes the pictures the cards show off the main thread: a value asked for is decoded on a job worker together
    // with a card-sized copy, and handed into its shared slot between frames. The askers are reported back by name
    // ---------------------------------------------------------------------------------------------------------------
    class PipelineImageLoader : public RefCounterable
    {
    public:
        // Returns the decoded bitmap of an image value; one not decoded yet starts decoding in the background and gives null.
        // The waiter is returned by TakeReady once the picture arrives
        Ref<Bitmap> Request(const PipelineValue& value, const String& waiter);

        // Returns true while the value decodes in the background
        bool IsPending(const PipelineValue& value) const;

        // Returns true while a picture the waiter asked for is on its way
        bool IsWaiting(const String& waiter) const;

        // Hands the finished decodes into their values within the budget; returns the waiters whose pictures arrived
        Vector<String> TakeReady(float budgetMs);

        // Returns the number of decodes waiting or running
        int GetPendingCount() const { return mJobs.Count(); }

        // Returns the number of decodes finished since the queue was last empty
        int GetFinishedCount() const { return mFinished; }

        // Blocks until every decode is finished; returns their waiters (tests)
        Vector<String> WaitAll();

    protected:
        // --------------------------------------------------------------------------
        // Work of one decode: bytes in, bitmaps out. Shared with the worker; holds no
        // main-thread reference, so it may die on either side
        // --------------------------------------------------------------------------
        struct Work
        {
            String            bytes;         // Encoded picture
            Ref<Bitmap>       bitmap;        // Decoded RGBA, written by the worker
            Ref<Bitmap>       display;       // Card-sized copy, written by the worker; null when the bitmap is small
            std::atomic<bool> done { false }; // Set by the worker after the bitmaps
        };

        // ------------------------------------------
        // Decode in flight with its slot and waiters
        // ------------------------------------------
        struct Job
        {
            std::shared_ptr<Work>     work;    // Decode shared with the worker
            Ref<PipelineDecodedImage> slot;    // Where the picture lands
            Vector<String>            waiters; // Who asked for it

            bool operator==(const Job& other) const { return slot == other.slot; }
        };

        Vector<Job>      mJobs;         // Decodes in flight, in the order they were asked for
        Map<String, int> mWaiting;      // Number of decodes each waiter waits for
        int              mFinished = 0; // Decodes finished since the queue was last empty

    protected:
        // Applies a finished decode to its slot and collects its waiters
        void Apply(Job& job, Vector<String>& ready);
    };

    // ---------------------------------------------------------------------------------------------------------------
    // Card-sized textures of decoded pictures, one per bitmap: views showing the same picture share it, and a rebuilt
    // card finds it ready. Main thread only; an entry goes with its bitmap
    // ---------------------------------------------------------------------------------------------------------------
    namespace PipelineDisplayCache
    {
        static constexpr int maxSide = 1024; // Largest side of a card-sized copy

        // Returns the copy of the bitmap downscaled to fit maxSide, the bitmap itself when it fits
        Ref<Bitmap> MakeDisplayCopy(const Bitmap& bitmap);

        // Returns the texture of the bitmap's card-sized copy, made once; size receives the copy's size
        TextureRef GetTexture(const Ref<Bitmap>& bitmap, Vec2I* size = nullptr);

        // Gives the bitmap the card-sized copy made in the background, so its texture is made without downscaling here
        void SetDisplayCopy(const Ref<Bitmap>& bitmap, const Ref<Bitmap>& display);

        // Returns the number of cached textures (tests)
        int GetCount();
    }
}
