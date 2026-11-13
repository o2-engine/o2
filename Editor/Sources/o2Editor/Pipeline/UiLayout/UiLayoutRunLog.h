#pragma once

#include "o2/Utils/Function/Function.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutJson.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"

namespace Editor::UiLayout
{
    // ------------------------------------------------------------------------------------------------
    // What the layout agent did in one run, as the editor shows it while the run goes and keeps it in the
    // step afterwards (port of AssetsLine backend/src/uiLayout/runLog.ts): phases and turns - acts
    // {id, kind, title, status, ms?, note?, data?, images?: [{title, file}]} - each with a status, a duration,
    // a line of words and the pictures it looked at. The pictures are kept in the node's work folder,
    // runs/<run id>/NNN.png (the web keeps JPEGs); the last 8 runs of a node stay
    // ------------------------------------------------------------------------------------------------
    class RunLog: public RefCounterable
    {
    public:
        Function<void()> onChange; // Called after every change (the agent's panel follows the run)

    public:
        // The log of a run kept in the folder
        explicit RunLog(const String& dir);

        // The folder of a run in a node's work folder: runs/<run id with anything but letters, digits, _ and - as _>
        static String DirOf(const String& workDir, const String& runId);

        // Makes a fresh folder for the run; the oldest runs of the node go
        void Open();

        // Starts an act (status run); returns its index
        int Begin(const String& id, const String& kind, const String& title);

        // Ends an act: status ok and its duration, then the patch's members (status, note, data...)
        void End(int act, const Json& patch = Json::Object());

        // Acts still marked running (the run was cut short) get a final status
        void Close(const String& status);

        // Keeps a picture with an act: at most max px on its longer side, over the editor's dark card
        void Picture(int act, const String& title, const UiImage& img, int max = 1100);

        // Sets one number or name of an act's data
        void SetData(int act, const String& key, const Json& value);

        // Returns an act
        Json& Act(int act);

        // Returns the acts so far
        const Json& GetActs() const { return mActs; }

        // Returns the run's folder
        const String& GetDir() const { return mDir; }

    private:
        String         mDir;
        Json           mActs = Json::Array();
        Vector<double> mStarted;
        int            mPictures = 0;

    private:
        void Changed();
    };
}
