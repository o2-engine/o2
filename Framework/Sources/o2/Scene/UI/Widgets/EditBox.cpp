#include "o2/stdafx.h"
#include "EditBox.h"

#include "o2/Application/Application.h"
#include "o2/Render/Mesh.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/Widgets/HorizontalScrollBar.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/VerticalScrollBar.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/System/Clipboard.h"
#include "o2/Utils/System/Time/Time.h"

namespace o2
{
    EditBox::EditBox(RefCounter* refCounter):
        ScrollArea(refCounter)
    {
        mSelectionMesh = mmake<Mesh>();
        mTextDrawable  = mmake<Text>();
        mCaretDrawable = mmake<Sprite>();
    }

    EditBox::EditBox(RefCounter* refCounter, const EditBox& other):
        ScrollArea(refCounter, other), mMultiLine(other.mMultiLine), mWordWrap(other.mWordWrap), mMaxLineChars(other.mMaxLineChars),
        mMaxLinesCount(other.mMaxLinesCount), mText(other.mText), mLastText(other.mText),
        mAvailableSymbols(other.mAvailableSymbols), mSelectionColor(other.mSelectionColor),
        mCaretBlinkDelay(other.mCaretBlinkDelay)
    {
        mSelectionMesh = mmake<Mesh>();
        mTextDrawable  = other.mTextDrawable->CloneAsRef<Text>();
        mCaretDrawable = other.mCaretDrawable->CloneAsRef<Sprite>();

        mTextDrawable->SetText(mText);

        RetargetStatesAnimations();
        SetLayoutDirty();
    }

    EditBox::~EditBox()
    {}

    EditBox& EditBox::operator=(const EditBox& other)
    {
        ScrollArea::operator=(other);

        mText = other.mText;
        mLastText = mText;
        mAvailableSymbols = other.mAvailableSymbols;
        mSelectionBegin = 0;
        mSelectionEnd = 0;
        mCaretBlinkTime = 0;
        mLastClickTime = -10.0f;
        mMultiLine = other.mMultiLine;
        mWordWrap = other.mWordWrap;
        mMaxLineChars = other.mMaxLineChars;
        mMaxLinesCount = other.mMaxLinesCount;
        mSelectionColor = other.mSelectionColor;
        mCaretBlinkDelay = other.mCaretBlinkDelay;
        mTextDrawable = other.mTextDrawable->CloneAsRef<Text>();
        mCaretDrawable = other.mCaretDrawable->CloneAsRef<Sprite>();

        mTextDrawable->SetText(mText);

        RetargetStatesAnimations();
        SetLayoutDirty();

        onChanged(mText);

        return *this;
    }

    void EditBox::Draw()
    {
        PROFILE_SAMPLE_FUNC();

        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        for (auto& layer : mDrawingLayers)
            layer->Draw();

        ISceneDrawable::OnDrawn();

        o2Render.EnableScissorTest(mAbsoluteClipArea);

        mTextDrawable->Draw();
        mSelectionMesh->Draw();

        if (mIsFocused)
            mCaretDrawable->Draw();

        for (auto& child : mChildrenInheritedDepth)
            child->Draw();

        o2Render.DisableScissorTest();

        CursorAreaEventsListener::OnDrawn();

        for (auto& layer : mTopDrawingLayers)
            layer->Draw();

        if (mOwnHorScrollBar)
            mHorScrollBar->Draw();

        if (mOwnVerScrollBar)
            mVerScrollBar->Draw();

        DrawDebugFrame();
    }

    void EditBox::Update(float dt)
    {
        PROFILE_SAMPLE_FUNC();

        ScrollArea::Update(dt);

        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        UpdateCaretBlinking(dt);

        mJustFocused = false;
    }

    void EditBox::SetText(const WString& text)
    {
        mText = GetFilteredText(text);
        mTextDrawable->SetText(mText);

        UpdateScrollParams();
        UpdateSelectionAndCaret();

        onChanged(mText);

        mLastText = mText;
    }

    WString EditBox::GetText() const
    {
        return mText;
    }

    void EditBox::SetCaretPosition(int caretPosition)
    {
        mSelectionBegin = mSelectionEnd = caretPosition;
        UpdateSelectionAndCaret();
    }

    int EditBox::GetCaretPosition()
    {
        return mSelectionEnd;
    }

    void EditBox::Select(int begin, int end)
    {
        mSelectionBegin = begin;
        mSelectionEnd = end;
        UpdateSelectionAndCaret();
    }

    void EditBox::SetSelectionBegin(int position)
    {
        mSelectionBegin = position;
        UpdateSelectionAndCaret();
    }

    int EditBox::GetSelectionBegin() const
    {
        return mSelectionBegin;
    }

    void EditBox::SetSelectionEnd(int position)
    {
        mSelectionEnd = position;
        UpdateSelectionAndCaret();
    }

    int EditBox::GetSelectionEnd() const
    {
        return mSelectionEnd;
    }

    void EditBox::Deselect()
    {
        mSelectionBegin = mSelectionEnd;
        UpdateSelectionAndCaret();
    }

    void EditBox::SelectAll()
    {
        mSelectionBegin = 0;
        mSelectionEnd = mText.Length();

        UpdateSelectionAndCaret();
        CheckScrollingToCaret();
    }

    const Ref<Text>& EditBox::GetTextDrawable()
    {
        return mTextDrawable;
    }

    const Ref<Sprite>& EditBox::GetCaretDrawable()
    {
        return mCaretDrawable;
    }

    void EditBox::SetSelectionColor(const Color4& color)
    {
        mSelectionColor = color;
        UpdateSelectionAndCaret();
    }

    Color4 EditBox::GetSelectionColor() const
    {
        return mSelectionColor;
    }

    void EditBox::SetFilterInteger()
    {
        SetAvailableSymbols("1234567890-");
    }

    void EditBox::SetFilterFloat()
    {
        SetAvailableSymbols("-1234567890.,");
    }

    void EditBox::SetFilterNames()
    {
        SetAvailableSymbols("1234567890qwertyuioplkjhgfdsazxcvbnm_ QWERTYUIOPLKJHGFDSAZXCVBNM.,");
    }

    void EditBox::SetAvailableSymbols(const WString& availableSymbols)
    {
        mAvailableSymbols = availableSymbols;

        if (mAvailableSymbols.Length() > 0)
        {
            mText = GetFilteredText(mText);
            mTextDrawable->SetText(mText);

            SetLayoutDirty();
        }

        onChanged(mText);
    }

    WString EditBox::GetAvailableSymbols() const
    {
        return mAvailableSymbols;
    }

    void EditBox::SetMaxSizes(int maxLineCharactersCount, int maxLinesCount)
    {
        mMaxLineChars = maxLineCharactersCount;
        mMaxLinesCount = maxLinesCount;

        CheckCharactersAndLinesBounds();
    }

    void EditBox::CheckCharactersAndLinesBounds()
    {
        int lines = 0;
        int lineChars = 0;
        WString filteredText(mText.Capacity() + 1);
        int fi = 0;
        for (int i = 0; i < mText.Length(); i++)
        {
            if (mText[i] == '\n')
            {
                lines++;
                lineChars = 0;
                if (lines == mMaxLinesCount && mMaxLinesCount > 0)
                    break;
            }

            lineChars++;

            if (lineChars < mMaxLineChars || mMaxLineChars == 0 || mText[i] == '\n')
                filteredText[fi++] = mText[i];
        }

        filteredText[fi] = '\0';

        mText = filteredText;
        mTextDrawable->SetText(filteredText);
        onChanged(mText);

        SetLayoutDirty();
    }

    void EditBox::SetMaxLineCharactersCount(int count)
    {
        mMaxLineChars = count;
        CheckCharactersAndLinesBounds();
    }

    int EditBox::GetMaxLineCharactersCount() const
    {
        return mMaxLineChars;
    }

    void EditBox::SetMaxLinesCount(int count)
    {
        mMaxLinesCount = count;
        CheckCharactersAndLinesBounds();
    }

    int EditBox::GetMaxLinesCount() const
    {
        return mMaxLinesCount;
    }

    void EditBox::SetMultiLine(bool multiline)
    {
        mMultiLine = multiline;

        if (!mMultiLine)
        {
            WString filteredText(mText.Capacity() + 1);
            int fi = 0;
            for (int i = 0; i < mText.Length(); i++)
            {
                if (mText[i] != '\n')
                    filteredText[fi++] = mText[i];
            }

            filteredText[fi] = '\0';

            mText = filteredText;
            mLastText = mText;
            mTextDrawable->SetText(filteredText);

            SetLayoutDirty();
        }

        onChanged(mText);
    }

    bool EditBox::IsMultiLine() const
    {
        return mMultiLine;
    }

    void EditBox::SetWordWrap(bool wordWrap)
    {
        mWordWrap = wordWrap;
        mTextDrawable->SetWordWrap(mWordWrap);
    }

    bool EditBox::IsWordWrap() const
    {
        return mWordWrap;
    }

    void EditBox::SetCaretBlinkingDelay(float delay)
    {
        mCaretBlinkDelay = delay;
    }

    float EditBox::GetCaretBlinkingDelay() const
    {
        return mCaretBlinkDelay;
    }

    bool EditBox::IsScrollable() const
    {
        return mEnableHorScroll || mEnableVerScroll;
    }

    bool EditBox::IsFocusable() const
    {
        return true;
    }

    void EditBox::OnEnabled()
    {
        Widget::OnEnabled();

        SetInteractable(true);
    }

    void EditBox::OnDisabled()
    {
        Widget::OnDisabled();

        SetInteractable(false);
    }

    void EditBox::OnFocused()
    {
        if (!mMultiLine)
            SelectAll();

        mJustFocused = true;

        if (IsUnderPoint(o2Input.GetCursorPos()))
            o2Application.SetCursor(CursorType::IBeam);

        Widget::OnFocused();
    }

    void EditBox::OnUnfocused()
    {
        mLastText = mText;
        onChangeCompleted(mText);

        if (!mMultiLine)
            Deselect();
    }

    void EditBox::OnCursorPressed(const Input::Cursor& cursor)
    {
        o2UI.FocusWidget(Ref(this));

        auto pressedState = state["pressed"];
        if (pressedState)
            *pressedState = true;

        if (o2Time.GetApplicationTime() - mLastClickTime < 0.3f || o2Input.IsKeyDown(VK_CONTROL))
        {
            mSelectionBegin = mSelectionEnd = GetTextCaretPosition(cursor.position);
            JumpSelection(false, false);
            JumpSelection(true, true);

            mSelWordBegin = Math::Min(mSelectionBegin, mSelectionEnd);
            mSelWordEnd = Math::Max(mSelectionBegin, mSelectionEnd);

            mSelectingByWords = true;
        }
        else
        {
            if ((mMultiLine || mIsFocused) && !mJustFocused)
                mSelectionBegin = mSelectionEnd = GetTextCaretPosition(cursor.position);

            mSelectingByWords = false;

            UpdateSelectionAndCaret();
            CheckScrollingToCaret();
        }

        mLastClickTime = o2Time.GetApplicationTime();
        mCaretBlinkTime = 0.0f;
        mLastCursorPos = cursor.position;
    }

    void EditBox::OnCursorReleased(const Input::Cursor& cursor)
    {
        auto pressedState = state["pressed"];
        if (pressedState)
            *pressedState = false;

        if (!Widget::IsUnderPoint(cursor.position))
        {
            o2Application.SetCursor(CursorType::Arrow);
            Widget::Unfocus();
        }
    }

    void EditBox::OnCursorPressBreak(const Input::Cursor& cursor)
    {
        auto pressedState = state["pressed"];
        if (pressedState)
            *pressedState = false;

        if (!Widget::IsUnderPoint(cursor.position))
            o2Application.SetCursor(CursorType::Arrow);
    }

    void EditBox::OnCursorReleasedOutside(const Input::Cursor& cursor)
    {
        if (mIsFocused)
            Unfocus();
    }

    void EditBox::OnCursorStillDown(const Input::Cursor& cursor)
    {
        if ((cursor.position - mLastCursorPos).Length() < 3.0f)
            return;

        if (mSelectingByWords)
        {
            int curIdx = GetTextCaretPosition(cursor.position);

            if (curIdx > mSelWordEnd)
            {
                mSelectionBegin = mSelWordBegin;
                mSelectionEnd = curIdx;
                JumpSelection(true, true);
            }
            else if (curIdx < mSelWordBegin)
            {
                mSelectionBegin = mSelWordEnd;
                mSelectionEnd = curIdx;
                JumpSelection(false, true);
            }
            else
            {
                mSelectionBegin = mSelWordBegin;
                mSelectionEnd = mSelWordEnd;

                UpdateSelectionAndCaret();
                CheckScrollingToCaret();
            }
        }
        else
        {
            mSelectionEnd = GetTextCaretPosition(cursor.position);
            UpdateSelectionAndCaret();
            CheckScrollingToCaret();
        }

        mCaretBlinkTime = 0;
        mLastCursorPos = cursor.position;
    }

    void EditBox::OnCursorEnter(const Input::Cursor& cursor)
    {
        auto selectState = state["hover"];
        if (selectState)
            *selectState = true;

        if (!cursor.isPressed)
            o2Application.SetCursor(CursorType::IBeam);
    }

    void EditBox::OnCursorExit(const Input::Cursor& cursor)
    {
        auto selectState = state["hover"];
        if (selectState)
            *selectState = false;

        if (!IsPressed() && !cursor.isPressed)
            o2Application.SetCursor(CursorType::Arrow);
    }

    void EditBox::OnCursorRightMousePressed(const Input::Cursor& cursor)
    {}

    void EditBox::OnCursorRightMouseStayDown(const Input::Cursor& cursor)
    {}

    void EditBox::OnCursorRightMouseReleased(const Input::Cursor& cursor)
    {}

    void EditBox::OnKeyPressed(const Input::Key& key)
    {
        if (!mIsFocused)
            return;

        CheckCharacterTyping(key.keyCode);
        CheckCaretMoving(key.keyCode);
        CheckCharactersErasing(key.keyCode);
        CheckClipboard(key.keyCode);
    }

    void EditBox::OnKeyReleased(const Input::Key& key)
    {
        if (mIsFocused)
        {
            if (key.keyCode == VK_ESCAPE)
            {
                if (!mMultiLine)
                    SetText(mLastText);

                Widget::Unfocus();
            }

            if (!mMultiLine)
            {
                if (key.keyCode == VK_RETURN)
                {
                    mLastText = mText;
                    Widget::Unfocus();
                }
            }
        }
    }

    void EditBox::OnKeyStayDown(const Input::Key& key)
    {
        if (!mIsFocused)
            return;

        if (!o2Input.IsKeyRepeating(key.keyCode))
            return;

        CheckCharacterTyping(key.keyCode);
        CheckCaretMoving(key.keyCode);
        CheckCharactersErasing(key.keyCode);
        CheckClipboard(key.keyCode);
    }

    WString EditBox::GetFilteredText(const WString& text)
    {
        if (mAvailableSymbols.Length() == 0)
            return text;

        WString filteredText;
        filteredText.Reserve(text.Capacity() + 1);
        for (int i = 0; i < text.Length(); i++)
        {
            bool pass = false;
            for (int j = 0; j < mAvailableSymbols.Length(); j++)
            {
                if (mAvailableSymbols[j] == text[i])
                {
                    pass = true;
                    break;
                }
            }

            if (pass)
                filteredText += text[i];
        }

        return filteredText;
    }

    void EditBox::UpdateScrollParams()
    {
        auto fontRef = mTextDrawable->GetFont();
        if (fontRef)
            fontRef->CheckCharacters(" ", mTextDrawable->GetFontHeight());

        mAbsoluteViewArea = mViewAreaLayout.Calculate(layout->worldRect);
        RectF localViewArea(0.0f, 0.0f, mAbsoluteViewArea.Width(), mAbsoluteViewArea.Height());

        mScrollArea = RectF(0.0f, 0.0f, localViewArea.Width(), localViewArea.Height());

        for (auto& child : mChildWidgets)
        {
            mScrollArea.left   = Math::Min(mScrollArea.left, child->layout->GetLeft());
            mScrollArea.bottom = Math::Min(mScrollArea.bottom, child->layout->GetBottom());
            mScrollArea.right  = Math::Max(mScrollArea.right, child->layout->GetRight());
            mScrollArea.top    = Math::Max(mScrollArea.top, child->layout->GetTop());
        }

        RectF localTextLayout = localViewArea;
        Vec2F textRealSize = mTextDrawable->GetRealSize();
        RectF textArea(localTextLayout.left, localTextLayout.top - textRealSize.y,
                       localTextLayout.left + textRealSize.x, localTextLayout.top);

        mScrollArea.left   = Math::Min(mScrollArea.left, textArea.left);
        mScrollArea.bottom = Math::Min(mScrollArea.bottom, textArea.bottom);
        mScrollArea.right  = Math::Max(mScrollArea.right, textArea.right);
        mScrollArea.top    = Math::Max(mScrollArea.top, textArea.top);

        mScrollRange = RectF(mScrollArea.left - localViewArea.left,
                             localViewArea.Height() - mScrollArea.top + localViewArea.bottom,
                             -(localViewArea.Width() - mScrollArea.right + localViewArea.left),
                             -mScrollArea.bottom + localViewArea.bottom);

        if (mHorScrollBar)
        {
            if (Math::Equals(mScrollRange.left, mScrollRange.right, 1.0f))
            {
                if (mEnableHorScroll)
                {
                    auto enableHorBarState = state["enableHorBar"];
                    if (enableHorBarState)
                        *enableHorBarState = false;

                    mHorScrollBar->Hide();
                }

                mEnableHorScroll = false;
            }
            else
            {
                if (!mEnableHorScroll)
                {
                    auto enableHorBarState = state["enableHorBar"];
                    if (enableHorBarState)
                        *enableHorBarState = true;

                    mHorScrollBar->Show();
                }

                mEnableHorScroll = true;

                mHorScrollBar->SetValueRange(mScrollRange.left, mScrollRange.right);
                mHorScrollBar->SetScrollHandleSize(localViewArea.Width());
            }
        }

        if (mVerScrollBar)
        {
            if (Math::Equals(mScrollRange.bottom, mScrollRange.top, 1.0f))
            {
                if (mEnableVerScroll)
                {
                    auto enableVerBarState = state["enableVerBar"];
                    if (enableVerBarState)
                        *enableVerBarState = false;

                    mVerScrollBar->Hide();
                }

                mEnableVerScroll = false;
            }
            else
            {
                if (!mEnableVerScroll)
                {
                    auto enableVerBarState = state["enableVerBar"];
                    if (enableVerBarState)
                        *enableVerBarState = true;

                    mVerScrollBar->Show();
                }

                mEnableVerScroll = true;

                mVerScrollBar->SetValueRange(mScrollRange.bottom, mScrollRange.top);
                mVerScrollBar->SetScrollHandleSize(localViewArea.Height());
            }
        }
    }

    bool EditBox::IsUnderPoint(const Vec2F& point)
    {
        return mDrawingScissorRect.IsInside(point) && mAbsoluteViewArea.IsInside(point);
    }

    bool EditBox::IsInputTransparent() const
    {
        return false;
    }

    String EditBox::GetCreateMenuGroup()
    {
        return "Basic";
    }

    void EditBox::UpdateTransparency()
    {
        ScrollArea::UpdateTransparency();

        mTextDrawable->transparency = mResTransparency;

        Color4 sc = mSelectionColor;
        sc.a = (int)(((float)sc.a)*mResTransparency);
        ULong selectionClr = sc.ABGR();

        Vertex* verts = mSelectionMesh->GetVertices<Vertex>();
        for (UInt i = 0; i < mSelectionMesh->vertexCount; i++)
            verts[i].color = selectionClr;
    }

    void EditBox::UpdateLayersLayouts()
    {
        ScrollArea::UpdateLayersLayouts();

        mTextDrawable->SetRect(GetChildrenWorldRect());
        UpdateSelectionAndCaret();
    }

    void EditBox::UpdateSelectionAndCaret()
    {
        Vec2F caretPosition = GetTextCaretPosition(mSelectionEnd);
        mCaretDrawable->SetPosition(caretPosition);

        mSelectionMesh->vertexCount = 0;
        mSelectionMesh->polyCount = 0;

        if (mSelectionBegin == mSelectionEnd)
            return;

        int beg = Math::Min(mSelectionBegin, mSelectionEnd);
        int end = Math::Max(mSelectionBegin, mSelectionEnd);
        float caretDown = -mCaretDrawable->GetSize().y*mCaretDrawable->GetPivot().y;
        float caretUp = caretDown + mCaretDrawable->GetSize().y;

        auto& symbolsSet = mTextDrawable->GetSymbolsSet();
        auto font = mTextDrawable->GetFont();
        float spaceAdvance = font->GetCharacter(' ', mTextDrawable->GetFontHeight()).mAdvance;

        for (auto& line : symbolsSet.mLines)
        {
            if (beg > line.mLineBegSymbol + line.mSymbols.Count() || end < line.mLineBegSymbol)
                continue;

            if (line.mSymbols.Count() == 0)
            {
                Vec2F lb = line.mPosition + Vec2F(0, caretDown);
                Vec2F rt = lb + Vec2F(spaceAdvance, caretUp);
                AddSelectionRect(RectF(lb, rt));
                continue;
            }

            int begSymbol = Math::Max(beg - line.mLineBegSymbol, 0);
            int endSymbol = end - line.mLineBegSymbol;
            float endOffs = 0, begOffs = 0;
            int lineSymbolsCount = line.mSymbols.Count();

            if (begSymbol >= lineSymbolsCount)
            {
                begOffs = line.mSymbols.Last().mAdvance;
                begSymbol = line.mSymbols.Count() - 1;
            }

            if (endSymbol >= lineSymbolsCount)
            {
                endOffs = line.mSymbols.Last().mAdvance;
                if (line.mEndedNewLine && endSymbol > lineSymbolsCount)
                    endOffs += spaceAdvance;

                endSymbol = lineSymbolsCount - 1;
            }

            Vec2F lb(line.mSymbols[begSymbol].mFrame.left + line.mSymbols[begSymbol].mOrigin.x + begOffs,
                     line.mPosition.y + caretDown);
            Vec2F rt(line.mSymbols[endSymbol].mFrame.left + line.mSymbols[endSymbol].mOrigin.x + endOffs,
                     line.mPosition.y + caretUp);

            AddSelectionRect(RectF(lb, rt));
        }
    }

    Vec2F EditBox::GetTextCaretPosition(int position)
    {
        bool fakeSymbols = false;
        if (mText.Length() == 0)
        {
            fakeSymbols = true;

            if (mTextDrawable->GetText() != "A")
                mTextDrawable->SetText("A");
        }

        auto& symbolsSet = mTextDrawable->GetSymbolsSet();
        for (auto& line : symbolsSet.mLines)
        {
            if (position >= line.mLineBegSymbol && position <= line.mLineBegSymbol + line.mSymbols.Count())
            {
                int off = position - line.mLineBegSymbol;

                if (off < line.mSymbols.Count())
                {
                    auto symb = line.mSymbols[off];
                    auto res = symb.mFrame.LeftBottom() + symb.mOrigin;
                    if (fakeSymbols)
                        mTextDrawable->SetText("");
                    return res;
                }
                else
                {
                    if (line.mSymbols.Count() == 0)
                        return line.mPosition;

                    auto symb = line.mSymbols.Last();
                    auto res = symb.mFrame.LeftBottom() + symb.mOrigin + Vec2F(symb.mAdvance, 0);
                    if (fakeSymbols)
                        mTextDrawable->SetText("");
                    return res;
                }
            }
        }

        if (auto fnt = mTextDrawable->GetFont())
        {
            if (fakeSymbols)
                mTextDrawable->SetText("");

            return mAbsoluteViewArea.LeftTop() - Vec2F(0, mTextDrawable->GetFont()->GetHeightPx(mTextDrawable->GetFontHeight()));
        }

        return Vec2F();
    }

    int EditBox::GetTextCaretPosition(const Vec2F& point)
    {
        auto& symbolsSet = mTextDrawable->GetSymbolsSet();
        auto font = mTextDrawable->GetFont();
        float lineHeight = font->GetLineHeightPx(mTextDrawable->GetFontHeight());
        float lineOffCoef = 0.25f;

        bool checkUp, checkDown, checkLeft, checkRight;
        int lineIdx = 0;
        for (auto& line : symbolsSet.mLines)
        {
            checkUp = lineIdx > 0;
            checkDown = lineIdx < (int)symbolsSet.mLines.Count() - 1;

            float lineTop = (line.mSize.y*(1.0f - lineOffCoef) + lineHeight)*0.5f + line.mPosition.y;
            float lineBottom = -line.mSize.y*lineOffCoef + line.mPosition.y;

            if (line.mSymbols.Count() == 0 && point.y > lineBottom && point.y < lineTop)
                return line.mLineBegSymbol;


            int idx = 0;
            for (auto& symb : line.mSymbols)
            {
                checkLeft = idx > 0;
                checkRight = idx < (int)line.mSymbols.Count() - 1;

                RectF sf(symb.mFrame.left, lineTop, symb.mFrame.right, lineBottom);

                bool ls = checkLeft ? sf.left < point.x : true;
                bool rs = checkRight ? sf.right > point.x : true;
                bool ts = checkUp ? sf.top > point.y:true;
                bool bs = checkDown ? sf.bottom < point.y : true;

                if (ls && rs && ts && bs)
                {
                    if (point.x >(symb.mFrame.left + symb.mFrame.right)*0.5f)
                        return line.mLineBegSymbol + idx + 1;
                    else
                        return line.mLineBegSymbol + idx;
                }

                idx++;
            }

            lineIdx++;
        }

        return 0;
    }

    void EditBox::UpdateCaretBlinking(float dt)
    {
        mCaretBlinkTime += dt;

        float blinkAlpha = Math::Clamp01(1.0f - (mCaretBlinkTime - mCaretBlinkDelay*0.3f) / (mCaretBlinkDelay*0.3f));
        mCaretDrawable->SetTransparency(blinkAlpha*mResTransparency);

        if (mCaretBlinkTime > mCaretBlinkDelay)
            mCaretBlinkTime -= mCaretBlinkDelay;
    }

    void EditBox::AddSelectionRect(const RectF& rect)
    {
        if (mSelectionMesh->GetMaxPolyCount() < mSelectionMesh->polyCount + 6)
        {
            int newPolyCount = mSelectionMesh->polyCount + 6;
            mSelectionMesh->Resize(newPolyCount * 2, newPolyCount);
        }

        unsigned long color = mSelectionColor.ABGR();

        Vertex* selVerts = mSelectionMesh->GetVertices<Vertex>();
        selVerts[mSelectionMesh->vertexCount++] = Vertex(rect.LeftBottom(), color, 0.0f, 0.0f);
        selVerts[mSelectionMesh->vertexCount++] = Vertex(rect.LeftTop(), color, 0.0f, 0.0f);
        selVerts[mSelectionMesh->vertexCount++] = Vertex(rect.RightTop(), color, 0.0f, 0.0f);
        selVerts[mSelectionMesh->vertexCount++] = Vertex(rect.RightBottom(), color, 0.0f, 0.0f);

        VertexIndex* idx = mSelectionMesh->GetIndexes();
        idx[mSelectionMesh->polyCount * 3] = mSelectionMesh->vertexCount - 4;
        idx[mSelectionMesh->polyCount * 3 + 1] = mSelectionMesh->vertexCount - 3;
        idx[mSelectionMesh->polyCount * 3 + 2] = mSelectionMesh->vertexCount - 2;
        mSelectionMesh->polyCount++;

        idx[mSelectionMesh->polyCount * 3] = mSelectionMesh->vertexCount - 4;
        idx[mSelectionMesh->polyCount * 3 + 1] = mSelectionMesh->vertexCount - 2;
        idx[mSelectionMesh->polyCount * 3 + 2] = mSelectionMesh->vertexCount - 1;
        mSelectionMesh->polyCount++;
    }

    void EditBox::CheckScrollingToCaret()
    {
        auto font = mTextDrawable->GetFont();
        if (!font)
            return;

        UpdateScrollParams();
        UpdateSelectionAndCaret();

        Vec2F caretPos = mCaretDrawable->GetPosition2D();
        RectF clipRect = mAbsoluteViewArea;

        float rightOffs = Math::Max(caretPos.x - clipRect.right + 5.0f, 0.0f);
        float leftOffs = Math::Max(clipRect.left - caretPos.x + 5.0f, 0.0f);

        float downOffs = Math::Max(caretPos.y - clipRect.top + font->GetHeightPx(mTextDrawable->GetFontHeight()), 0.0f);
        float topOffs = Math::Max(clipRect.bottom - caretPos.y, 0.0f);

        float horOffs = rightOffs - leftOffs;

//         o2Debug.Log(String("caretPos: ") + (String)caretPos + ", clipRect: " + (String)mAbsoluteViewArea + 
//                     ", horOffs: " + (String)horOffs + ", scrollPos: " + (String)mScrollPos);

        if (!Math::Equals(horOffs, 0.0f))
        {
            if (mHorScrollBar)
                mHorScrollBar->SetValue(mScrollPos.x + horOffs);
            else
            {
                mScrollPos.x = Math::Clamp(mScrollPos.x + horOffs, mScrollRange.left, mScrollRange.right);
                SetLayoutDirty();
            }
        }

        if (mMultiLine)
        {
            float verOffs = topOffs - downOffs;

            if (!Math::Equals(verOffs, 0.0f))
            {
                if (mVerScrollBar)
                    mVerScrollBar->SetValue(mScrollPos.y + verOffs);
                else
                {
                    mScrollPos.y = Math::Clamp(mScrollPos.y + verOffs, mScrollRange.bottom, mScrollRange.top);
                    SetLayoutDirty();
                }
            }
        }
    }

    void EditBox::JumpSelection(bool forward, bool selecting)
    {
        static char jumpSymbols[] = " \n()-=_+\\|/**&^%$#@!~,.?";
        int jumpSymbolsCount = (int)strlen(jumpSymbols);

        int jumpIdx = mSelectionEnd + 1;

        if (forward)
        {
            bool stop = false;
            for (int i = mSelectionEnd; i < mText.Length() && !stop; i++)
            {
                jumpIdx = i;
                for (int j = 0; j < jumpSymbolsCount; j++)
                {
                    if (mText[i] == jumpSymbols[j])
                    {
                        stop = true;
                        break;
                    }
                }
            }

            if (!stop) jumpIdx++;
        }
        else
        {
            bool stop = false;
            for (int i = Math::Min(mSelectionEnd - 1, mText.Length() - 1); i >= 0 && !stop; i--)
            {
                jumpIdx = i;
                for (int j = 0; j < jumpSymbolsCount; j++)
                {
                    if (mText[i] == jumpSymbols[j])
                    {
                        jumpIdx++;
                        stop = true;
                        break;
                    }
                }
            }

            if (!stop) jumpIdx--;
        }

        if (jumpIdx == mSelectionEnd)
        {
            if (forward)
                jumpIdx++;
            else
                jumpIdx--;
        }

        mSelectionEnd = Math::Clamp(jumpIdx, 0, mText.Length());
        if (!selecting)
            mSelectionBegin = mSelectionEnd;

        mCaretBlinkTime = 0;

        UpdateSelectionAndCaret();
        CheckScrollingToCaret();
    }

    UInt16 GetUnicodeFromVirtualCode(KeyboardKey code)
    {
#ifdef PLATFORM_WINDOWS
        HKL layout = GetKeyboardLayout(0);

        BYTE allKeys[256];
        GetKeyboardState(allKeys);

        UInt16 unicode;
        ToUnicodeEx(code, 0, allKeys, reinterpret_cast<wchar_t*>(&unicode), 1, 0, layout);

        return unicode;
        
#elif defined PLATFORM_MAC
        if (code < 0)
            return 0;
        
        return (UInt16)code;

#elif defined PLATFORM_LINUX
        if (code < 0)
            return 0;

        return (UInt16)code;
#endif
        return 0;
    }

    void EditBox::CheckCharacterTyping(KeyboardKey key)
    {
        if (o2Input.IsKeyDown(VK_CONTROL))
            return;

        if (key == VK_ESCAPE)
            return;

        if (!mMultiLine && key == VK_RETURN)
            return;

        char16_t unicode = (char16_t)GetUnicodeFromVirtualCode(key);

        if (unicode != 0 && unicode != 8)
        {
            const char16_t returnChar = 13;
            const char16_t newlineChar = 10;
            const char16_t tabChar = 9;
            
            if (unicode == returnChar) 
                unicode = newlineChar;

            if (unicode == newlineChar && !mMultiLine)
                return;

            if (unicode == tabChar)
                return;

            if (mAvailableSymbols.Length() > 0)
            {
                bool found = false;
                for (int i = 0; i < mAvailableSymbols.Length(); i++)
                {
                    if (mAvailableSymbols[i] == unicode)
                    {
                        found = true;
                        break;
                    }
                }

                if (!found)
                    return;
            }

            if (mSelectionBegin != mSelectionEnd)
            {
                mText.Erase(Math::Min(mSelectionBegin, mSelectionEnd), Math::Max(mSelectionBegin, mSelectionEnd));
                mSelectionEnd = Math::Min(mSelectionBegin, mSelectionEnd);
                mSelectionBegin = mSelectionEnd;
            }

            mText.Insert(unicode, mSelectionEnd);
            mSelectionBegin = mSelectionEnd = mSelectionEnd + 1;
            mTextDrawable->SetText(mText);

            mCaretBlinkTime = 0;

            SetLayoutDirty();
            CheckScrollingToCaret();

            onChanged(mText);
        }
    }

    void EditBox::CheckCharactersErasing(KeyboardKey key)
    {
        if (key == VK_BACK)
        {
            if (mSelectionEnd == mSelectionBegin)
            {
                if (mSelectionEnd > 0)
                    mText.Erase(mSelectionEnd - 1, mSelectionEnd);
            }
            else
            {
                mText.Erase(Math::Min(mSelectionBegin, mSelectionEnd), Math::Max(mSelectionEnd, mSelectionBegin));
            }

            mTextDrawable->SetText(mText);
            onChanged(mText);
            SetLayoutDirty();

            if (mSelectionEnd == mSelectionBegin)
                MoveCaret(mSelectionEnd - 1, false);
            else
                MoveCaret(Math::Min(mSelectionEnd, mSelectionBegin), false);
        }

        if (key == VK_DELETE)
        {
            if (mSelectionEnd < mText.Length() + 1)
            {
                if (mSelectionEnd == mSelectionBegin)
                    mText.Erase(mSelectionEnd, mSelectionBegin + 1);
                else
                    mText.Erase(Math::Min(mSelectionBegin, mSelectionEnd), Math::Max(mSelectionEnd, mSelectionBegin));

                mTextDrawable->SetText(mText);
                onChanged(mText);
                SetLayoutDirty();

                MoveCaret(Math::Min(mSelectionEnd, mSelectionBegin), false);
            }
        }
    }

    void EditBox::CheckCaretMoving(KeyboardKey key)
    {
        bool selecting = o2Input.IsKeyDown(VK_SHIFT);
        bool control = o2Input.IsKeyDown(VK_CONTROL);

        if (key == VK_LEFT)
        {
            if (control) JumpSelection(false, selecting);
            else MoveCaret(mSelectionEnd - 1, selecting);
        }

        if (key == VK_RIGHT)
        {
            if (control)
                JumpSelection(true, selecting);
            else
                MoveCaret(mSelectionEnd + 1, selecting);
        }

        if (key == 'A' && control)
            SelectAll();

        if (key == VK_UP)
            MoveCaret(GetTextCaretPosition(GetTextCaretPosition(mSelectionEnd) +
                      Vec2F(0.0f, mTextDrawable->GetFont()->GetLineHeightPx(mTextDrawable->GetFontHeight())*1.5f)), selecting);

        if (key == VK_DOWN)
            MoveCaret(GetTextCaretPosition(GetTextCaretPosition(mSelectionEnd) -
                      Vec2F(0.0f, mTextDrawable->GetFont()->GetLineHeightPx(mTextDrawable->GetFontHeight())*0.5f)), selecting);

        if (key == VK_END)
        {
            int endLineSymbol = mSelectionEnd;
            for (int i = mSelectionEnd; i < mText.Length(); i++)
            {
                endLineSymbol = i + 1;
                if (mText[i] == '\n')
                {
                    endLineSymbol--;
                    break;
                }
            }

            MoveCaret(endLineSymbol, selecting);
        }

        if (key == VK_HOME)
        {
            int endLineSymbol = mSelectionEnd;
            for (int i = mSelectionEnd - 1; i >= 0; i--)
            {
                endLineSymbol = i;
                if (mText[i] == '\n')
                {
                    endLineSymbol++;
                    break;
                }
            }

            MoveCaret(endLineSymbol, selecting);
        }
    }

    void EditBox::CheckClipboard(KeyboardKey key)
    {
        if (!o2Input.IsKeyDown(VK_CONTROL))
            return;

        if (key == 'C')
            Clipboard::SetText(mText.SubStr(Math::Min(mSelectionBegin, mSelectionEnd),
                               Math::Max(mSelectionBegin, mSelectionEnd)));

        if (key == 'X')
        {
            Clipboard::SetText(mText.SubStr(Math::Min(mSelectionBegin, mSelectionEnd),
                               Math::Max(mSelectionBegin, mSelectionEnd)));

            if (mSelectionBegin != mSelectionEnd)
            {
                mText.Erase(Math::Min(mSelectionBegin, mSelectionEnd), Math::Max(mSelectionBegin, mSelectionEnd));
                mSelectionBegin = mSelectionEnd = Math::Min(mSelectionBegin, mSelectionEnd);
            }

            mTextDrawable->SetText(mText);
            onChanged(mText);
            SetLayoutDirty();
            CheckScrollingToCaret();
        }

        if (key == 'V')
        {
            WString clipboard = Clipboard::GetText();

            if (mSelectionBegin != mSelectionEnd)
                mText.Erase(Math::Min(mSelectionBegin, mSelectionEnd), Math::Max(mSelectionBegin, mSelectionEnd));

            mSelectionEnd = Math::Min(mSelectionBegin, mSelectionEnd);
            mSelectionBegin = mSelectionEnd;

            mText.Insert(clipboard, mSelectionEnd);

            mSelectionBegin = mSelectionEnd = mSelectionEnd + clipboard.Length();

            mTextDrawable->SetText(mText);
            onChanged(mText);
            SetLayoutDirty();
            CheckScrollingToCaret();
        }
    }

    void EditBox::MoveCaret(int newPosition, bool selecting)
    {
        mSelectionEnd = Math::Clamp(newPosition, 0, mText.Length());
        if (!selecting)
            mSelectionBegin = mSelectionEnd;

        UpdateSelectionAndCaret();
        CheckScrollingToCaret();
        mCaretBlinkTime = 0;
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::EditBox>);
// --- META ---

DECLARE_CLASS(o2::EditBox, o2__EditBox);
// --- END META ---
