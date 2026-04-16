#include "o2/stdafx.h"
#include "Label.h"

#include "o2/Application/Input.h"
#include "o2/Render/Render.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"

namespace o2
{
    Label::Label(RefCounter* refCounter):
        Widget(refCounter)
    {
        CreateDefaultText();
    }

    Label::Label(RefCounter* refCounter, const Label& other):
        Widget(refCounter, other), mHorOverflow(other.mHorOverflow), mVerOverflow(other.mVerOverflow),
        mExpandBorder(other.mExpandBorder)
    {
        mTextDrawable = GetLayerDrawable<Text>("text");
        if (!mTextDrawable)
            CreateDefaultText();
        
        RetargetStatesAnimations();
        SetLayoutDirty();
    }

    Label& Label::operator=(const Label& other)
    {
        Widget::operator=(other);

        mTextDrawable = GetLayerDrawable<Text>("text");
        mHorOverflow = other.mHorOverflow;
        mVerOverflow = other.mVerOverflow;
        mExpandBorder = other.mExpandBorder;

        if (!mTextDrawable)
            CreateDefaultText();

        RetargetStatesAnimations();
        SetLayoutDirty();

        return *this;
    }

    void Label::Draw()
    {
        PROFILE_SAMPLE_FUNC();

        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        bool enabledClipping = false;
        if (mHorOverflow == HorOverflow::Cut || mVerOverflow == VerOverflow::Cut)
        {
            enabledClipping = true;

            Vec2I halfRes =  (Vec2F)o2Render.GetResolution()*0.5f;
            RectI cutRect(-halfRes.x, halfRes.y, halfRes.x, -halfRes.y);

            if (mHorOverflow == HorOverflow::Cut)
            {
                cutRect.left = (int)layout->worldLeft;
                cutRect.right = (int)layout->worldRight;
            }

            if (mVerOverflow == VerOverflow::Cut)
            {
                cutRect.top = (int)layout->worldTop;
                cutRect.bottom = (int)layout->worldBottom;
            }

            o2Render.EnableScissorTest(cutRect);
        }

        for (auto& layer : mDrawingLayers)
            layer->Draw();

        if (enabledClipping)
            o2Render.DisableScissorTest();

        ISceneDrawable::Draw();

        DrawDebugFrame();
    }

    void Label::SetFont(const Ref<Font>& font)
    {
        if (mTextDrawable)
            mTextDrawable->SetFont(font);

        SetLayoutDirty();
    }

    Ref<Font> Label::GetFont() const
    {
        if (mTextDrawable)
            return mTextDrawable->GetFont();

        return Ref<Font>();
    }

    void Label::SetFontAsset(const AssetRef<FontAsset>& asset)
    {
        if (mTextDrawable)
            mTextDrawable->SetFontAsset(asset);
    }

    AssetRef<FontAsset> Label::GetFontAsset() const
    {
        if (mTextDrawable)
            return mTextDrawable->GetFontAsset();

        return AssetRef<FontAsset>();
    }

    void Label::SetText(const WString& text)
    {
        if (mTextDrawable)
            mTextDrawable->SetText(text);

        if (mHorOverflow == HorOverflow::Expand || mVerOverflow == VerOverflow::Expand)
            SetLayoutDirty();
    }

    const WString& Label::GetText() const
    {
        if (mTextDrawable)
            return mTextDrawable->GetText();

        return WString::empty;
    }

    void Label::SetColor(const Color4& color)
    {
        if (mTextDrawable)
            mTextDrawable->SetColor(color);
    }

    Color4 Label::GetColor() const
    {
        if (mTextDrawable)
            return mTextDrawable->GetColor();

        return Color4();
    }

    void Label::SetHorAlign(HorAlign align)
    {
        if (mTextDrawable)
            mTextDrawable->SetHorAlign(align);

        SetLayoutDirty();
    }

    HorAlign Label::GetHorAlign() const
    {
        if (mTextDrawable)
            return mTextDrawable->GetHorAlign();

        return HorAlign::Left;
    }

    void Label::SetVerAlign(VerAlign align)
    {
        if (mTextDrawable)
            mTextDrawable->SetVerAlign(align);

        SetLayoutDirty();
    }

    VerAlign Label::GetVerAlign() const
    {
        if (mTextDrawable)
            return mTextDrawable->GetVerAlign();

        return VerAlign::Top;
    }

    void Label::SetHorOverflow(HorOverflow overflow)
    {
        mHorOverflow = overflow;

        if (mTextDrawable)
        {
            mTextDrawable->wordWrap = mHorOverflow == HorOverflow::Wrap;
            mTextDrawable->dotsEngings = mHorOverflow == HorOverflow::Dots;
        }

        SetLayoutDirty();
    }

    Label::HorOverflow Label::GetHorOverflow()
    {
        return mHorOverflow;
    }

    void Label::SetVerOverflow(VerOverflow overflow)
    {
        mVerOverflow = overflow;
        SetLayoutDirty();
    }

    Label::VerOverflow Label::GetVerOverflow()
    {
        return mVerOverflow;
    }

    void Label::SetSymbolsDistanceCoef(float coef /*= 1*/)
    {
        if (mTextDrawable)
            mTextDrawable->SetSymbolsDistanceCoef(coef);

        SetLayoutDirty();
    }

    float Label::GetSymbolsDistanceCoef() const
    {
        if (mTextDrawable)
            return mTextDrawable->GetSymbolsDistanceCoef();

        return 1.0f;
    }

    void Label::SetLinesDistanceCoef(float coef /*= 1*/)
    {
        if (mTextDrawable)
            mTextDrawable->SetLinesDistanceCoef(coef);

        SetLayoutDirty();
    }

    float Label::GetLinesDistanceCoef() const
    {
        if (mTextDrawable)
            return mTextDrawable->GetLinesDistanceCoef();

        return 1.0f;
    }

    void Label::SetExpandBorder(const Vec2F& border)
    {
        mExpandBorder = border;
        SetLayoutDirty();
    }

    Vec2F Label::GetExpandBorder() const
    {
        return mExpandBorder;
    }

    void Label::SetHeight(int height)
    {
        if (mTextDrawable)
            mTextDrawable->SetHeight(height);
    }

    int Label::GetHeight() const
    {
        if (mTextDrawable)
            return mTextDrawable->GetFontHeight();

        return 0;
    }

    void Label::UpdateSelfTransform()
{
        if (mTextDrawable)
        {
            if (mHorOverflow == HorOverflow::Expand)
            {
                layout->Update();

                float realSize = mTextDrawable->GetRealSize().x + mExpandBorder.x*2.0f;
                float thisSize = layout->width;
                float sizeDelta = realSize - thisSize;
                GetLayoutData().minSize.x = realSize;

                switch (mTextDrawable->GetHorAlign())
                {
                    case HorAlign::Left:
                    GetLayoutData().offsetMax.x += sizeDelta;
                    break;

                    case HorAlign::Middle:
                    case HorAlign::Both:
                    GetLayoutData().offsetMax.x += sizeDelta*0.5f;
                    GetLayoutData().offsetMin.x -= sizeDelta*0.5f;
                    break;

                    case HorAlign::Right:
                    GetLayoutData().offsetMin.x -= sizeDelta;
                    break;
                }
            }

            if (mVerOverflow == VerOverflow::Expand)
            {
                layout->Update();

                float realSize = mTextDrawable->GetRealSize().y + mExpandBorder.y*2.0f;
                float thisSize = layout->height;
                float sizeDelta = realSize - thisSize;

                switch (mTextDrawable->GetVerAlign())
                {
                    case VerAlign::Top:
                    GetLayoutData().offsetMin.y -= sizeDelta;
                    break;

                    case VerAlign::Middle:
                    case VerAlign::Both:
                    GetLayoutData().offsetMax.y += sizeDelta*0.5f;
                    GetLayoutData().offsetMin.y -= sizeDelta*0.5f;
                    break;

                    case VerAlign::Bottom:
                    GetLayoutData().offsetMax.y += sizeDelta;
                    break;
                }
            }
        }

        layout->Update();
    }

    void Label::OnLayerAdded(const Ref<WidgetLayer>& layer)
    {
        if (layer->name == "text" && layer->GetDrawable() && layer->GetDrawable()->GetType() == TypeOf(Text))
            mTextDrawable = DynamicCast<Text>(layer->GetDrawable());
    }

    void Label::CreateDefaultText()
    {
        mTextDrawable = DynamicCast<Text>(AddLayer("text", mmake<Text>())->GetDrawable());
        mTextDrawable->SetFontAsset(AssetRef<VectorFontAsset>("stdFont.ttf"));
    }

    void Label::OnDeserialized(const DataValue& node)
    {
        Widget::OnDeserialized(node);
        mTextDrawable = GetLayerDrawable<Text>("text");
    }

    String Label::GetCreateMenuGroup()
    {
        return "Basic";
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::Label>);
// --- META ---

ENUM_META(o2::Label::HorOverflow, o2__Label__HorOverflow)
{
    ENUM_ENTRY(Cut);
    ENUM_ENTRY(Dots);
    ENUM_ENTRY(Expand);
    ENUM_ENTRY(None);
    ENUM_ENTRY(Wrap);
}
END_ENUM_META;

ENUM_META(o2::Label::VerOverflow, o2__Label__VerOverflow)
{
    ENUM_ENTRY(Cut);
    ENUM_ENTRY(Expand);
    ENUM_ENTRY(None);
}
END_ENUM_META;

DECLARE_CLASS(o2::Label, o2__Label);
// --- END META ---
