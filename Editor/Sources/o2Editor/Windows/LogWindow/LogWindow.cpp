#include "o2Editor/stdafx.h"
#include "LogWindow.h"

#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/List.h"
#include "o2/Scene/UI/Widgets/LongList.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Utils/System/Time/Time.h"

namespace Editor
{
    void LogWindow::Update(float dt)
    {
        if (o2Input.IsKeyDown('H'))
            o2Debug.Log("Regular message " + (String)o2Time.GetLocalTime());

        if (o2Input.IsKeyDown('J'))
            o2Debug.LogWarning("Warning message " + (String)o2Time.GetLocalTime());

        if (o2Input.IsKeyDown('K'))
            o2Debug.LogError("Error message " + (String)o2Time.GetLocalTime());
    }

    Ref<RefCounterable> LogWindow::CastToRefCounterable(const Ref<LogWindow>& ref)
    {
        return DynamicCast<IEditorWindow>(ref);
    }

    LogWindow::LogWindow(RefCounter* refCounter) :
        IEditorWindow(refCounter), LogStream(refCounter)
    {
        InitializeWindow();
        BindStream(o2Debug.GetLog());
    }

    LogWindow::~LogWindow()
    {}

    void LogWindow::InitializeWindow()
    {
        mWindow->caption = "Log";
        mWindow->name = "log window";
        mWindow->SetIcon(mmake<Sprite>("ui/UI4_log_wnd_icon.png"));
        mWindow->SetIconLayout(Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(-1, 1)));
        mWindow->SetViewLayout(Layout::BothStretch(-2, 0, 0, 18));
        mWindow->SetClippingLayout(Layout::BothStretch(-1, 0, 0, 18));

        mList = o2UI.CreateWidget<LongList>();
        *mList->layout = WidgetLayout::BothStretch(0, 18, 0, -1);
        mList->SetViewLayout(Layout::BothStretch());
        mList->getItemsCountFunc = THIS_FUNC(GetVisibleMessagesCount);
        mList->getItemsRangeFunc = THIS_FUNC(GetVisibleMessagesRange);
        mList->setupItemFunc = THIS_FUNC(SetupListMessage);

        auto listItemSample = mmake<Widget>();
        listItemSample->layout->minHeight = 25;
        listItemSample->AddLayer("back", mmake<Sprite>(Color4(0, 0, 0, 255)));
        listItemSample->AddLayer("warning", mmake<Sprite>(Color4(226, 198, 83, 255)),
                                 Layout::VerStretch(HorAlign::Left, 0, 0, 10, 0));
        listItemSample->AddLayer("error", mmake<Sprite>(Color4(248, 94, 72, 255)),
                                 Layout::VerStretch(HorAlign::Left, 0, 0, 10, 0));

        auto captionText = mmake<Text>("stdFont.ttf");
        captionText->text = "text";
        captionText->horAlign = HorAlign::Left;
        captionText->verAlign = VerAlign::Middle;
        captionText->dotsEngings = true;
        captionText->color = Color4(96, 125, 139);
        listItemSample->AddLayer("caption", captionText, Layout::BothStretch(15, 0, 0, 0));

        mList->SetItemSample(listItemSample);

        mWindow->AddChild(mList);

        auto downPanel = mmake<Widget>();
        downPanel->AddLayer("back", mmake<Sprite>("ui/UI4_small_panel_down_back.png"),
                            Layout::BothStretch(-4, -5, -4, -5));
        *downPanel->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 0, 0, 20, 0);
        mWindow->AddChild(downPanel);

        auto clearBtn = o2UI.CreateWidget<Button>("down panel trash");
        *clearBtn->layout = WidgetLayout::Based(BaseCorner::Left, Vec2F(20, 20), Vec2F(0, 0));
        clearBtn->onClick = [&]() { OnClearPressed(); };
        downPanel->AddChild(clearBtn);

        auto messagesToggle = o2UI.CreateWidget<Toggle>("log messages");
        *messagesToggle->layout = WidgetLayout::Based(BaseCorner::Left, Vec2F(60, 20), Vec2F(20, 0));
        mMessagesCountLabel = messagesToggle->GetLayerDrawable<Text>("caption");
        mMessagesCountLabel->text = "0";
        messagesToggle->value = true;
        messagesToggle->onToggle = [&](bool value) { OnRegularMessagesToggled(value); };
        downPanel->AddChild(messagesToggle);

        auto warningsToggle = o2UI.CreateWidget<Toggle>("log warnings");
        *warningsToggle->layout = WidgetLayout::Based(BaseCorner::Left, Vec2F(60, 20), Vec2F(80, 0));
        mWarningsCountLabel = warningsToggle->GetLayerDrawable<Text>("caption");
        mWarningsCountLabel->text = "0";
        warningsToggle->value = true;
        warningsToggle->onToggle = [&](bool value) { OnWarningMessagesToggled(value); };
        downPanel->AddChild(warningsToggle);

        auto errorsToggle = o2UI.CreateWidget<Toggle>("log errors");
        *errorsToggle->layout = WidgetLayout::Based(BaseCorner::Left, Vec2F(60, 20), Vec2F(140, 0));
        mErrorsCountLabel = errorsToggle->GetLayerDrawable<Text>("caption");
        mErrorsCountLabel->text = "0";
        errorsToggle->value = true;
        errorsToggle->onToggle = [&](bool value) { OnErrorMessagesToggled(value); };
        downPanel->AddChild(errorsToggle);

        mLastMessageView = listItemSample->CloneAsRef<Widget>();
        *mLastMessageView->layout = WidgetLayout::BothStretch(200, 1, 0, 1);
        downPanel->AddChild(mLastMessageView);
        mLastMessageView->Hide(true);
    }

    void LogWindow::OnClearPressed()
    {
        mAllMessages.Clear();
        mVisibleMessages.Clear();
        mList->OnItemsUpdated();

        mRegularMessagesCount = 0;
        mWarningMessagesCount = 0;
        mErrorMessagesCount = 0;

        mMessagesCountLabel->text = "0";
        mWarningsCountLabel->text = "0";
        mErrorsCountLabel->text = "0";

        mLastMessageView->Hide(true);
    }

    void LogWindow::OnRegularMessagesToggled(bool value)
    {
        mRegularMessagesEnabled = value;
        UpdateVisibleMessages();
    }

    void LogWindow::OnWarningMessagesToggled(bool value)
    {
        mWarningMessagesEnabled = value;
        UpdateVisibleMessages();
    }

    void LogWindow::OnErrorMessagesToggled(bool value)
    {
        mErrorMessagesEnabled = value;
        UpdateVisibleMessages();
    }

    void LogWindow::UpdateVisibleMessages()
    {
        mVisibleMessages.Clear();

        for (auto& msg : mAllMessages)
        {
            msg.idx = mVisibleMessages.Count();

            if (msg.type == LogMessage::Type::Regular && mRegularMessagesEnabled)
                mVisibleMessages.Add(msg);

            if (msg.type == LogMessage::Type::Warning && mWarningMessagesEnabled)
                mVisibleMessages.Add(msg);

            if (msg.type == LogMessage::Type::Error && mErrorMessagesEnabled)
                mVisibleMessages.Add(msg);
        }

        mList->OnItemsUpdated(true);
    }

    int LogWindow::GetVisibleMessagesCount()
    {
        return mVisibleMessages.Count();
    }

    Vector<void*> LogWindow::GetVisibleMessagesRange(int min, int max)
    {
        Vector<void*> res;
        for (int i = min; i < max; i++)
        {
            if (i == mVisibleMessages.Count())
                break;

            res.Add((void*)(void*)&mVisibleMessages[i]);
        }

        return res;
    }

    void LogWindow::SetupListMessage(const Ref<Widget>& item, void* object)
    {
        LogMessage* message = (LogMessage*)(void*)object;

        item->layer["warning"]->GetDrawable()->enabled = message->type == LogMessage::Type::Warning;
        item->layer["error"]->GetDrawable()->enabled = message->type == LogMessage::Type::Error;
        item->layer["back"]->GetDrawable()->transparency = message->idx % 2 == 1 ? 0.05f : 0.0f;

        auto text = item->GetLayerDrawable<Text>("caption");
        text->text = message->message.SubStr(0, message->message.Find("\n"));
    }

    void LogWindow::OutStrEx(const WString& str)
    {
        bool isScrollDown = Math::Equals(mList->GetScroll().y, mList->GetScrollRange().bottom, 5.0f);

        LogMessage msg;
        msg.message = str;
        msg.type = LogMessage::Type::Regular;
        msg.idx = mAllMessages.Count();

        mAllMessages.Add(msg);
        if (mRegularMessagesEnabled)
        {
            msg.idx = mVisibleMessages.Count();
            mVisibleMessages.Add(msg);
        }

        mList->OnItemsUpdated();

        if (isScrollDown)
            mList->SetScrollForcible(Vec2F(0, mList->GetScrollRange().top));

        mRegularMessagesCount++;
        mMessagesCountLabel->text = (String)mRegularMessagesCount;

        UpdateLastMessageView();
    }

    void LogWindow::OutErrorEx(const WString& str)
    {
        bool isScrollDown = Math::Equals(mList->GetScroll().y, mList->GetScrollRange().bottom, 5.0f);

        LogMessage msg;
        msg.message = str;
        msg.type = LogMessage::Type::Error;
        msg.idx = mAllMessages.Count();

        mAllMessages.Add(msg);
        if (mErrorMessagesEnabled)
        {
            msg.idx = mVisibleMessages.Count();
            mVisibleMessages.Add(msg);
        }

        mList->OnItemsUpdated();

        if (isScrollDown)
            mList->SetScrollForcible(Vec2F(0, mList->GetScrollRange().top));

        mErrorMessagesCount++;
        mErrorsCountLabel->text = (String)mErrorMessagesCount;

        UpdateLastMessageView();
    }

    void LogWindow::OutWarningEx(const WString& str)
    {
        bool isScrollDown = Math::Equals(mList->GetScroll().y, mList->GetScrollRange().bottom, 5.0f);

        LogMessage msg;
        msg.message = str;
        msg.type = LogMessage::Type::Warning;
        msg.idx = mAllMessages.Count();

        mAllMessages.Add(msg);
        if (mWarningMessagesEnabled)
        {
            msg.idx = mVisibleMessages.Count();
            mVisibleMessages.Add(msg);
        }

        mList->OnItemsUpdated();

        if (isScrollDown)
            mList->SetScrollForcible(Vec2F(0, mList->GetScrollRange().top));

        mWarningMessagesCount++;
        mWarningsCountLabel->text = (String)mWarningMessagesCount;

        UpdateLastMessageView();
    }

    void LogWindow::UpdateLastMessageView()
    {
        if (mAllMessages.Count() > 0)
        {
            mLastMessageView->Show(true);
            SetupListMessage(mLastMessageView, (void*)(void*)&mAllMessages.Last());
        }
        else mLastMessageView->Hide(true);
    }

    bool LogWindow::LogMessage::operator==(const LogMessage& other) const
    {
        return type == other.type && message == other.message;
    }

}
// --- META ---

ENUM_META(Editor::LogWindow::LogMessage::Type)
{
    ENUM_ENTRY(Error);
    ENUM_ENTRY(Regular);
    ENUM_ENTRY(Warning);
}
END_ENUM_META;

DECLARE_CLASS(Editor::LogWindow, Editor__LogWindow);
// --- END META ---
