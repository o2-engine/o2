#include "TextSplitterComponent.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Editor/Attributes/EditorPropertyAttribute.h"
#include "TextSymbolComponent.h"
#include "AnimationComponent.h"

namespace o2
{
    TextSplitterComponent::TextSplitterComponent()
    {}

    TextSplitterComponent::TextSplitterComponent(const TextSplitterComponent& other):
        Component(other), mText(other.mText), mFont(other.mFont), mHeight(other.mHeight), mSymbolsDistCoef(other.mSymbolsDistCoef),
        mLinesDistCoef(other.mLinesDistCoef), mHorAlign(other.mHorAlign), mVerAlign(other.mVerAlign),
		mWordWrap(other.mWordWrap), mDotsEndings(other.mDotsEndings), mColor(other.mColor),
		text(this), font(this), height(this), symbolsDistanceCoef(this), linesDistanceCoef(this), horAlign(this), verAlign(this), 
		wordWrap(this), dotsEndings(this), symbolsAnimationDelay(this), autoPlaySymbolsAnimation(this), color(this), transparency(this)
    {
        if (mFont)
			mFont->GetFont()->onCharactersRebuilt += THIS_FUNC(CheckCharactersAndRebuild);
    }

    TextSplitterComponent::~TextSplitterComponent()
    {
        if (mFont)
            mFont->GetFont()->onCharactersRebuilt -= THIS_FUNC(CheckCharactersAndRebuild);

        ClearSymbols();
    }

    void TextSplitterComponent::SetText(const WString& text)
    {
        if (mText != text)
        {
            mText = text;
            CheckCharactersAndRebuild();
        }
    }

    const WString& TextSplitterComponent::GetText() const
    {
        return mText;
    }

    void TextSplitterComponent::SetFont(const AssetRef<FontAsset>& font)
    {
        if (mFont == font)
			return;

        if (mFont)
			mFont->GetFont()->onCharactersRebuilt -= THIS_FUNC(CheckCharactersAndRebuild);

        mFont = font;
        CheckCharactersAndRebuild();

        if (mFont)
			mFont->GetFont()->onCharactersRebuilt += THIS_FUNC(CheckCharactersAndRebuild);
    }

    const AssetRef<FontAsset>& TextSplitterComponent::GetFont() const
    {
        return mFont;
    }

	void TextSplitterComponent::SetSymbolPrototype(const LinkRef<Actor>& prototype)
	{
		mSymbolPrototype = prototype;
        CheckCharactersAndRebuild();
	}

	const LinkRef<Actor>& TextSplitterComponent::GetSymbolPrototype() const
	{
		return mSymbolPrototype;
	}

	void TextSplitterComponent::SetHeight(int height)
    {
        if (mHeight != height)
        {
            mHeight = height;
            CheckCharactersAndRebuild();
        }
    }

    int TextSplitterComponent::GetHeight() const
    {
        return mHeight;
    }

    void TextSplitterComponent::SetSymbolsDistanceCoef(float coef)
    {
        if (mSymbolsDistCoef != coef)
        {
            mSymbolsDistCoef = coef;
            CheckCharactersAndRebuild();
        }
    }

    float TextSplitterComponent::GetSymbolsDistanceCoef() const
    {
        return mSymbolsDistCoef;
    }

    void TextSplitterComponent::SetLinesDistanceCoef(float coef)
    {
        if (mLinesDistCoef != coef)
        {
            mLinesDistCoef = coef;
            CheckCharactersAndRebuild();
        }
    }

    float TextSplitterComponent::GetLinesDistanceCoef() const
    {
        return mLinesDistCoef;
    }

    void TextSplitterComponent::SetHorAlign(HorAlign align)
    {
        if (mHorAlign != align)
        {
            mHorAlign = align;
            CheckCharactersAndRebuild();
        }
    }

    HorAlign TextSplitterComponent::GetHorAlign() const
    {
        return mHorAlign;
    }

    void TextSplitterComponent::SetVerAlign(VerAlign align)
    {
        if (mVerAlign != align)
        {
            mVerAlign = align;
            CheckCharactersAndRebuild();
        }
    }

    VerAlign TextSplitterComponent::GetVerAlign() const
    {
        return mVerAlign;
    }

    void TextSplitterComponent::SetWordWrap(bool flag)
    {
        if (mWordWrap != flag)
        {
            mWordWrap = flag;
            CheckCharactersAndRebuild();
        }
    }

    bool TextSplitterComponent::GetWordWrap() const
    {
        return mWordWrap;
    }

    void TextSplitterComponent::SetDotsEndings(bool flag)
    {
        if (mDotsEndings != flag)
        {
            mDotsEndings = flag;
            CheckCharactersAndRebuild();
        }
    }

    bool TextSplitterComponent::IsDotsEndings() const
    {
        return mDotsEndings;
    }

	void TextSplitterComponent::RunSymbolsAnimation()
	{
		StopSymbolsAnimation();

		mSymbolsAnimationTime = 0.0f;
        mSymbolAnimationIndex = 0;
		mSymbolsAnimationStarted = true;
	}

	void TextSplitterComponent::SetSymbolsAnimationDelay(float delay)
	{
		mSymbolsAnimationDelay = delay;
	}

	float TextSplitterComponent::GetSymbolsAnimationDelay() const
	{
		return mSymbolsAnimationDelay;
	}

	void TextSplitterComponent::SetAutoPlaySymbolsAnimation(bool autoPlay) 
	{
		mAutoPlaySymbolsAnimation = autoPlay;
	}

	bool TextSplitterComponent::IsAutoPlaySymbolsAnimation() const
	{
		return mAutoPlaySymbolsAnimation;
	}

	void TextSplitterComponent::SetColor(const Color4& color)
	{
		if (mColor != color)
		{
			mColor = color;
			UpdateColor();
		}
	}

	Color4 TextSplitterComponent::GetColor() const
	{
		return mColor;
	}

	void TextSplitterComponent::SetTransparency(float transparency)
	{
		mColor.SetAF(transparency);
    	UpdateColor();
	}

	float TextSplitterComponent::GetTransparency() const
	{
		return mColor.AF();
	}

	String TextSplitterComponent::GetName()
    {
	    return "Text splitter";
    }

	String TextSplitterComponent::GetCategory()
    {
	    return "Render";
    }

	void TextSplitterComponent::RebuildCharacters()
    {
        ClearSymbols();

        if (mText.IsEmpty() || !mFont)
            return;

        // Create text object to get symbol set
		RectF transformRect = GetActor()->transform->GetWorldAxisAlignedRect();
		mLastSize = transformRect.Size();

		Text::SymbolsSet symbolSet;
		symbolSet.Initialize(mFont->GetFont(), mText, mHeight, transformRect.LeftBottom(), transformRect.Size(),
                             mHorAlign, mVerAlign, mWordWrap, mDotsEndings, mSymbolsDistCoef, mLinesDistCoef);

        // Create actors for each symbol
        for (auto& line : symbolSet.mLines)
        {
            for (auto& symbol : line.mSymbols)
            {
                auto symbolActor = mSymbolPrototype ? mSymbolPrototype->CloneAsRef<Actor>() : mmake<Actor>();
				symbolActor->name = String("Symbol: ") + symbol.mCharId;
				symbolActor->SetParent(GetActor());
				symbolActor->transform->worldRect = symbol.mFrame;

				auto symbolComponent = symbolActor->GetComponentInChildren<TextSymbolComponent>();
				if (!symbolComponent)
                    symbolComponent = symbolActor->AddComponent<TextSymbolComponent>();

				symbolComponent->SetTextureAndRect(mFont->GetFont()->GetTexture(), symbol.mTexSrc);
            	symbolComponent->SetColor(mColor);

                if (symbolComponent->GetActor() != symbolActor)
                {
                    symbolComponent->GetActor()->transform->position = Vec2F();
                    symbolComponent->GetActor()->transform->size = symbol.mFrame.Size();
                }
            }
        }

    	GetActor()->UpdateTransform();
    }

    void TextSplitterComponent::ClearSymbols()
    {
        Vector<Ref<Actor>> childrenToRemove = GetActor()->GetChildren().FindAll([](const Ref<Actor>& actor) { 
            return actor->GetComponentInChildren<TextSymbolComponent>() != nullptr; });

        for (auto& child : childrenToRemove)
            child->Destroy();
    }

	void TextSplitterComponent::CheckCharactersAndRebuild()
	{
        if (mFont && !mText.IsEmpty())
		{
			mFont->GetFont()->CheckCharacters(mText, mHeight);
			mFont->GetFont()->CheckCharacters(".", mHeight);
		}

		RebuildCharacters();
	}

	void TextSplitterComponent::StopSymbolsAnimation()
    {
    	auto& children = GetActor()->GetChildren();
    	for (auto& child : children)
    	{
    		if (auto animationCommponent = child->GetComponent<AnimationComponent>())
    		{
    			if (auto animationState = animationCommponent->GetFirstState())
    			{
    				animationState->GetPlayer().GoToBegin();
    				animationState->GetPlayer().Stop();
    				animationCommponent->OnUpdate(0.0f);
    				animationState->autoPlay = false;
    			}
    		}
    	}
    }

	void TextSplitterComponent::UpdateSymbolsAnimation(float dt)
	{
        if (!mSymbolsAnimationStarted)
			return;

		mSymbolsAnimationTime += dt;

        if (mSymbolsAnimationTime < mSymbolsAnimationDelay)
			return;

		mSymbolsAnimationTime = 0.0f;

		auto children = GetActor()->GetChildren();
        if (mSymbolAnimationIndex >= children.Count())
        {
            mSymbolsAnimationStarted = false;
            return;
		}

		auto symbolActor = children[mSymbolAnimationIndex];
        if (auto animationComponent = symbolActor->GetComponent<AnimationComponent>())
            animationComponent->PlayFirstState();

        mSymbolAnimationIndex++;
		if (mSymbolAnimationIndex >= children.Count())
			mSymbolsAnimationStarted = false;
	}

	void TextSplitterComponent::UpdateColor()
	{
		for (auto& child : GetActor()->GetChildren())
		{
			auto symbolComponent = child->GetComponentInChildren<TextSymbolComponent>();
			if (symbolComponent)
				symbolComponent->SetOverrideColor(mColor);
		}
	}

	void TextSplitterComponent::OnStart()
	{
		Component::OnStart();

    	StopSymbolsAnimation();

		if (mAutoPlaySymbolsAnimation)
			RunSymbolsAnimation();
	}

	void TextSplitterComponent::OnDestroy()
    {
        ClearSymbols();
        Component::OnDestroy();
    }

	void TextSplitterComponent::OnEnabled()
	{
        RebuildCharacters();

        if (mFont)
			mFont->GetFont()->onCharactersRebuilt += THIS_FUNC(CheckCharactersAndRebuild);

		Component::OnEnabled();
	}

	void TextSplitterComponent::OnDisabled()
	{
		if (mFont)
			mFont->GetFont()->onCharactersRebuilt -= THIS_FUNC(CheckCharactersAndRebuild);
	}

	void TextSplitterComponent::OnUpdate(float dt)
	{
		UpdateSymbolsAnimation(dt);
	}

	void TextSplitterComponent::OnTransformUpdated()
	{
		Vec2F transformSize = GetActor()->transform->GetWorldAxisAlignedRect().Size();
        if (transformSize == mLastSize)
			return;

        CheckCharactersAndRebuild();
	}

}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::TextSplitterComponent>);
// --- META ---

DECLARE_CLASS(o2::TextSplitterComponent, o2__TextSplitterComponent);
// --- END META ---
