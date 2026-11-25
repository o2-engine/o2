#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>
#include <chrono>
#include <thread>

#include "EditorVectorUIFixture.h"
#include "o2/Assets/Assets.h"
#include "o2/Render/Text/Text.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/DropDown.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/Test/AppTestDriver.h"
#include "o2Editor/Properties/Basic/AssetProperty.h"
#include "o2Editor/Properties/Basic/BooleanProperty.h"
#include "o2Editor/Properties/Basic/EnumIconsProperty.h"
#include "o2Editor/Properties/Objects/TextPropertiesBlock.h"
#include "o2Editor/UI/SegmentedIcons.h"
#include "o2Editor/Windows/PropertiesWindow/PropertiesWindow.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"

using namespace o2;
using namespace Editor;
using namespace EditorVectorUITest;

// The text settings of the Properties driven by the mouse as the platform sends it, one draw per frame
namespace
{
    struct Mouse
    {
        EditorShell& shell;

        void Step(int frames = 1)
        {
            for (int i = 0; i < frames; i++)
            {
                shell.Update(1.0f/60.0f);
                {
                    PushEditorScopeOnStack scope;
                    o2Render.Begin();
                    o2Render.SetCamera(Camera());
                    shell.Draw();
                    o2UI.DrawCurrentLayerTopWidgets();
                    o2Render.End();
                }
                AppTestDriver::PumpFrames(1);
            }
        }

        void Move(const Vec2F& point, int steps = 4)
        {
            Vec2F from = o2Input.GetCursorPos();
            for (int i = 1; i <= steps; i++)
            {
                o2Input.OnCursorMoved(Math::Lerp(from, point, (float)i/(float)steps), 0);
                Step(1);
            }
        }

        void Click(const Vec2F& point)
        {
            Move(point);
            o2Input.OnCursorPressed(point);
            Step(2);
            o2Input.OnCursorReleased();
            Step(4);
        }

        // Waits out the double click time: a second quick click on the same button is a double click
        void Pause()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(600));
            Step(1);
        }

        void Click(const Ref<Widget>& widget)
        {
            ASSERT_TRUE(widget);
            Click(widget->layout->GetWorldRect().Center());
        }
    };

    template<typename _type>
    Vector<Ref<_type>> FindAll(const Ref<Widget>& widget)
    {
        Vector<Ref<_type>> res;
        if (auto typed = DynamicCast<_type>(widget))
            res.Add(typed);

        for (auto& child : widget->GetChildWidgets())
            res.Add(FindAll<_type>(child));

        return res;
    }

    // The field of the properties window built for the field name, shown now
    template<typename _type>
    Ref<_type> FindField(const String& fieldName, bool hiddenToo = false)
    {
        for (auto& field : FindAll<_type>(o2EditorPropertiesWindow.GetWindow()))
        {
            if ((hiddenToo || field->IsEnabledInHierarchy()) && field->GetName().StartsWith(fieldName + " : "))
                return field;
        }

        return nullptr;
    }

    // The group of icons of the properties window with the name
    Ref<SegmentedIcons> FindGroup(const String& name)
    {
        for (auto& group : FindAll<SegmentedIcons>(o2EditorPropertiesWindow.GetWindow()))
        {
            if (group->IsEnabledInHierarchy() && group->GetName() == name)
                return group;
        }

        return nullptr;
    }

    Vec2F SegmentCenter(const Ref<SegmentedIcons>& group, int index)
    {
        return group->GetSegmentRect(index).Center();
    }

    Vec2F OptionCenter(const Ref<EnumIconsProperty>& field, int value)
    {
        return SegmentCenter(field->GetSegments(), field->GetOptionIndex(value));
    }

    bool IsChosen(const Ref<EnumIconsProperty>& field, int value)
    {
        return field->GetSegments()->GetSelected() == field->GetOptionIndex(value);
    }

    Ref<Label> MakeLabel(const String& name, const WString& text, const Vec2F& position)
    {
        ForcePopEditorScopeOnStack scope;

        auto label = o2UI.CreateWidget<Label>();
        label->name = name;
        label->text = text;
        *label->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(200, 40), position);
        o2Scene.UpdateAddedEntities();
        return label;
    }

    void ClearScene()
    {
        ForcePopEditorScopeOnStack scope;
        o2Scene.Clear(true);
        o2Scene.UpdateDestroyingEntities();
    }

    SceneEditableObject* AsEditable(const Ref<Actor>& actor)
    {
        return dynamic_cast<SceneEditableObject*>(actor.Get());
    }
}

class TextPropertiesUI: public EditorVectorUIFixture
{
public:
    void Select(const Vector<Ref<SceneEditableObject>>& objects, Mouse& mouse)
    {
        o2EditorSceneScreen.SelectObjects(objects, false);
        o2EditorPropertiesWindow.SetTargets(objects.Convert<IObject*>([](auto& x) { return dynamic_cast<IObject*>(x.Get()); }));
        mouse.Step(20);
    }
};

// The alignment and overflow icons of a label set its fields, the one of the value is on, an undo puts them back; the
// expand border shows only while an overflow expands
TEST_F(TextPropertiesUI, LabelAlignmentAndOverflowIcons)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));
    Mouse mouse { shell };

    auto label = MakeLabel("label", "Hello", Vec2F());
    label->horAlign = HorAlign::Middle;
    label->horOverflow = Label::HorOverflow::Dots;
    Select({ DynamicCast<SceneEditableObject>(label) }, mouse);

    auto horAlign = FindField<EnumIconsProperty>("horAlign");
    ASSERT_TRUE(horAlign) << "the label's alignment is a group of icons";
    ASSERT_EQ(horAlign->GetSegments()->GetSegments().Count(), 4);
    EXPECT_TRUE(IsChosen(horAlign, (int)HorAlign::Middle));

    // The block slides from the middle to the right icon
    Vec2F middleCenter = OptionCenter(horAlign, (int)HorAlign::Middle);
    Vec2F rightCenter = OptionCenter(horAlign, (int)HorAlign::Right);
    EXPECT_NEAR(horAlign->GetSegments()->GetThumbRect().Center().x, middleCenter.x, 0.5f);

    mouse.Move(rightCenter);
    o2Input.OnCursorPressed(rightCenter);
    mouse.Step(1);
    o2Input.OnCursorReleased();
    mouse.Step(3);
    EXPECT_EQ(label->GetHorAlign(), HorAlign::Right);
    EXPECT_TRUE(IsChosen(horAlign, (int)HorAlign::Right));
    float slidingX = horAlign->GetSegments()->GetThumbRect().Center().x;
    EXPECT_GT(slidingX, middleCenter.x + 0.5f) << "on its way";
    EXPECT_LT(slidingX, rightCenter.x - 0.5f) << "not there at once";

    float lastX = slidingX;
    for (int i = 0; i < 120 && lastX < rightCenter.x - 0.01f; i++)
    {
        mouse.Step(1);
        float x = horAlign->GetSegments()->GetThumbRect().Center().x;
        EXPECT_GE(x, lastX - 0.01f) << "it slides one way";
        lastX = x;
    }
    EXPECT_NEAR(lastX, OptionCenter(horAlign, (int)HorAlign::Right).x, 0.5f);

    // A click on the chosen icon keeps it
    mouse.Click(rightCenter);
    EXPECT_EQ(label->GetHorAlign(), HorAlign::Right);
    EXPECT_TRUE(IsChosen(horAlign, (int)HorAlign::Right));

    auto verAlign = FindField<EnumIconsProperty>("verAlign");
    ASSERT_TRUE(verAlign);
    EXPECT_GT(verAlign->layout->GetWorldRect().left, horAlign->layout->GetWorldRect().right) << "the groups stand in a row";
    mouse.Click(OptionCenter(verAlign, (int)VerAlign::Bottom));
    EXPECT_EQ(label->GetVerAlign(), VerAlign::Bottom);

    o2EditorSceneWindow.UndoAction();
    mouse.Step(5);
    EXPECT_EQ(label->GetVerAlign(), VerAlign::Middle) << "the icons change through undoable actions";
    EXPECT_TRUE(IsChosen(verAlign, (int)VerAlign::Middle));

    // Overflow: the border of expanding only for Expand
    auto expandBorder = FindField<IPropertyField>("expandBorder");
    auto overflow = FindField<EnumIconsProperty>("horOverflow");
    ASSERT_TRUE(overflow);
    EXPECT_FALSE(expandBorder) << "dots don't expand";

    mouse.Click(OptionCenter(overflow, (int)Label::HorOverflow::Expand));
    EXPECT_EQ(label->GetHorOverflow(), Label::HorOverflow::Expand);
    mouse.Step(5);
    EXPECT_TRUE(FindField<IPropertyField>("expandBorder"));

    mouse.Click(OptionCenter(overflow, (int)Label::HorOverflow::Wrap));
    EXPECT_EQ(label->GetHorOverflow(), Label::HorOverflow::Wrap);
    mouse.Step(5);
    EXPECT_FALSE(FindField<IPropertyField>("expandBorder"));

    o2EditorPropertiesWindow.SetTargets({});
    ClearScene();
}

// A text layer has wrap and dots icons in the row of the alignments instead of the overflows
TEST_F(TextPropertiesUI, TextLayerWrapToggles)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));
    Mouse mouse { shell };

    Ref<Button> button;
    {
        ForcePopEditorScopeOnStack scope;
        button = o2UI.CreateWidget<Button>();
        button->name = "button";
        button->caption = "Play";
        o2Scene.UpdateAddedEntities();
    }

    auto caption = button->GetLayer("caption");
    ASSERT_TRUE(caption);
    auto text = DynamicCast<Text>(caption->GetDrawable());
    ASSERT_TRUE(text);
    text->wordWrap = true;
    text->dotsEngings = false;

    Select({ DynamicCast<SceneEditableObject>(caption) }, mouse);

    EXPECT_FALSE(FindField<EnumIconsProperty>("horOverflow"));
    auto wrap = FindField<BooleanProperty>("wordWrap", true);
    auto dots = FindField<BooleanProperty>("dotsEngings", true);
    ASSERT_TRUE(wrap && dots);

    auto flags = FindGroup("wrap");
    ASSERT_TRUE(flags);
    auto horAlignField = FindField<EnumIconsProperty>("horAlign");
    ASSERT_TRUE(horAlignField);
    EXPECT_EQ(flags->GetParentWidget().Lock(), horAlignField->GetParentWidget().Lock()) << "in the row of the alignments";
    EXPECT_GT(flags->GetTrackRect().left, horAlignField->GetSegments()->GetTrackRect().right);
    EXPECT_FALSE(wrap->IsEnabled()) << "the fields stay hidden, the icons show them";
    EXPECT_TRUE(flags->IsOn(0));
    EXPECT_FALSE(flags->IsOn(1));

    mouse.Click(SegmentCenter(flags, 0));
    EXPECT_FALSE(text->GetWordWrap());
    EXPECT_FALSE(flags->IsOn(0));
    mouse.Click(SegmentCenter(flags, 1));
    EXPECT_TRUE(text->IsDotsEngings());
    EXPECT_TRUE(flags->IsOn(1));

    o2EditorSceneWindow.UndoAction();
    mouse.Step(5);
    EXPECT_FALSE(text->IsDotsEngings());
    EXPECT_FALSE(flags->IsOn(1)) << "the icons follow the fields";

    auto horAlign = FindField<EnumIconsProperty>("horAlign");
    ASSERT_TRUE(horAlign);
    mouse.Click(OptionCenter(horAlign, (int)HorAlign::Left));
    EXPECT_EQ(text->GetHorAlign(), HorAlign::Left);

    o2EditorPropertiesWindow.SetTargets({});
    ClearScene();
}

// The case dropdown shows the case the selected texts are in, none when they aren't in one; the chosen case is put on
// every selected text, Cyrillic too, one undo puts them all back
TEST_F(TextPropertiesUI, CaseDropDownChangesEveryText)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));
    Mouse mouse { shell };

    auto first = MakeLabel("first", "hello wide world", Vec2F(0, 100));
    auto second = MakeLabel("second", L"Привет, мир. как дела?", Vec2F(0, -100));

    Select({ DynamicCast<SceneEditableObject>(first) }, mouse);
    auto caseDropDown = FindAll<DropDown>(o2EditorPropertiesWindow.GetWindow()).FindOrDefault(
        [](auto& x) { return x->GetName() == "case" && x->IsEnabledInHierarchy(); });
    ASSERT_TRUE(caseDropDown);
    EXPECT_EQ(caseDropDown->GetSelectedItemText(), WString("lower case"));

    Select({ DynamicCast<SceneEditableObject>(first), DynamicCast<SceneEditableObject>(second) }, mouse);
    EXPECT_EQ(caseDropDown->GetSelectedItemText(), WString()) << "the second mixes cases";

    auto choose = [&](const WString& name)
    {
        mouse.Pause();
        mouse.Click(caseDropDown->layout->GetWorldRect().Center());
        ASSERT_TRUE(caseDropDown->IsExpanded()) << "a click opens the list";
        int index = caseDropDown->FindItem(name);
        ASSERT_GE(index, 0);
        mouse.Click(caseDropDown->GetItem(index)->layout->GetWorldRect().Center());
        EXPECT_FALSE(caseDropDown->IsExpanded());
    };

    choose("UPPER CASE");
    EXPECT_EQ(first->GetText(), WString("HELLO WIDE WORLD"));
    EXPECT_EQ(second->GetText(), WString(L"ПРИВЕТ, МИР. КАК ДЕЛА?"));
    EXPECT_EQ(caseDropDown->GetSelectedItemText(), WString("UPPER CASE")) << "both are in it now";

    choose("Title Case");
    EXPECT_EQ(first->GetText(), WString("Hello Wide World"));
    EXPECT_EQ(second->GetText(), WString(L"Привет, Мир. Как Дела?"));

    o2EditorSceneWindow.UndoAction();
    mouse.Step(5);
    EXPECT_EQ(first->GetText(), WString("HELLO WIDE WORLD")) << "one undo for both labels";
    EXPECT_EQ(second->GetText(), WString(L"ПРИВЕТ, МИР. КАК ДЕЛА?"));
    EXPECT_EQ(caseDropDown->GetSelectedItemText(), WString("UPPER CASE"));

    choose("Sentence case");
    EXPECT_EQ(first->GetText(), WString("Hello wide world"));
    EXPECT_EQ(second->GetText(), WString(L"Привет, мир. Как дела?"));

    o2EditorPropertiesWindow.SetTargets({});
    ClearScene();
}

// The text settings stand in the column of the other fields: the fields, the dropdown and the groups of icons begin
// where the text field does
TEST_F(TextPropertiesUI, RowsLineUpWithTheFields)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));
    Mouse mouse { shell };

    auto label = MakeLabel("label", "Column", Vec2F());
    Select({ DynamicCast<SceneEditableObject>(label) }, mouse);

    auto fieldLeft = [](const String& fieldName)
    {
        auto field = FindField<IPropertyField>(fieldName);
        if (!field)
            return -10000.0f;

        auto caption = field->FindChildByTypeAndName<Label>("propertyName");
        return caption ? caption->layout->GetWorldRect().right : -10000.0f;
    };

    float column = fieldLeft("text");
    ASSERT_GT(column, -1000.0f);
    EXPECT_NEAR(fieldLeft("height"), column, 1.0f);
    EXPECT_NEAR(fieldLeft("color"), column, 1.0f);

    auto rowColumn = [](const Ref<Widget>& widget)
    {
        for (auto parent = widget->GetParentWidget().Lock(); parent; parent = parent->GetParentWidget().Lock())
        {
            for (auto& child : parent->GetChildWidgets())
            {
                if (child->GetName() == "propertyName")
                    return child->layout->GetWorldRect().right;
            }
        }

        return -10000.0f;
    };

    auto horAlign = FindField<EnumIconsProperty>("horAlign");
    ASSERT_TRUE(horAlign);
    EXPECT_NEAR(rowColumn(horAlign), column, 1.0f);
    EXPECT_NEAR(horAlign->GetSegments()->GetTrackRect().left, horAlign->layout->GetWorldRect().left, 1.0f);

    auto caseDropDown = FindAll<DropDown>(o2EditorPropertiesWindow.GetWindow()).FindOrDefault(
        [](auto& x) { return x->GetName() == "case"; });
    ASSERT_TRUE(caseDropDown);
    EXPECT_NEAR(rowColumn(caseDropDown), column, 1.0f);

    o2EditorPropertiesWindow.SetTargets({});
    ClearScene();
}
