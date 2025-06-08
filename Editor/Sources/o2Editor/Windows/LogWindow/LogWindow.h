#pragma once

#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2Editor/Windows/IEditorWindow.h"

using namespace o2;

namespace o2
{
    class Label;
    class LongList;
    class Text;
}

namespace Editor
{
    // ----------------
    // Log window class
    // ----------------
    class LogWindow : public IEditorWindow, public LogStream
    {
        IOBJECT(LogWindow);

    public:
        // -----------------------------------------------------------------------------------
        // LogMessage class represents a log message with its type, message content, and index
        // -----------------------------------------------------------------------------------        
        class LogMessage
        {
        public:
            // Enum to represent the type of log message (Regular, Warning, Error)
            enum class Type { Regular, Warning, Error };

            Type   type;     // Type of the log message
            String message;  // Content of the log message
            int    idx;      // Index of the log message

            // Overloading the equality operator to compare two LogMessage objects
            bool operator==(const LogMessage& other) const;
        };

    public:
        // Default constructor
        LogWindow(RefCounter* refCounter);

        // Destructor
        ~LogWindow();

        // Updates window logic
        void Update(float dt) override;

        // Dynamic cast to RefCounterable via IEditorWindow
        static Ref<RefCounterable> CastToRefCounterable(const Ref<LogWindow>& ref);

    protected:
        Ref<LongList> mList;            // Reference to the LongList widget
        Ref<Widget>   mLastMessageView; // Reference to the last message view widget

        Ref<Text> mMessagesCountLabel; // Reference to the label displaying the count of all messages
        Ref<Text> mWarningsCountLabel; // Reference to the label displaying the count of warning messages
        Ref<Text> mErrorsCountLabel;   // Reference to the label displaying the count of error messages

        Vector<LogMessage> mAllMessages;    // Vector storing all log messages
        Vector<LogMessage> mVisibleMessages;// Vector storing visible log messages

        bool mRegularMessagesEnabled = true; // Flag indicating if regular messages are enabled
        bool mWarningMessagesEnabled = true; // Flag indicating if warning messages are enabled
        bool mErrorMessagesEnabled = true;   // Flag indicating if error messages are enabled

        int mRegularMessagesCount = 0; // Count of regular messages
        int mWarningMessagesCount = 0; // Count of warning messages
        int mErrorMessagesCount = 0;   // Count of error messages

    protected:
        // Initializes window
        void InitializeWindow();

        // Called when clear button pressed
        void OnClearPressed();

        // Called when regular messages toggled
        void OnRegularMessagesToggled(bool value);

        // Called when warning messages toggled
        void OnWarningMessagesToggled(bool value);

        // Called when error messages toggled
        void OnErrorMessagesToggled(bool value);

        // Updates visible messages
        void UpdateVisibleMessages();

        // Returns visible items count
        int GetVisibleMessagesCount();

        // Returns array of visible messages in range from min to max
        Vector<void*> GetVisibleMessagesRange(int min, int max);

        // Sets list item by message
        void SetupListMessage(const Ref<Widget>& item, void* object);

        // Outs string to stream
        void OutStrEx(const WString& str) override;

        // Outs error to stream
        void OutErrorEx(const WString& str) override;

        // Outs warning to stream
        void OutWarningEx(const WString& str) override;

        // Updates last message view
        void UpdateLastMessageView();

        REF_COUNTERABLE_IMPL(IEditorWindow, LogStream);
    };
}
// --- META ---

PRE_ENUM_META(Editor::LogWindow::LogMessage::Type);

CLASS_BASES_META(Editor::LogWindow)
{
    BASE_CLASS(Editor::IEditorWindow);
    BASE_CLASS(o2::LogStream);
}
END_META;
CLASS_FIELDS_META(Editor::LogWindow)
{
    FIELD().PROTECTED().NAME(mList);
    FIELD().PROTECTED().NAME(mLastMessageView);
    FIELD().PROTECTED().NAME(mMessagesCountLabel);
    FIELD().PROTECTED().NAME(mWarningsCountLabel);
    FIELD().PROTECTED().NAME(mErrorsCountLabel);
    FIELD().PROTECTED().NAME(mAllMessages);
    FIELD().PROTECTED().NAME(mVisibleMessages);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mRegularMessagesEnabled);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mWarningMessagesEnabled);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mErrorMessagesEnabled);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mRegularMessagesCount);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mWarningMessagesCount);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mErrorMessagesCount);
}
END_META;
CLASS_METHODS_META(Editor::LogWindow)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<LogWindow>&);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeWindow);
    FUNCTION().PROTECTED().SIGNATURE(void, OnClearPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRegularMessagesToggled, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnWarningMessagesToggled, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnErrorMessagesToggled, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateVisibleMessages);
    FUNCTION().PROTECTED().SIGNATURE(int, GetVisibleMessagesCount);
    FUNCTION().PROTECTED().SIGNATURE(Vector<void*>, GetVisibleMessagesRange, int, int);
    FUNCTION().PROTECTED().SIGNATURE(void, SetupListMessage, const Ref<Widget>&, void*);
    FUNCTION().PROTECTED().SIGNATURE(void, OutStrEx, const WString&);
    FUNCTION().PROTECTED().SIGNATURE(void, OutErrorEx, const WString&);
    FUNCTION().PROTECTED().SIGNATURE(void, OutWarningEx, const WString&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateLastMessageView);
}
END_META;
// --- END META ---
