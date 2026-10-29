#include "o2Editor/stdafx.h"
#include "TextureInspector.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2Editor/UI/Preview/PreviewToolbar.h"

namespace Editor
{
    namespace
    {
        const float statusHeight = 18.0f;
    }

    TextureInspector::TextureInspector(RefCounter* refCounter):
        Widget(refCounter)
    {
        Build();
    }

    TextureInspector::TextureInspector(RefCounter* refCounter, const TextureInspector& other):
        Widget(refCounter), mNote(other.mNote)
    {
        Build();
        SetTexture(other.mView->GetTexture(), other.mView->GetSourceRect());
    }

    TextureInspector& TextureInspector::operator=(const TextureInspector& other)
    {
        mNote = other.mNote;
        SetTexture(other.mView->GetTexture(), other.mView->GetSourceRect());
        return *this;
    }

    void TextureInspector::Build()
    {
        mToolbar = PreviewToolbar::Create();
        AddChild(mToolbar);

        mControls = PreviewToolbar::CreateGroup("texture controls");

        PreviewToolbar::AddButton(mControls, "Fit", 0.0f, [this]() { mView->FitToView(); });
        PreviewToolbar::AddButton(mControls, "1:1", 0.0f, [this]() { mView->SetPixelZoom(1.0f); });
        PreviewToolbar::AddGap(mControls, 6.0f);

        mChannels = PreviewToolbar::AddChoice(mControls, { "RGBA", "RGB", "R", "G", "B", "A" }, { 24.0f },
                                              [this](int position) { mView->SetChannel((TextureView::Channel)position); });
        PreviewToolbar::AddGap(mControls, 6.0f);

        mBackgrounds = PreviewToolbar::AddChoice(mControls, { "Checker", "Black", "White" }, { 0.0f },
                                                 [this](int position) { mView->SetBackground((TextureView::Background)position); });
        PreviewToolbar::AddGap(mControls, 6.0f);

        PreviewToolbar::AddButton(mControls, "Reread", 0.0f, [this]() { mView->Reread(); UpdateStatus(); });

        mToolbar->AddChild(mControls);
        PreviewToolbar::AddLabel(mToolbar, "");

        mView = mmake<TextureView>();
        mView->name = "texture view";
        *mView->layout = WidgetLayout::BothStretch(0, statusHeight, 0, PreviewToolbar::height);
        mView->onHoveredPixelChanged = [this]() { UpdateStatus(); };
        AddChild(mView);

        mStatus = o2UI.CreateLabel("");
        mStatus->name = "status";
        mStatus->horAlign = HorAlign::Left;
        mStatus->horOverflow = Label::HorOverflow::Cut;
        *mStatus->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 4, 4, statusHeight, 0);
        AddChild(mStatus);
    }

    void TextureInspector::SetTexture(const TextureRef& texture, const RectI& sourceRect /*= RectI()*/)
    {
        mView->SetTexture(texture, sourceRect);
        PreviewToolbar::SetChoice(mChannels, (int)mView->GetChannel());
        UpdateStatus();
    }

    void TextureInspector::SetHighlights(const Vector<RectI>& rects)
    {
        mView->SetHighlights(rects);
    }

    void TextureInspector::SetChannel(TextureView::Channel channel)
    {
        mView->SetChannel(channel);
        PreviewToolbar::SetChoice(mChannels, (int)channel);
    }

    void TextureInspector::SetBackground(TextureView::Background background)
    {
        mView->SetBackground(background);
        PreviewToolbar::SetChoice(mBackgrounds, (int)background);
    }

    void TextureInspector::PlaceControlsInto(const Ref<HorizontalLayout>& toolbar, int position)
    {
        mControls->SetParent(toolbar, false, position);

        mToolbar->SetEnabledForcible(false);
        *mView->layout = WidgetLayout::BothStretch(0, statusHeight, 0, 0);
    }

    const Ref<HorizontalLayout>& TextureInspector::GetControls() const
    {
        return mControls;
    }

    const Vector<Ref<Toggle>>& TextureInspector::GetChannelToggles() const
    {
        return mChannels;
    }

    const Vector<Ref<Toggle>>& TextureInspector::GetBackgroundToggles() const
    {
        return mBackgrounds;
    }

    void TextureInspector::SetNote(const WString& note)
    {
        mNote = note;
        UpdateStatus();
    }

    const WString& TextureInspector::GetNote() const
    {
        return mNote;
    }

    const Ref<TextureView>& TextureInspector::GetView() const
    {
        return mView;
    }

    const Ref<HorizontalLayout>& TextureInspector::GetToolbar() const
    {
        return mToolbar;
    }

    WString TextureInspector::GetStatusText() const
    {
        return mStatus->GetText();
    }

    void TextureInspector::UpdateStatus()
    {
        auto& texture = mView->GetTexture();
        if (!texture)
        {
            mStatus->SetText(mNote);
            return;
        }

        const RectI& rect = mView->GetSourceRect();
        String text = mNote.IsEmpty() ? String() : (String)mNote + "  ";
        text += (String)Math::Abs(rect.right - rect.left) + "x" + (String)Math::Abs(rect.bottom - rect.top);
        if (rect != RectI(Vec2I(), texture->GetSize()))
            text += " of " + (String)texture->GetSize().x + "x" + (String)texture->GetSize().y;

        text += ", " + TextureView::GetFormatName(texture->GetFormat()) + ", " +
            TextureView::FormatBytes(TextureView::GetTextureMemory(texture));

        if (!mView->IsReadable())
            text += ", not readable: channels off";

        Vec2I pixel;
        Color4 color;
        if (mView->GetHoveredPixel(pixel, color))
        {
            text += String::Format("  |  x %i, y %i: R %i G %i B %i A %i", pixel.x, pixel.y, color.r, color.g, color.b, color.a);
        }

        mStatus->SetText(text);
    }
}
// --- META ---

DECLARE_CLASS(Editor::TextureInspector, Editor__TextureInspector);
// --- END META ---
