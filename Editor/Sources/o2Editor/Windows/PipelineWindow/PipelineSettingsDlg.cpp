#include "o2Editor/stdafx.h"
#include "PipelineSettingsDlg.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Pipeline/PipelineSettings.h"
#include "o2Editor/UIRoot.h"

DECLARE_SINGLETON(Editor::PipelineSettingsDlg);

namespace Editor
{
    PipelineSettingsDlg::PipelineSettingsDlg(RefCounter* refCounter):
        Singleton<PipelineSettingsDlg>(refCounter)
    {
        PushEditorScopeOnStack scope;
        InitializeControls();
    }

    PipelineSettingsDlg::~PipelineSettingsDlg()
    {}

    void PipelineSettingsDlg::InitializeControls()
    {
        mWindow = o2UI.CreateWindow("Pipeline settings");
        *mWindow->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(460, 260));
        mWindow->SetClippingLayout(Layout::BothStretch(-1, 0, 0, 17));
        mWindow->SetViewLayout(Layout::BothStretch(5, 5, 5, 20));

        auto content = mmake<VerticalLayout>();
        *content->layout = WidgetLayout::BothStretch(10, 10, 10, 10);
        content->spacing = 6;
        content->expandWidth = true;
        content->expandHeight = false;
        content->fitByChildren = false;
        content->baseCorner = BaseCorner::Top;
        mWindow->AddChild(content);

        auto addField = [&](const String& caption, Ref<EditBox>& edit)
        {
            auto label = o2UI.CreateLabel(caption);
            label->horAlign = HorAlign::Left;
            label->layout->minHeight = 18;
            content->AddChild(label);

            edit = o2UI.CreateWidget<EditBox>("singleline");
            edit->layout->minHeight = 22;
            content->AddChild(edit);
        };

        addField("Gemini API key (images, text, speech, music, Veo)", mGeminiEdit);
        addField("ElevenLabs API key (sound effects, expressive speech)", mElevenEdit);
        addField("Kling API key or access key", mKlingAccessEdit);
        addField("Kling secret key (legacy key pairs only)", mKlingSecretEdit);

        auto buttons = mmake<HorizontalLayout>();
        buttons->layout->minHeight = 26;
        buttons->spacing = 8;
        buttons->expandWidth = false;
        buttons->expandHeight = true;
        buttons->baseCorner = BaseCorner::Right;
        content->AddChild(buttons);

        auto save = o2UI.CreateButton("Save", [this]() { OnSavePressed(); });
        save->layout->minWidth = 90;
        buttons->AddChild(save);

        auto cancel = o2UI.CreateButton("Cancel", [this]() { mWindow->Hide(); });
        cancel->layout->minWidth = 90;
        buttons->AddChild(cancel);

        mWindow->Hide(true);
        EditorUIRoot.AddWidget(mWindow);
    }

    void PipelineSettingsDlg::Show()
    {
        if (!IsSingletonInitialzed())
            mmake<PipelineSettingsDlg>();

        auto& dlg = Instance();
        auto settings = PipelineSettings::Load();
        dlg.mGeminiEdit->SetText(settings.geminiApiKey);
        dlg.mElevenEdit->SetText(settings.elevenLabsApiKey);
        dlg.mKlingAccessEdit->SetText(settings.klingAccessKey);
        dlg.mKlingSecretEdit->SetText(settings.klingSecretKey);
        dlg.mWindow->ShowModal();
    }

    void PipelineSettingsDlg::OnSavePressed()
    {
        PipelineSettings settings;
        settings.geminiApiKey = (String)mGeminiEdit->GetText();
        settings.elevenLabsApiKey = (String)mElevenEdit->GetText();
        settings.klingAccessKey = (String)mKlingAccessEdit->GetText();
        settings.klingSecretKey = (String)mKlingSecretEdit->GetText();
        settings.Save();
        mWindow->Hide();
    }
}
