#pragma once

#include "o2/Events/CursorEventsArea.h"
#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Math/Curve.h"
#include "o2/Utils/Singleton.h"

using namespace o2;

namespace o2
{
    class Window;
}

namespace Editor
{
    FORWARD_CLASS_REF(CurvesEditor);

    // --------------------
    // Curve editing dialog
    // --------------------
    class CurveEditorDlg: public Singleton<CurveEditorDlg>, public CursorEventsListener
    {
    public:
        // Default constructor
        CurveEditorDlg(RefCounter* refCounter);

        // Destructor
        ~CurveEditorDlg();

        // Shows editor dialog
        static void Show(const Function<void()>& onChanged, const Function<void()>& onCompleted = {});

        // Adds editing curve with color. If color is default it will be randomized
        static void AddEditingCurve(const String& id, const Ref<Curve>& curve, const Color4& color = Color4::Green());

        // Removed curve from editing
        static void RemoveEditingCurve(const Ref<Curve>& curve);

        // Removed curve from editing
        static void RemoveEditingCurve(const String& id);

        // Removes all editing curves
        static void RemoveAllEditingCurves();

        REF_COUNTERABLE_IMPL(Singleton<CurveEditorDlg>, CursorEventsListener);

    protected:
        Function<void()> mOnChangedCallback;         // On changed callback
        Function<void()> mOnChangeCompletedCallback; // On change completed callback

        Ref<o2::Window>   mWindow;       // Dialog window
        Ref<CurvesEditor> mEditorWidget; // Curves editor widget

    protected:
        // Called when hiding, calls completed callback
        void OnHide();

        // Initialize controls
        void InitializeControls();

        // Called when cursor was pressed outside, closed dialog
        void OnCursorPressedOutside();
    };
     
}
