#pragma once

#include "o2/Utils/Singleton.h"
#include "o2/Utils/Types/String.h"

#undef DrawText

// Debug stuff access macros
#define o2Debug o2::Debug::Instance()

namespace o2
{
    class LogStream;
    class VectorFont;
    class FontStyle;
    class Text;

    // ---------------
    // Debugging stuff
    // ---------------
    class Debug: public Singleton<Debug>
    {
    public:
        // Default constructor
        Debug(RefCounter* refCounter);

        // Destructor
        ~Debug();

        // Out message into main log with formatting
        void Log(WString format, ...);

        // Out message string into main log
        void LogStr(const WString& out);

        // Out warning message into main log with formatting
        void LogWarning(WString format, ...);

        // Out warning message string into main log
        void LogWarningStr(const WString& out);

        // Out error message into main log with formatting
        void LogError(WString format, ...);

        // Out error message string into main log
        void LogErrorStr(const WString& out);

        // Returns pointer to main log
        const Ref<LogStream>& GetLog();

        // Draws debug line from begin to end with color and disappearing delay
        void DrawLine(const Vec2F& begin, const Vec2F& end, const Color4& color, float delay);

        // Draws debug line from begin to end with color at one frame
        void DrawLine(const Vec2F& begin, const Vec2F& end, const Color4& color = Color4::White());

        // Draws white debug line from begin to end with disappearing delay
        void DrawLine(const Vec2F& begin, const Vec2F& end, float delay);

        // Draws debug arrow from begin to end with color and disappearing delay
        void DrawArrow(const Vec2F& begin, const Vec2F& end, const Color4& color, float delay);

        // Draws debug arrow from begin to end with color at one frame
        void DrawArrow(const Vec2F& begin, const Vec2F& end, const Color4& color = Color4::White());

        // Draws white debug arrow from begin to end with disappearing delay
        void DrawArrow(const Vec2F& begin, const Vec2F& end, float delay);

        // Draws debug ray from begin to dir with color and disappearing delay
        void DrawRay(const Vec2F& begin, const Vec2F& dir, const Color4& color, float delay);

        // Draws debug ray from begin to dir with color at one frame
        void DrawRay(const Vec2F& begin, const Vec2F& dir, const Color4& color = Color4::White());

        // Draws white debug ray from begin to dir with disappearing delay
        void DrawRay(const Vec2F& begin, const Vec2F& dir, float delay);

        // Draws debug circle at origin with radius, color and disappearing delay
        void DrawCircle(const Vec2F& origin, float radius, const Color4& color, float delay);

        // Draws debug circle at origin with radius and color
        void DrawCircle(const Vec2F& origin, float radius, const Color4& color = Color4::White());

        // Draws white debug circle at origin with radius and disappearing delay
        void DrawCircle(const Vec2F& origin, float radius, float delay);

        // Draws debug rectangle with color and disappearing delay
        void DrawRect(const RectF& rect, const Color4& color, float delay);

        // Draws debug rectangle with color
        void DrawRect(const RectF& rect, const Color4& color = Color4::White());

        // Draws white debug rectangle with disappearing delay
        void DrawRect(const RectF& rect, float delay);

        // Draws debug line with points with color and disappearing delay
        void DrawLine(const Vector<Vec2F>& points, const Color4& color, float delay);

        // Draws debug line with points with color
        void DrawLine(const Vector<Vec2F>& points, const Color4& color = Color4::White());

        // Draws white debug line with points with disappearing delay
        void DrawLine(const Vector<Vec2F>& points, float delay);

        // Draws debug text with color and disappearing delay
        void DrawText(const Vec2F& position, const String& text, const Color4& color, float delay);

        // Draws debug text with color
        void DrawText(const Vec2F& position, const String& text, const Color4& color = Color4::White());

        // Draws white debug text with disappearing delay
        void DrawText(const Vec2F& position, const String& text, float delay);

        // Updates drawables delay
        void Update(bool isEditor, float dt);

        // Draws debug drawables
        void Draw(bool isEditor);

    protected:
        // ------------------------------------------------------
        // Debug drawable interface: color and disappearing delay
        // ------------------------------------------------------
        struct IDbgDrawable: public RefCounterable
        {
            Color4 color;
            float  delay;

        public:
            IDbgDrawable();
            IDbgDrawable(const Color4& color, float delay);
            virtual ~IDbgDrawable();

            virtual void Draw() = 0;
        };

        // --------------------------------------------
        // Debug line with color and disappearing delay
        // --------------------------------------------
        struct DbgLine : public IDbgDrawable
        {
            Vec2F begin;
            Vec2F end;

        public:
            DbgLine();
            DbgLine(const Vec2F& begin, const Vec2F& end, const Color4& color, float delay = -1.0f);
            void Draw();
        };

        // --------------------------------------------
        // Debug arrow with color and disappearing delay
        // --------------------------------------------
        struct DbgArrow : public IDbgDrawable
        {
            Vec2F begin;
            Vec2F end;

        public:
            DbgArrow();
            DbgArrow(const Vec2F& begin, const Vec2F& end, const Color4& color, float delay = -1.0f);
            void Draw();
        };

        // ----------------------------------------------
        // Debug circle with color and disappearing delay
        // ----------------------------------------------
        struct DbgCircle: public IDbgDrawable
        {
            Vec2F origin;
            float radius;

        public:
            DbgCircle();
            DbgCircle(const Vec2F& origin, float radius, const Color4& color, float delay = -1.0f);
            void Draw();
        };

        // -------------------------------------------------
        // Debug rectangle with color and disappearing delay
        // -------------------------------------------------
        struct DbgRect: public IDbgDrawable
        {
            RectF rect;

        public:
            DbgRect();
            DbgRect(const RectF& rect, const Color4& color, float delay = -1.0f);
            void Draw();
        };

        // -------------------------------------------------
        // Debug poly line with color and disappearing delay
        // -------------------------------------------------
        struct DbgPolyLine: public IDbgDrawable
        {
            Vector<Vec2F> points;

        public:
            DbgPolyLine();
            DbgPolyLine(const Vector<Vec2F>& points, const Color4& color, float delay = -1.0f);
            void Draw();
        };

        // --------------------------------------------
        // Debug text with color and disappearing delay
        // --------------------------------------------
        struct DbgText: public IDbgDrawable
        {
            Vec2F     position;
            String    text;
            Ref<Text> textDrawable;

        public:
            DbgText();
            DbgText(const Vec2F& position, const String& text, const Ref<Text>& textDrawable, const Color4& color);
            DbgText(const Vec2F& position, const String& text, const Ref<VectorFont>& font, const Ref<FontStyle>& style,
                    const Color4& color, float delay = -1.0f);
            ~DbgText();
            void Draw();
        };

    protected:
        Ref<LogStream> mFileLogStream; // File log stream
        Ref<LogStream> mLogStream;     // Main log stream

        Vector<Ref<IDbgDrawable>> mDbgDrawables;       // Debug drawables array
        Vector<Ref<IDbgDrawable>> mEditorDbgDrawables; // Debug drawables array for editor

        Ref<VectorFont> mFont;      // Font for debug captions
        Ref<FontStyle>  mFontStyle; // Font style for debug captions
        Ref<Text>       mText;      // Text for one frame debug captions

    private:
        // Protect copying
        Debug operator=(const Debug& other);

        // Initializes font and text
        void InitializeFont();

        // Deinitializes debug system
        void DeinitializeFont();

        // Updates drawables list
        void UpdateDrawables(Vector<Ref<IDbgDrawable>>& drawables, float dt);

        // Returns regular or editor drawables by current context
        Vector<Ref<IDbgDrawable>>& GetCurrentScopeDrawables();

        friend class Application;
        friend class BaseApplication;
        friend class Integration;
        friend class Singleton<Debug>;
    };
}
