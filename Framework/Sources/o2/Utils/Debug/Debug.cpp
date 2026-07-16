#include "o2/stdafx.h"
#include "Debug.h"

#include <cstdarg>
#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/BitmapFontAsset.h"
#include "o2/Assets/Types/VectorFontAsset.h"
#include "o2/Render/FontStyle.h"
#include "o2/Render/Render.h"
#include "o2/Render/Text.h"
#include "o2/Render/VectorFont.h"
#include "o2/Render/VectorFontEffects.h"
#include "o2/Utils/Debug/Log/ConsoleLogStream.h"
#include "o2/Utils/Debug/Log/FileLogStream.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/Editor/EditorScope.h"

#undef DrawText

namespace o2
{
    Debug::Debug(RefCounter* refCounter):
        Singleton<Debug>(refCounter)
    {
        mFileLogStream = mmake<FileLogStream>("", "log.txt");
        mLogStream = mmake<ConsoleLogStream>("");
        mFileLogStream->BindStream(mLogStream);
    }

    Debug::~Debug()
    {}

    void Debug::InitializeFont()
    {
        mFont = mmake<VectorFont>(o2Assets.GetBuiltAssetsPath() + "debugFont.ttf");

        mFontStyle = mmake<FontStyle>();
        mFontStyle->AddEffect<FontStrokeEffect>(1.5f, Color4(0, 0, 0, 150), 100);

        mText = mmake<Text>(mFont);
        mText->SetFontStyle(mFontStyle);
    }

    void Debug::DeinitializeFont()
    {
        mText = nullptr;
        mFontStyle = nullptr;
        mFont = nullptr;
    }

    void Debug::Update(bool isEditor, float dt)
    {
        //PROFILE_SAMPLE_FUNC();

        UpdateDrawables(isEditor ? mEditorDbgDrawables : mDbgDrawables, dt);
    }

    void Debug::UpdateDrawables(Vector<Ref<IDbgDrawable>>& drawables, float dt)
    {
        Vector<Ref<IDbgDrawable>> freeDrawables;
        for (auto& drawable : drawables)
        {
            drawable->delay -= dt;
            if (drawable->delay <= 0.0f)
                freeDrawables.Add(drawable);
        }

        drawables.Remove(freeDrawables);
    }

    Vector<Ref<Debug::IDbgDrawable>>& Debug::GetCurrentScopeDrawables()
    {
        return EditorScope::IsInScope() ? mEditorDbgDrawables : mDbgDrawables;
    }

    void Debug::Draw(bool isEditor)
    {
        //PROFILE_SAMPLE_FUNC();

        for (auto& drw : isEditor ? mEditorDbgDrawables : mDbgDrawables)
            drw->Draw();
    }

    void Debug::Log(WString format, ...)
    {
        va_list vlist;
        va_start(vlist, format);

        mInstance->mLogStream->OutStr(String::Format(format, vlist));

        va_end(vlist);
    }

    void Debug::LogStr(const WString& out)
    {
        mInstance->mLogStream->OutStr(out);
    }

    void Debug::LogWarning(WString format, ...)
    {
        va_list vlist;
        va_start(vlist, format);

        mInstance->mLogStream->WarningStr(String::Format(format, vlist));

        va_end(vlist);
    }

    void Debug::LogWarningStr(const WString& out)
    {
        mInstance->mLogStream->WarningStr(out);
    }

    void Debug::LogError(WString format, ...)
    {
        va_list vlist;
        va_start(vlist, format);

        mInstance->mLogStream->ErrorStr(String::Format(format, vlist));

        va_end(vlist);
    }

    void Debug::LogErrorStr(const WString& out)
    {
        mInstance->mLogStream->ErrorStr(out);
    }

    const Ref<LogStream>& Debug::GetLog()
    {
        return mInstance->mLogStream;
    }

    void Debug::DrawRect(const RectF& rect, const Color4& color, float delay)
    {
        GetCurrentScopeDrawables().Add(mmake<DbgRect>(rect, color, delay));
    }

    void Debug::DrawRect(const RectF& rect, const Color4& color)
    {
        GetCurrentScopeDrawables().Add(mmake<DbgRect>(rect, color, -1.0f));
    }

    void Debug::DrawRect(const RectF& rect, float delay)
    {
        GetCurrentScopeDrawables().Add(mmake<DbgRect>(rect, Color4::White(), delay));
    }

    void Debug::DrawLine(const Vec2F& begin, const Vec2F& end, const Color4& color, float delay)
    {
        GetCurrentScopeDrawables().Add(mmake<DbgLine>(begin, end, color, delay));
    }

    void Debug::DrawLine(const Vec2F& begin, const Vec2F& end, const Color4& color)
    {
        GetCurrentScopeDrawables().Add(mmake<DbgLine>(begin, end, color, -1.0f));
    }

    void Debug::DrawLine(const Vec2F& begin, const Vec2F& end, float delay)
    {
        GetCurrentScopeDrawables().Add(mmake<DbgLine>(begin, end, Color4::White(), delay));
    }

    void Debug::DrawLine(const Vector<Vec2F>& points, const Color4& color, float delay)
    {
        GetCurrentScopeDrawables().Add(mmake<DbgPolyLine>(points, color, delay));
    }

    void Debug::DrawLine(const Vector<Vec2F>& points, const Color4& color)
    {
        GetCurrentScopeDrawables().Add(mmake<DbgPolyLine>(points, color, -1.0f));
    }

    void Debug::DrawLine(const Vector<Vec2F>& points, float delay)
    {
        GetCurrentScopeDrawables().Add(mmake<DbgPolyLine>(points, Color4::White(), delay));
    }

    void Debug::DrawText(const Vec2F& position, const String& text, const Color4& color, float delay)
    {
        GetCurrentScopeDrawables().Add(mmake<DbgText>(position, text, mFont, mFontStyle, color, delay));
    }

    void Debug::DrawText(const Vec2F& position, const String& text, const Color4& color)
    {
        GetCurrentScopeDrawables().Add(mmake<DbgText>(position, text, mText, color));
    }

    void Debug::DrawText(const Vec2F& position, const String& text, float delay)
    {
        GetCurrentScopeDrawables().Add(mmake<DbgText>(position, text, mFont, mFontStyle, Color4::White(), delay));
    }

    void Debug::DrawArrow(const Vec2F& begin, const Vec2F& end, const Color4& color, float delay)
    {
        GetCurrentScopeDrawables().Add(mmake<DbgArrow>(begin, end, color, delay));
    }

    void Debug::DrawArrow(const Vec2F& begin, const Vec2F& end, const Color4& color /*= Color4::White()*/)
    {
        GetCurrentScopeDrawables().Add(mmake<DbgArrow>(begin, end, color, -1.0f));
    }

    void Debug::DrawArrow(const Vec2F& begin, const Vec2F& end, float delay)
    {
        GetCurrentScopeDrawables().Add(mmake<DbgArrow>(begin, end, Color4::White(), delay));
    }

    void Debug::DrawRay(const Vec2F& begin, const Vec2F& dir, const Color4& color, float delay)
    {
        GetCurrentScopeDrawables().Add(mmake<DbgLine>(begin, begin + dir, color, delay));
    }

    void Debug::DrawRay(const Vec2F& begin, const Vec2F& dir, const Color4& color)
    {
        GetCurrentScopeDrawables().Add(mmake<DbgLine>(begin, begin + dir, color, -1.0f));
    }

    void Debug::DrawRay(const Vec2F& begin, const Vec2F& dir, float delay)
    {
        GetCurrentScopeDrawables().Add(mmake<DbgLine>(begin, begin + dir, Color4::White(), delay));
    }

    void Debug::DrawCircle(const Vec2F& origin, float radius, const Color4& color, float delay)
    {
        GetCurrentScopeDrawables().Add(mmake<DbgCircle>(origin, radius, color, delay));
    }

    void Debug::DrawCircle(const Vec2F& origin, float radius, const Color4& color)
    {
        GetCurrentScopeDrawables().Add(mmake<DbgCircle>(origin, radius, color, -1.0f));
    }

    void Debug::DrawCircle(const Vec2F& origin, float radius, float delay)
    {
        GetCurrentScopeDrawables().Add(mmake<DbgCircle>(origin, radius, Color4::White(), delay));
    }

    Debug::IDbgDrawable::IDbgDrawable():
        delay(-1.0f)
    {}

    Debug::IDbgDrawable::IDbgDrawable(const Color4& color, float delay):
        color(color), delay(delay)
    {}

    Debug::IDbgDrawable::~IDbgDrawable()
    {}

    Debug::DbgLine::DbgLine()
    {}

    Debug::DbgLine::DbgLine(const Vec2F& begin, const Vec2F& end, const Color4& color, float delay /*= -1.0f*/):
        begin(begin), end(end), IDbgDrawable(color, delay)
    {}

    void Debug::DbgLine::Draw()
    {
        o2Render.DrawAALine(begin, end, color);
    }

    Debug::DbgCircle::DbgCircle():
        radius(0)
    {}

    Debug::DbgCircle::DbgCircle(const Vec2F& origin, float radius, const Color4& color, float delay /*= -1.0f*/):
        origin(origin), radius(radius), IDbgDrawable(color, delay)
    {}

    void Debug::DbgCircle::Draw()
    {
        o2Render.DrawAACircle(origin, radius, color);
    }

    Debug::DbgRect::DbgRect()
    {}

    Debug::DbgRect::DbgRect(const RectF& rect, const Color4& color, float delay /*= -1.0f*/):
        rect(rect), IDbgDrawable(color, delay)
    {}

    void Debug::DbgRect::Draw()
    {
        o2Render.DrawAARectFrame(rect, color);
    }

    Debug::DbgPolyLine::DbgPolyLine()
    {}

    Debug::DbgPolyLine::DbgPolyLine(const Vector<Vec2F>& points, const Color4& color, float delay /*= -1.0f*/):
        points(points), IDbgDrawable(color, delay)
    {}

    void Debug::DbgPolyLine::Draw()
    {
        o2Render.DrawAALine(points, color);
    }

    Debug::DbgText::DbgText():
        textDrawable(nullptr)
    {}

    Debug::DbgText::DbgText(const Vec2F& position, const String& text, const Ref<Text>& textDrawable, const Color4& color):
        position(position), text(text), textDrawable(textDrawable), IDbgDrawable(color, -1.0f)
    {}

    Debug::DbgText::DbgText(const Vec2F& position, const String& text, const Ref<VectorFont>& font,
                            const Ref<FontStyle>& style, const Color4& color, float delay /*= -1.0f*/):
        position(position), text(text), IDbgDrawable(color, delay)
    {
        textDrawable = mmake<Text>(font);
        textDrawable->SetFontStyle(style);
    }

    Debug::DbgText::~DbgText()
    {}

    void Debug::DbgText::Draw()
    {
        if (textDrawable)
        {
            textDrawable->SetPosition(position);
            textDrawable->SetText(text);
            textDrawable->SetColor(color);
            textDrawable->Draw();
        }
    }

    Debug::DbgArrow::DbgArrow()
    { }

    Debug::DbgArrow::DbgArrow(const Vec2F& begin, const Vec2F& end, const Color4& color, float delay /*= -1.0f*/) :
        begin(begin), end(end), IDbgDrawable(color, delay)
    { }

    void Debug::DbgArrow::Draw()
    {
        o2Render.DrawAAArrow(begin, end, color);
    }

}
