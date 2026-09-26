#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Application/Application.h"
#include "o2/Application/Input.h"
#include "o2/Events/EventSystem.h"
#include "o2/Render/Render.h"
#include "o2/Render/TextureRef.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/Test/AppTestDriver.h"
#include "o2Editor/UI/ScrollView.h"
#include "o2Editor/UIRoot.h"

using namespace o2;
using namespace Editor;

// Вид редактора перерисовывает свой render target каждый кадр, даже когда его никто не трогает:
// слушатели содержимого регистрируются в системе курсора вместе с перерисовкой
namespace
{
    // Слушатель внутри содержимого вида - как ручка перетаскивания ноды
    class ContentProbe : public RefCounterable, public CursorAreaEventsListener
    {
    public:
        RectF rect = RectF(150, 250, 250, 150);
        int presses = 0;

        bool IsUnderPoint(const Vec2F& point) override { return rect.IsInside(point); }
        void OnCursorPressed(const Input::Cursor& cursor) override { presses++; }

        RefCounter* GetRefCounter() const override { return RefCounterable::GetRefCounter(); }
    };

    class IdleProbeView : public ScrollView
    {
    public:
        explicit IdleProbeView(RefCounter* refCounter) : ScrollView(refCounter) {}

        Camera GetCamera() const { return mViewCamera; }

        Ref<ContentProbe> content = mmake<ContentProbe>();

        int redraws = 0;
        int rightPresses = 0;
        int leftPresses = 0;
        int scrolls = 0;

        // Состояние простоя: камера доехала до цели, перерисовывать нечего - ровно так вид стоит
        // в редакторе, пока его не трогают
        void Settle()
        {
            mViewCameraTargetPos = mViewCamera.GetPosition2D();
            mViewCameraTargetScale = mViewCamera.GetScale2D();
            mViewCameraVelocity = Vec2F();
            mNeedRedraw = false;
        }

        const TextureRef& RenderTarget() const { return mRenderTarget; }
        RectF SpriteRect() const { return mRenderTargetSprite->GetRect(); }
        Vec2F CameraSize() const { return mViewCamera.GetSize2D(); }

        void RedrawContent() override
        {
            redraws++;
            ScrollView::RedrawContent();
            content->OnDrawn();
        }

        void OnCursorPressed(const Input::Cursor& cursor) override
        {
            leftPresses++;
            ScrollView::OnCursorPressed(cursor);
        }

        void OnCursorRightMousePressed(const Input::Cursor& cursor) override
        {
            rightPresses++;
            ScrollView::OnCursorRightMousePressed(cursor);
        }

        void OnScrolled(float scroll) override
        {
            scrolls++;
            ScrollView::OnScrolled(scroll);
        }
    };

    // Тестовый Application рисует только сцену: корень редакторского UI прогоняем руками,
    // иначе виджеты не регистрируются в системе курсора
    // layout=true повторяет первые кадры окна, когда раскладка ещё едет; дальше трансформы не трогаются,
    // как у окна, которое не двигают
    void Step(int frames = 1, bool layout = false)
    {
        for (int i = 0; i < frames; i++)
        {
            {
                PushEditorScopeOnStack scope;
                o2Render.Begin();
                o2Render.SetCamera(Camera());
                o2Render.Clear(Color4(30, 31, 34, 255));
                auto root = EditorUIRoot.GetRootWidget();
                root->Update(1.0f/60.0f);
                root->UpdateChildren(1.0f/60.0f);
                if (layout)
                    root->UpdateChildrenTransforms();
                root->Draw();
                o2Render.End();
            }
            AppTestDriver::PumpFrames(1);
        }
    }

    struct IdleViewInputFixture : public ::testing::Test
    {
        Ref<IdleProbeView> view;

        void SetUp() override
        {
            if (!UIRoot::IsSingletonInitialzed())
            {
                PushEditorScopeOnStack scope;
                mmake<UIRoot>();
            }

            PushEditorScopeOnStack scope;
            view = mmake<IdleProbeView>();
            // вид занимает часть экрана, как окно в доке, а не весь корень
            *view->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(800, 600), Vec2F(120, -80));

            auto root = EditorUIRoot.GetRootWidget();
            *root->layout = WidgetLayout::Based(BaseCorner::Center, (Vec2F)o2Application.GetContentSize());
            EditorUIRoot.AddWidget(view);
            view->SetEnabledForcible(false);
            view->SetEnabledForcible(true);

            Step(10, true);
            view->Settle();
            Step(2); // кадры, в которые вид никто не трогает
            view->redraws = 0;
        }

        void TearDown() override
        {
            EditorUIRoot.RemoveWidget(view);
            view = nullptr;
        }

        Vec2F Center() const { return view->layout->GetWorldRect().Center(); }
    };
}

TEST_F(IdleViewInputFixture, IdleViewRepaintsEveryFrame)
{
    Step(5);
    EXPECT_EQ(view->redraws, 5) << "нетронутый вид пропускает перерисовку";
}

TEST_F(IdleViewInputFixture, IdleViewTakesTheLeftButton)
{
    Vec2F at = Center();
    o2Input.OnCursorMoved(at);
    Step(2);

    o2Input.OnCursorPressed(at);
    Step(2);
    o2Input.OnCursorReleased();
    Step();

    EXPECT_GT(view->leftPresses, 0) << "левая кнопка не доходит до простоявшего вида";
}

// Иначе нажатие на ноду достаётся канвасу, и ноду не утащить
TEST_F(IdleViewInputFixture, IdleViewContentTakesTheLeftButton)
{
    Vec2F at = view->LocalToScreenPoint(view->content->rect.Center());
    o2Input.OnCursorMoved(at);
    Step(2);

    o2Input.OnCursorPressed(at);
    Step(2);
    o2Input.OnCursorReleased();
    Step();

    EXPECT_GT(view->content->presses, 0) << "левая кнопка не доходит до содержимого простоявшего вида";
    EXPECT_EQ(view->leftPresses, 0) << "нажатие на содержимое досталось самому виду";
}

TEST_F(IdleViewInputFixture, IdleViewTakesTheWheel)
{
    Vec2F at = Center();
    o2Input.OnCursorMoved(at);
    Step(2);

    float before = view->GetCamera().GetScale2D().x;
    o2Input.OnMouseWheel(-120.0f);
    Step(8);

    EXPECT_GT(view->scrolls, 0) << "колесо не доходит до простоявшего вида";
    EXPECT_GT(Math::Abs(view->GetCamera().GetScale2D().x - before), 0.01f) << "колесо не масштабирует";
}

TEST_F(IdleViewInputFixture, IdleViewPansWithTheRightButton)
{
    Vec2F at = Center();
    o2Input.OnCursorMoved(at);
    Step(2);

    Vec2F before = view->GetCamera().GetPosition2D();

    o2Input.OnAltCursorPressed(at);
    Step();
    for (int i = 1; i <= 10; i++)
    {
        o2Input.OnCursorMoved(at + Vec2F(-14.0f*i, 0)); // с дельтой - её и читает пан
        Step();
    }
    o2Input.OnAltCursorReleased();
    Step(4);

    EXPECT_GT(view->rightPresses, 0) << "правая кнопка не доходит до простоявшего вида";
    EXPECT_GT(Math::Abs(view->GetCamera().GetPosition2D().x - before.x), 10.0f)
        << "правая кнопка не двигает камеру простоявшего вида";
}


// Обновление трансформа само по себе не повод выбрасывать текстуру: пока едет раскладка -
// тянут сплиттер дока, меняют размер окна - это была аллокация render target каждый кадр
TEST_F(IdleViewInputFixture, TransformUpdateKeepsTheRenderTargetWhileSizeHoldsStill)
{
    auto before = view->RenderTarget();
    ASSERT_TRUE(before.Get());

    // тот же размер, но другое место на экране
    *view->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(800, 600), Vec2F(40, 60));
    Step(2, true);

    EXPECT_EQ(view->RenderTarget().Get(), before.Get()) << "текстура пересоздана без смены размера";
    EXPECT_TRUE(view->SpriteRect() == view->layout->GetWorldRect()) << "спрайт остался на старом месте";

    // а смена размера текстуру меняет
    *view->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(640, 480), Vec2F(40, 60));
    Step(2, true);

    EXPECT_NE(view->RenderTarget().Get(), before.Get()) << "текстура не пересоздана под новый размер";
    EXPECT_EQ(view->CameraSize(), Vec2F(640, 480));
}
