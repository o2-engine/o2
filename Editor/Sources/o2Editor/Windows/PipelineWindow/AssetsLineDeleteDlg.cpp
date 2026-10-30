#include "o2Editor/stdafx.h"
#include "AssetsLineDeleteDlg.h"

#include "o2/Render/Text.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/UI/Style/EditorTheme.h"
#include "o2Editor/UI/Style/EditorThemeDark.h"
#include "o2Editor/UIRoot.h"

namespace Editor
{
    namespace AssetsLineDeleteDlg
    {
        static const float kWidth = 470.0f;
        static const float kMargin = 14.0f;
        static const float kButtonsHeight = 22.0f;
        static const int   kListed = 8;

        struct Question
        {
            Vector<String>       names;
            String               folder;
            String               projectName;
            Function<void(bool)> answer;
        };

        // Never destroyed: the window must not outlive the UI at exit
        struct State
        {
            Ref<Window>      window;
            Ref<Label>       message;
            bool             shown = false;
            Question         current;
            Vector<Question> queue;
        };

        static State& St()
        {
            static State* state = new State();
            return *state;
        }

        String Message(const Vector<String>& names, const String& folder, const String& projectName)
        {
            int count = names.Count();
            String place = folder.Trimed(" /\\");
            place = place.IsEmpty() ? String("Assets") : "Assets/" + place;
            String project = projectName.IsEmpty() ? String("the linked AssetsLine project") :
                "the AssetsLine project \"" + projectName + "\"";

            String list;
            for (int i = 0; i < count && i < kListed; i++)
                list += (i > 0 ? ", " : "") + names[i];

            if (count > kListed)
                list += ", and " + (String)(count - kListed) + " more";

            if (count == 1)
                return "1 pipeline is missing in " + place + " but still exists in " + project + ": " + list +
                    ". Delete it in AssetsLine too? Deleted pipelines stay in the AssetsLine trash for 30 days.";

            return (String)count + " pipelines are missing in " + place + " but still exist in " + project + ": " + list +
                ". Delete them in AssetsLine too? Deleted pipelines stay in the AssetsLine trash for 30 days.";
        }

        static void Create()
        {
            PushEditorScopeOnStack scope;
            auto& st = St();
            st.window = o2UI.CreateWindow("AssetsLine sync");
            st.window->name = "assetsline delete question";
            st.window->SetIcon(mmake<VectorSprite>("ui/pipeline/window_icon.svg"));
            st.window->SetIconLayout(Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(-1, 1)));
            *st.window->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(kWidth, 160));
            st.window->SetClippingLayout(Layout::BothStretch(-1, 0, 0, 17));
            st.window->SetViewLayout(Layout::BothStretch(0, 5, 0, 18));

            st.message = o2UI.CreateLabel("");
            st.message->name = "message";
            st.message->horAlign = HorAlign::Left;
            st.message->verAlign = VerAlign::Top;
            st.message->horOverflow = Label::HorOverflow::Wrap;
            st.message->color = EditorTheme::IsDark() ? EditorThemeDark::textStrong : Color4(38, 50, 56, 255);
            *st.message->layout = WidgetLayout::BothStretch(kMargin, kMargin + kButtonsHeight + 12, kMargin, 12);
            st.window->AddChild(st.message);

            auto buttons = mmake<HorizontalLayout>();
            buttons->name = "buttons";
            *buttons->layout = WidgetLayout::HorStretch(VerAlign::Bottom, kMargin, kMargin, kButtonsHeight, kMargin);
            buttons->spacing = 6;
            buttons->expandWidth = false;
            buttons->expandHeight = true;
            buttons->baseCorner = BaseCorner::Right;
            st.window->AddChild(buttons);

            auto addButton = [&](const String& caption, const String& name, float width, bool deleteThere)
            {
                auto button = o2UI.CreateButton(caption, [deleteThere]() { Answer(deleteThere); });
                button->name = name;
                button->layout->minWidth = width;
                button->layout->maxWidth = width;
                buttons->AddChild(button);
            };
            addButton("Delete in AssetsLine", "delete there", 150, true);
            addButton("Keep (download again)", "keep", 160, false);

            st.window->onClosed = []()
            {
                if (St().shown)
                    Answer(false);
            };

            EditorUIRoot.AddWidget(st.window);
            st.window->Hide(true);
        }

        static void ShowNext()
        {
            auto& st = St();
            if (st.shown || st.queue.IsEmpty())
                return;

            if (!st.window)
                Create();

            PushEditorScopeOnStack scope;
            st.current = st.queue[0];
            st.queue.RemoveAt(0);

            String text = Message(st.current.names, st.current.folder, st.current.projectName);
            st.message->text = text;
            float textWidth = kWidth - kMargin*2.0f;
            float textHeight = 16.0f*Math::Max(1, (text.Length()*7 + (int)textWidth - 1)/(int)textWidth);
            if (auto font = st.message->GetFont())
            {
                textHeight = Text::GetTextSize(text, font, st.message->GetHeight(), Vec2F(textWidth, 0), HorAlign::Left,
                                               VerAlign::Top, true).y;
            }

            float height = 18.0f + 5.0f + 12.0f + Math::Ceil(textHeight) + 12.0f + kButtonsHeight + kMargin;
            *st.window->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(kWidth, height));
            if (!st.window->GetParent())
                EditorUIRoot.AddWidget(st.window);

            st.shown = true;
            st.window->ShowModal();
        }

        void Show(const Vector<String>& names, const String& folder, const String& projectName,
                  const Function<void(bool)>& answer)
        {
            St().queue.Add(Question{ names, folder, projectName, answer });
            ShowNext();
        }

        bool IsShown()
        {
            return St().shown;
        }

        int GetQueuedCount()
        {
            return St().queue.Count();
        }

        void Answer(bool deleteThere)
        {
            auto& st = St();
            if (!st.shown)
                return;

            auto answer = st.current.answer;
            st.current = Question();
            st.shown = false;
            {
                PushEditorScopeOnStack scope;
                st.window->Hide(true);
            }

            if (answer)
                answer(deleteThere);

            ShowNext();
        }
    }
}
