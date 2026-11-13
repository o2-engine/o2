#pragma once

#include "o2/Utils/Function/Function.h"
#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsDraw.h"

using namespace o2;

namespace Editor
{
    // ---------------------------------------------------------------------------------------------------------------
    // What the layout agent did in a run - reading the reference, the text styles, the graphics it cut, the first layout,
    // the skill's steps, each turn - as a list inside the agent's own panel (port of AssetsLine uiLayout/RunActs.tsx): the
    // same list while the run goes (the running act pulses, the newest stays in view) and in the step afterwards. A row
    // unfolds into what the agent said and the pictures it looked at (kept in the node's work folder, runs/<run id>/)
    // ---------------------------------------------------------------------------------------------------------------
    namespace UiWsRunActs
    {
        // How long something took, short: 42 s, 7 min, 1 h 5 min ("" under half a second)
        String Took(double ms);

        // An act's title in the editor's words
        String ActTitle(const UiDoc::Json& act);

        // An act's numbers in the editor's words ("" while it runs)
        String ActDetail(const UiDoc::Json& act);

        // The line that says what a run is doing now: the running act and how long the run has gone
        String RunNow(const UiDoc::Json& acts, double startedMs, double nowMs);

        // What a finished run came to, in a line: turns, likeness before -> after, kit use, time
        String RunSummary(const UiDoc::Json& run);

        // A picture's title in the editor's words (the run names them in English)
        String PictureTitle(const String& title);

        // The picture of a run's act, decoded once (false when the file is gone)
        bool RunPicture(const String& path, TextureRef& texture, Vec2I& size);

        // -----------------------------------------------------------------------------------------
        // Where the list is drawn and how it talks to its panel: hot spots and hover are the panel's
        // -----------------------------------------------------------------------------------------
        struct Frame
        {
            UiWsDraw::Box                            box;
            float                                    x = 0, w = 0;     // The column, box units
            float                                    top = 0, bottom = 0; // The visible part, box units
            bool                                     draw = false;
            Function<void(const RectF&, const String&)> addHot;
            Function<bool(const String&)>            isHover;
        };

        // Lays out the acts from y (box units) and returns the height they take. key names the run among others (its hot
        // spots are "act:<key>:<act id>"), open holds the unfolded rows by those ids, dir is the folder of their pictures
        float Lay(const Frame& frame, float y, const UiDoc::Json& acts, const String& dir, const String& key, const Vector<String>& open,
                  bool live);
    }
}
