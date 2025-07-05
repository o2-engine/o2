#pragma once

#include "o2/Events/CursorEventsArea.h"
#include "o2/Render/TextureRef.h"
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Singleton.h"

using namespace o2;

namespace o2
{
	class Label;
	class Widget;
	class WidgetLayer;
	class Window;
}

namespace Editor
{
	// -------------------------
	// Yes/No/Cancel confirmation dialog
	// -------------------------
	class YesNoCancelDlg : public Singleton<YesNoCancelDlg>, public CursorEventsListener
	{
	public:
		// Default constructor
		YesNoCancelDlg(RefCounter* refCounter);

		// Destructor
		~YesNoCancelDlg();

		// Shows confirmation dialog with Yes/No/Cancel buttons
		static void ShowYesNoCancel(const String& message,
									const Function<void()>& onYes,
									const Function<void()>& onNo = Function<void()>(),
									const Function<void()>& onCancel = Function<void()>());

		// Shows confirmation dialog with Yes/No buttons
		static void ShowYesNo(const String& message,
							  const Function<void()>& onYes,
							  const Function<void()>& onNo = Function<void()>());

		REF_COUNTERABLE_IMPL(Singleton<YesNoCancelDlg>, CursorEventsListener);

	protected:
		Function<void()> mOnYesCallback;    // On Yes button callback
		Function<void()> mOnNoCallback;     // On No button callback
		Function<void()> mOnCancelCallback; // On Cancel button callback

		Ref<o2::Window> mWindow;      // Dialog window
		Ref<Label>      mMessageLabel; // Message label
		Ref<Button>     mYesButton;    // Yes button
		Ref<Button>     mNoButton;     // No button
		Ref<Button>     mCancelButton; // Cancel button

	protected:
		// Calls when hiding dialog
		void OnHide();

		// Initializes message label and buttons
		void InitializeControls();

		// Called when Yes button pressed, calls mOnYesCallback and closes window
		void OnYesPressed();

		// Called when No button pressed, calls mOnNoCallback and closes window  
		void OnNoPressed();

		// Called when Cancel button pressed, calls mOnCancelCallback and closes window
		void OnCancelPressed();

		// Called when cursor pressed outside from window
		void OnCursorPressedOutside();
	};
}
