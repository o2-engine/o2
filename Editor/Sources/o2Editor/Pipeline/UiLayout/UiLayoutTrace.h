#pragma once

#include "o2/Utils/Threading/Mutex.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutJson.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"

namespace Editor::UiLayout
{
    // ------------------------------------------------------------------------------------------------
    // The debug trace of one run: every step the node took with the pictures it looked at and the model's
    // answers, then the agent's turns (agent.py appends them to the same file). Written into the node's work
    // folder as trace.json and PNGs; the debug views of every editor read the same file
    // ------------------------------------------------------------------------------------------------
    class Trace: public RefCounterable
    {
    public:
        // Trace written into dir (nothing written when empty) for a design of the size
        Trace(const String& dir, int w, int h);

        // Empties the folder
        void Reset();

        // Stores a picture for a step; returns its entry {title, file, w, h}. name defaults to the next NNN.png
        Json Image(const String& title, const UiImage& img, const String& name = "");

        // Adds a step: images first, then the fields of the step in order (id, title, kind, ms, ...)
        void Step(const Json& fields);

        // Sets the summary of the run
        void SetSummary(const Json& summary);

        // Writes every step so far to trace.json, before another writer (the agent) appends to it
        void Flush();

        // Takes over the steps and the summary another writer appended to trace.json; the design stays the host's
        void Reload();

        // Takes over a kept trace document (a state of the rounds of edits) as it is; the design stays the host's
        void Adopt(const Json& file);

        // Returns the id of the last step, empty without steps
        String LastStepId() const;

        // Removes the last step
        void PopLastStep();

        // Writes trace.json with the finishing time
        void Save();

        // Returns the trace document
        Json GetFile() const;

        // Returns the folder
        const String& GetDir() const { return mDir; }

    private:
        String mDir;
        Json   mFile;
        Json   mSteps;
        Json   mSummary;
        int    mImages = 0;

        mutable Mutex mMutex;
        Mutex         mWriteMutex;

    private:
        // Writes trace.json through a temporary file
        void Write();
    };

    // ISO 8601 time in UTC with milliseconds, as Date#toISOString
    String IsoNow();

    // Milliseconds of a monotonic clock
    double NowMs();

    // Whole milliseconds since t0
    inline double Since(double t0) { return JsRound(NowMs() - t0); }
}
