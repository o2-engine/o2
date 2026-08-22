#include "CodeToolTestUtils.h"

using namespace codetool_tests;

TEST(CppParser, NamespacesAndClasses)
{
    auto file = ParseString(R"(
namespace game
{
    namespace ui
    {
        class Control {};
    }

    class Button: public ui::Control
    {
    public:
        class Inner {};
    };
}
)");

    auto game = FindSection(*file, "game");
    ASSERT_NE(game, nullptr);
    EXPECT_FALSE(game->IsClass());

    auto ui = FindSection(*file, "game::ui");
    ASSERT_NE(ui, nullptr);
    EXPECT_EQ(ui->GetParentSection(), game);

    auto control = FindClass(*file, "game::ui::Control");
    ASSERT_NE(control, nullptr);
    EXPECT_TRUE(control->IsClass());
    EXPECT_EQ(control->GetName(), "Control");

    auto button = FindClass(*file, "game::Button");
    ASSERT_NE(button, nullptr);

    auto inner = FindClass(*file, "game::Button::Inner");
    ASSERT_NE(inner, nullptr);
    EXPECT_EQ(inner->GetParentSection(), button);
}

TEST(CppParser, BaseClasses)
{
    auto file = ParseString(R"(
class A {};
class B: public A, protected C, private D, E {};
class F: virtual public A {};
struct S: A {};
)");

    auto b = FindClass(*file, "B");
    ASSERT_NE(b, nullptr);
    ASSERT_EQ(b->GetBaseClasses().size(), 4u);
    EXPECT_EQ(b->GetBaseClasses()[0].GetClassName(), "A");
    EXPECT_EQ(b->GetBaseClasses()[0].GetInheritanceType(), SyntaxProtectionSection::Public);
    EXPECT_EQ(b->GetBaseClasses()[1].GetClassName(), "C");
    EXPECT_EQ(b->GetBaseClasses()[1].GetInheritanceType(), SyntaxProtectionSection::Protected);
    EXPECT_EQ(b->GetBaseClasses()[2].GetClassName(), "D");
    EXPECT_EQ(b->GetBaseClasses()[2].GetInheritanceType(), SyntaxProtectionSection::Private);
    EXPECT_EQ(b->GetBaseClasses()[3].GetClassName(), "E");
    EXPECT_EQ(b->GetBaseClasses()[3].GetInheritanceType(), SyntaxProtectionSection::Private);

    auto f = FindClass(*file, "F");
    ASSERT_NE(f, nullptr);
    ASSERT_EQ(f->GetBaseClasses().size(), 1u);
    EXPECT_EQ(f->GetBaseClasses()[0].GetClassName(), "A");
    EXPECT_EQ(f->GetBaseClasses()[0].GetInheritanceType(), SyntaxProtectionSection::Public);

    auto s = FindClass(*file, "S");
    ASSERT_NE(s, nullptr);
    ASSERT_EQ(s->GetBaseClasses().size(), 1u);
    EXPECT_EQ(s->GetBaseClasses()[0].GetClassName(), "A");
}

TEST(CppParser, Fields)
{
    auto file = ParseString(R"(
class Foo
{
public:
    int publicField = 5;
    static float staticField;
    const char* pointerField = nullptr;
    Vec2F& refField;
    Vector<Pair<int, float>> tableField;

protected:
    bool flag = true;

private:
    int hidden;
};
)");

    auto foo = FindClass(*file, "Foo");
    ASSERT_NE(foo, nullptr);

    auto publicField = FindVariable(foo, "publicField");
    ASSERT_NE(publicField, nullptr);
    EXPECT_EQ(publicField->GetVariableType().GetName(), "int");
    EXPECT_EQ(publicField->GetDefaultValue(), "5");
    EXPECT_EQ(publicField->GetClassSection(), SyntaxProtectionSection::Public);
    EXPECT_FALSE(publicField->IsStatic());

    auto staticField = FindVariable(foo, "staticField");
    ASSERT_NE(staticField, nullptr);
    EXPECT_TRUE(staticField->IsStatic());
    EXPECT_EQ(staticField->GetVariableType().GetName(), "float");

    auto pointerField = FindVariable(foo, "pointerField");
    ASSERT_NE(pointerField, nullptr);
    EXPECT_TRUE(pointerField->GetVariableType().IsConstant());
    EXPECT_TRUE(pointerField->GetVariableType().IsPointer());
    EXPECT_EQ(pointerField->GetVariableType().GetName(), "char*");
    EXPECT_EQ(pointerField->GetDefaultValue(), "nullptr");

    auto refField = FindVariable(foo, "refField");
    ASSERT_NE(refField, nullptr);
    EXPECT_TRUE(refField->GetVariableType().IsReference());
    EXPECT_EQ(refField->GetVariableType().GetName(), "Vec2F&");

    auto tableField = FindVariable(foo, "tableField");
    ASSERT_NE(tableField, nullptr);
    EXPECT_EQ(tableField->GetVariableType().GetName(), "Vector<Pair<int, float>>");

    auto flag = FindVariable(foo, "flag");
    ASSERT_NE(flag, nullptr);
    EXPECT_EQ(flag->GetClassSection(), SyntaxProtectionSection::Protected);
    EXPECT_EQ(flag->GetDefaultValue(), "true");

    auto hidden = FindVariable(foo, "hidden");
    ASSERT_NE(hidden, nullptr);
    EXPECT_EQ(hidden->GetClassSection(), SyntaxProtectionSection::Private);
}

TEST(CppParser, DefaultProtectionSections)
{
    auto file = ParseString(R"(
class C { int classField; };
struct S { int structField; };
)");

    auto classField = FindVariable(FindClass(*file, "C"), "classField");
    ASSERT_NE(classField, nullptr);
    EXPECT_EQ(classField->GetClassSection(), SyntaxProtectionSection::Private);

    auto structField = FindVariable(FindClass(*file, "S"), "structField");
    ASSERT_NE(structField, nullptr);
    EXPECT_EQ(structField->GetClassSection(), SyntaxProtectionSection::Public);
}

TEST(CppParser, Functions)
{
    auto file = ParseString(R"(
class Foo
{
public:
    Foo();
    Foo(int x);
    virtual ~Foo();

    void DoIt(int a, float b = 1.0f);
    virtual bool IsOn() const;
    static Foo* Make(const std::string& name);
    std::map<int, float> GetTable() const;

    template<typename T>
    void Templated(T t);
};
)");

    auto foo = FindClass(*file, "Foo");
    ASSERT_NE(foo, nullptr);

    auto ctor = FindFunction(foo, "Foo");
    ASSERT_NE(ctor, nullptr);

    auto doIt = FindFunction(foo, "DoIt");
    ASSERT_NE(doIt, nullptr);
    EXPECT_EQ(doIt->GetReturnType().GetName(), "void");
    ASSERT_EQ(doIt->GetParameters().size(), 2u);
    EXPECT_EQ(doIt->GetParameters()[0]->GetVariableType().GetName(), "int");
    EXPECT_EQ(doIt->GetParameters()[0]->GetName(), "a");
    EXPECT_EQ(doIt->GetParameters()[1]->GetVariableType().GetName(), "float");
    EXPECT_EQ(doIt->GetParameters()[1]->GetName(), "b");
    EXPECT_EQ(doIt->GetParameters()[1]->GetDefaultValue(), "1.0f");

    auto isOn = FindFunction(foo, "IsOn");
    ASSERT_NE(isOn, nullptr);
    EXPECT_EQ(isOn->GetReturnType().GetName(), "bool");
    EXPECT_FALSE(isOn->IsStatic());

    auto make = FindFunction(foo, "Make");
    ASSERT_NE(make, nullptr);
    EXPECT_TRUE(make->IsStatic());
    EXPECT_EQ(make->GetReturnType().GetName(), "Foo*");
    ASSERT_EQ(make->GetParameters().size(), 1u);
    EXPECT_TRUE(make->GetParameters()[0]->GetVariableType().IsConstant());
    EXPECT_EQ(make->GetParameters()[0]->GetVariableType().GetName(), "std::string&");

    auto getTable = FindFunction(foo, "GetTable");
    ASSERT_NE(getTable, nullptr);
    EXPECT_EQ(getTable->GetReturnType().GetName(), "std::map<int, float>");

    auto templated = FindFunction(foo, "Templated");
    ASSERT_NE(templated, nullptr);
    EXPECT_TRUE(templated->IsTemplate());
    EXPECT_EQ(templated->GetTemplates(), "typename T");
}

TEST(CppParser, MacroMarkersParsedAsFunctions)
{
    auto file = ParseString(R"(
class Foo
{
public:
    IOBJECT(Foo);
    SERIALIZABLE(Foo);
};
)");

    auto foo = FindClass(*file, "Foo");
    ASSERT_NE(foo, nullptr);
    EXPECT_NE(FindFunction(foo, "IOBJECT"), nullptr);
    EXPECT_NE(FindFunction(foo, "SERIALIZABLE"), nullptr);
}

TEST(CppParser, TemplateClasses)
{
    auto file = ParseString(R"(
namespace game
{
    template<typename T, typename U>
    class Pair
    {
    public:
        T first;
        U second;

        class Inner {};
    };

    template<typename T>
    struct Holder {};
}
)");

    auto pair = FindClass(*file, "game::Pair");
    ASSERT_NE(pair, nullptr);
    EXPECT_TRUE(pair->IsTemplate());
    EXPECT_EQ(pair->GetTemplateParameters(), "typename T, typename U");

    auto inner = FindClass(*file, "game::Pair::Inner");
    ASSERT_NE(inner, nullptr);
    EXPECT_TRUE(inner->IsTemplate());
    EXPECT_TRUE(inner->GetTemplateParameters().empty());

    auto holder = FindClass(*file, "game::Holder");
    ASSERT_NE(holder, nullptr);
    EXPECT_TRUE(holder->IsTemplate());
    EXPECT_EQ(holder->GetTemplateParameters(), "typename T");
}

TEST(CppParser, Enums)
{
    auto file = ParseString(R"(
enum class Direction { Left, Right, Up = 10, Down };

namespace game
{
    enum Flags
    {
        First = 1,
        Second = 2, // with comment
        Third = First | Second
    };

    class Holder
    {
    public:
        enum class State { On, Off };

    private:
        enum class Hidden { A };
    };
}
)");

    auto direction = FindEnum(*file, "Direction");
    ASSERT_NE(direction, nullptr);
    EXPECT_EQ(direction->GetName(), "Direction");
    ASSERT_EQ(direction->GetEntries().size(), 4u);
    EXPECT_EQ(direction->GetEntries().at("Up"), "10");
    EXPECT_EQ(direction->GetEntries().at("Left"), "");

    auto flags = FindEnum(*file, "game::Flags");
    ASSERT_NE(flags, nullptr);
    ASSERT_EQ(flags->GetEntries().size(), 3u);
    EXPECT_EQ(flags->GetEntries().at("First"), "1");
    EXPECT_EQ(flags->GetEntries().at("Third"), "First | Second");

    auto state = FindEnum(*file, "game::Holder::State");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->GetClassSection(), SyntaxProtectionSection::Public);
    EXPECT_EQ(state->GetOwnerSyntaxSection(), FindClass(*file, "game::Holder"));

    auto hidden = FindEnum(*file, "game::Holder::Hidden");
    ASSERT_NE(hidden, nullptr);
    EXPECT_EQ(hidden->GetClassSection(), SyntaxProtectionSection::Private);
}

TEST(CppParser, TypedefsAndUsingNamespaces)
{
    auto file = ParseString(R"(
typedef std::vector<int> IntsVec;

namespace game
{
    typedef Map<String, int> Dict;

    using namespace o2;
}
)");

    auto& globalTypedefs = file->GetGlobalNamespace()->GetTypedefs();
    ASSERT_EQ(globalTypedefs.size(), 1u);
    EXPECT_EQ(globalTypedefs[0]->GetWhatName(), "std::vector<int>");
    EXPECT_EQ(globalTypedefs[0]->GetNewDefName(), "IntsVec");

    auto game = FindSection(*file, "game");
    ASSERT_NE(game, nullptr);
    ASSERT_EQ(game->GetTypedefs().size(), 1u);
    EXPECT_EQ(game->GetTypedefs()[0]->GetWhatName(), "Map<String, int>");
    EXPECT_EQ(game->GetTypedefs()[0]->GetNewDefName(), "Dict");

    ASSERT_EQ(game->GetUsingNamespaces().size(), 1u);
    EXPECT_EQ(game->GetUsingNamespaces()[0]->GetUsingNamespaceName(), "o2");
}

TEST(CppParser, CommentAttachment)
{
    auto file = ParseString(R"(
class Foo
{
public:
    // Comment for a
    int a;

    int b; // Inline for b

    // Multi line one
    // and second line
    int c;

    // Far away comment

    int d;

    int e; // Inline for e
    // Comment for f
    int f;

    // Comment for method
    void Method();
};
)");

    auto foo = FindClass(*file, "Foo");
    ASSERT_NE(foo, nullptr);

    auto a = FindVariable(foo, "a");
    ASSERT_NE(a, nullptr);
    ASSERT_NE(a->GetComment(), nullptr);
    EXPECT_EQ(a->GetComment()->GetData(), "Comment for a");

    auto b = FindVariable(foo, "b");
    ASSERT_NE(b, nullptr);
    ASSERT_NE(b->GetComment(), nullptr);
    EXPECT_EQ(b->GetComment()->GetData(), "Inline for b");

    auto c = FindVariable(foo, "c");
    ASSERT_NE(c, nullptr);
    ASSERT_NE(c->GetComment(), nullptr);
    EXPECT_EQ(c->GetComment()->GetData(), "Multi line one\nand second line");

    // Not directly above - not attached
    auto d = FindVariable(foo, "d");
    ASSERT_NE(d, nullptr);
    EXPECT_EQ(d->GetComment(), nullptr);

    // Trailing comment goes to e, the next line comment to f
    auto e = FindVariable(foo, "e");
    ASSERT_NE(e, nullptr);
    ASSERT_NE(e->GetComment(), nullptr);
    EXPECT_EQ(e->GetComment()->GetData(), "Inline for e");

    auto f = FindVariable(foo, "f");
    ASSERT_NE(f, nullptr);
    ASSERT_NE(f->GetComment(), nullptr);
    EXPECT_EQ(f->GetComment()->GetData(), "Comment for f");

    auto method = FindFunction(foo, "Method");
    ASSERT_NE(method, nullptr);
    ASSERT_NE(method->GetComment(), nullptr);
    EXPECT_EQ(method->GetComment()->GetData(), "Comment for method");
}

TEST(CppParser, PreprocessorDefines)
{
    auto file = ParseString(R"(
#if IS_EDITOR
class EditorThing
{
public:
    int x;
};
#endif

class After
{
public:
    int y;
};

class Members
{
public:
#ifdef DEBUG_MODE
    int debugField;
#else
    int releaseField;
#endif
    int common;
};
)");

    auto editorThing = FindClass(*file, "EditorThing");
    ASSERT_NE(editorThing, nullptr);
    ASSERT_NE(editorThing->GetDefine(), nullptr);
    EXPECT_EQ(editorThing->GetDefine()->GetDefinition(), " IS_EDITOR");

    auto after = FindClass(*file, "After");
    ASSERT_NE(after, nullptr);
    EXPECT_EQ(after->GetDefine(), nullptr);

    auto members = FindClass(*file, "Members");
    ASSERT_NE(members, nullptr);

    auto debugField = FindVariable(members, "debugField");
    ASSERT_NE(debugField, nullptr);
    ASSERT_NE(debugField->GetDefine(), nullptr);
    EXPECT_EQ(debugField->GetDefine()->GetDefinition(), "defined  DEBUG_MODE");

    auto releaseField = FindVariable(members, "releaseField");
    ASSERT_NE(releaseField, nullptr);
    ASSERT_NE(releaseField->GetDefine(), nullptr);
    EXPECT_EQ(releaseField->GetDefine()->GetDefinition(), "!(defined  DEBUG_MODE)");

    auto common = FindVariable(members, "common");
    ASSERT_NE(common, nullptr);
    EXPECT_EQ(common->GetDefine(), nullptr);
}

TEST(CppParser, NestedPreprocessorConditions)
{
    auto file = ParseString(R"(
class Foo
{
public:
#if OUTER
    int outerBefore;
#ifdef INNER
    int inner;
#endif
    int outerAfter;
#endif
    int free;

#ifndef GUARD
    int underIfndef;
#endif
};
)");

    auto foo = FindClass(*file, "Foo");
    ASSERT_NE(foo, nullptr);

    auto outerBefore = FindVariable(foo, "outerBefore");
    ASSERT_NE(outerBefore, nullptr);
    ASSERT_NE(outerBefore->GetDefine(), nullptr);
    EXPECT_EQ(outerBefore->GetDefine()->GetDefinition(), " OUTER");

    auto inner = FindVariable(foo, "inner");
    ASSERT_NE(inner, nullptr);
    ASSERT_NE(inner->GetDefine(), nullptr);
    EXPECT_EQ(inner->GetDefine()->GetDefinition(), "defined  INNER");

    // After the inner #endif the outer condition is restored
    auto outerAfter = FindVariable(foo, "outerAfter");
    ASSERT_NE(outerAfter, nullptr);
    ASSERT_NE(outerAfter->GetDefine(), nullptr);
    EXPECT_EQ(outerAfter->GetDefine()->GetDefinition(), " OUTER");

    auto freeField = FindVariable(foo, "free");
    ASSERT_NE(freeField, nullptr);
    EXPECT_EQ(freeField->GetDefine(), nullptr);

    auto underIfndef = FindVariable(foo, "underIfndef");
    ASSERT_NE(underIfndef, nullptr);
    ASSERT_NE(underIfndef->GetDefine(), nullptr);
    EXPECT_EQ(underIfndef->GetDefine()->GetDefinition(), "!defined(GUARD)");
}

TEST(CppParser, AttributeDefinitions)
{
    auto file = ParseString(R"(
class MyAttribute
{
public:
    ATTRIBUTE_COMMENT_DEFINITION("MY_ATTR");
    ATTRIBUTE_SHORT_DEFINITION("MY_ATTR_ATTRIBUTE");
};
)");

    auto attr = FindClass(*file, "MyAttribute");
    ASSERT_NE(attr, nullptr);
    EXPECT_EQ(attr->GetAttributeCommentDef(), "MY_ATTR");
    EXPECT_EQ(attr->GetAttributeShortDef(), "MY_ATTR_ATTRIBUTE");
}

TEST(CppParser, AttributesMacro)
{
    auto file = ParseString(R"(
class Foo
{
public:
    ATTRIBUTES(One, two::Three);
    int x;

    ATTRIBUTES(Far);

    int y;
};
)");

    auto foo = FindClass(*file, "Foo");
    ASSERT_NE(foo, nullptr);

    auto x = FindVariable(foo, "x");
    ASSERT_NE(x, nullptr);
    ASSERT_NE(x->GetAttributesMacro(), nullptr);

    auto& list = x->GetAttributesMacro()->GetAttributesList();
    ASSERT_EQ(list.size(), 2u);
    EXPECT_EQ(list[0], "One");
    EXPECT_EQ(list[1], "two::Three");

    // ATTRIBUTES not directly above the member is not attached
    auto y = FindVariable(foo, "y");
    ASSERT_NE(y, nullptr);
    EXPECT_EQ(y->GetAttributesMacro(), nullptr);
}

TEST(CppParser, PropertyMacros)
{
    auto file = ParseString(R"(
class Foo
{
public:
    PROPERTIES(Foo);
    PROPERTY(float, transparency, SetTransparency, GetTransparency);
    GETTER(bool, isVisible, IsVisible);
    SETTER(int, order, SetOrder);
    ACCESSOR(float, param, int, GetParam, GetAllParams);
};
)");

    auto foo = FindClass(*file, "Foo");
    ASSERT_NE(foo, nullptr);

    auto transparency = FindVariable(foo, "transparency");
    ASSERT_NE(transparency, nullptr);
    EXPECT_EQ(transparency->GetVariableType().GetName(), "float");

    auto isVisible = FindVariable(foo, "isVisible");
    ASSERT_NE(isVisible, nullptr);
    EXPECT_EQ(isVisible->GetVariableType().GetName(), "bool");

    auto order = FindVariable(foo, "order");
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->GetVariableType().GetName(), "int");

    auto param = FindVariable(foo, "param");
    ASSERT_NE(param, nullptr);
    EXPECT_EQ(param->GetVariableType().GetName(), "float");
}

TEST(CppParser, FunctionBodiesAreSkipped)
{
    auto file = ParseString(R"(
class Foo
{
public:
    int GetValue() const { return mValue; }

    void Complex()
    {
        if (true) { int local = 0; }
        // comment inside body
        for (int i = 0; i < 10; i++) {}
    }

    int mValue = 0;
};
)");

    auto foo = FindClass(*file, "Foo");
    ASSERT_NE(foo, nullptr);

    EXPECT_NE(FindFunction(foo, "GetValue"), nullptr);
    EXPECT_NE(FindFunction(foo, "Complex"), nullptr);

    EXPECT_EQ(FindVariable(foo, "local"), nullptr);
    ASSERT_NE(FindVariable(foo, "mValue"), nullptr);
    EXPECT_EQ(FindVariable(foo, "mValue")->GetDefaultValue(), "0");
}

TEST(CppParser, TemplateSpecialization)
{
    auto file = ParseString(R"(
namespace o2
{
    template<>
    class Track<o2::Color4>: public ITrack
    {
    public:
        int value = 0;
    };
}
)");

    auto track = FindClass(*file, "o2::Track<o2::Color4>");
    ASSERT_NE(track, nullptr);
    EXPECT_EQ(track->GetName(), "Track<o2::Color4>");
    EXPECT_FALSE(track->IsTemplate());
    ASSERT_EQ(track->GetBaseClasses().size(), 1u);
    EXPECT_EQ(track->GetBaseClasses()[0].GetClassName(), "ITrack");
}

TEST(CppParser, EnumValuesWithShifts)
{
    auto file = ParseString(R"(
enum class Flags
{
    Bool = 1 << 0,
    Int  = 1 << 1,
    Both = Bool | Int,
    Cmp  = 2 > 1
};
)");

    auto flags = FindEnum(*file, "Flags");
    ASSERT_NE(flags, nullptr);
    ASSERT_EQ(flags->GetEntries().size(), 4u);
    EXPECT_EQ(flags->GetEntries().at("Bool"), "1 << 0");
    EXPECT_EQ(flags->GetEntries().at("Int"), "1 << 1");
    EXPECT_EQ(flags->GetEntries().at("Both"), "Bool | Int");
    EXPECT_EQ(flags->GetEntries().at("Cmp"), "2 > 1");
}

TEST(CppParser, EastConstPointerField)
{
    auto file = ParseString(R"(
class Foo
{
public:
    Transform* const transform;
};
)");

    auto transform = FindVariable(FindClass(*file, "Foo"), "transform");
    ASSERT_NE(transform, nullptr);
    EXPECT_EQ(transform->GetVariableType().GetName(), "Transform*");
    EXPECT_TRUE(transform->GetVariableType().IsPointer());
}

TEST(CppParser, PointerToMemberIsSkipped)
{
    auto file = ParseString(R"(
class Foo
{
public:
    void(Foo::*mCallback)();

    int after = 1;
};
)");

    auto foo = FindClass(*file, "Foo");
    ASSERT_NE(foo, nullptr);

    // Pointer-to-member is not reflectable and must not become a function or a variable
    EXPECT_EQ(foo->GetFunctions().size(), 0u);
    ASSERT_EQ(foo->GetVariables().size(), 1u);
    EXPECT_EQ(foo->GetVariables()[0]->GetName(), "after");
}

TEST(CppParser, CommentAttachesToMultilineDeclarationStart)
{
    auto file = ParseString(R"(
class Foo
{
public:
    // Adds layer @SCRIPTABLE
    Ref<Layer> AddLayer(const String& name, const Ref<Drawable>& drawable,
                        const Layout& layout, float depth = 0.0f);
};
)");

    auto addLayer = FindFunction(FindClass(*file, "Foo"), "AddLayer");
    ASSERT_NE(addLayer, nullptr);
    ASSERT_NE(addLayer->GetComment(), nullptr);
    EXPECT_EQ(addLayer->GetComment()->GetData(), "Adds layer @SCRIPTABLE");
    ASSERT_EQ(addLayer->GetParameters().size(), 4u);
    EXPECT_EQ(addLayer->GetParameters()[3]->GetDefaultValue(), "0.0f");
}

TEST(CppParser, ConstructorsAndMarkersAsFunctions)
{
    auto file = ParseString(R"(
class Health
{
public:
    Health();
    Health(game::Config config);

    void Heal(float amount);
};
)");

    auto health = FindClass(*file, "Health");
    ASSERT_NE(health, nullptr);

    // Constructor with a namespace-qualified parameter type is still a function
    int constructors = 0;
    for (auto function : health->GetFunctions())
    {
        if (function->GetName() == "Health")
            constructors++;
    }
    EXPECT_EQ(constructors, 2);

    EXPECT_NE(FindFunction(health, "Heal"), nullptr);
}

TEST(CppParser, EnumEntryWithTrailingCommentBeforeBrace)
{
    auto file = ParseString(R"(
enum class Loop
{
    None,
    Repeat, // repeat comment
    PingPong // last entry comment
};
)");

    auto loop = FindEnum(*file, "Loop");
    ASSERT_NE(loop, nullptr);
    ASSERT_EQ(loop->GetEntries().size(), 3u);
    EXPECT_EQ(loop->GetEntries().count("PingPong"), 1u);
    EXPECT_EQ(loop->GetEntries().count("Repeat"), 1u);
}

TEST(CppParser, DefaultArgumentsWithArrowsAndComparisons)
{
    auto file = ParseString(R"(
class Foo
{
public:
    void SetRange(int a = config->value, int b = 0);
    bool Compare(float x = 1 > 0, int c = 1);
};
)");

    auto foo = FindClass(*file, "Foo");
    ASSERT_NE(foo, nullptr);

    auto setRange = FindFunction(foo, "SetRange");
    ASSERT_NE(setRange, nullptr);
    ASSERT_EQ(setRange->GetParameters().size(), 2u);
    EXPECT_EQ(setRange->GetParameters()[1]->GetVariableType().GetName(), "int");

    auto compare = FindFunction(foo, "Compare");
    ASSERT_NE(compare, nullptr);
    ASSERT_EQ(compare->GetParameters().size(), 2u);
}

TEST(CppParser, RawStringLiteralInBody)
{
    auto file = ParseString(R"__(
class Foo
{
public:
    void Run() { const char* script = R"js(var x = "quoted"; if (x) { })js"; }

    int after = 1;
};

enum class After { A, B };
)__");

    auto foo = FindClass(*file, "Foo");
    ASSERT_NE(foo, nullptr);
    EXPECT_NE(FindVariable(foo, "after"), nullptr);

    auto afterEnum = FindEnum(*file, "After");
    ASSERT_NE(afterEnum, nullptr);
    EXPECT_EQ(afterEnum->GetEntries().size(), 2u);
}

TEST(CppParser, IgnoredFileIsNotParsed)
{
    auto file = ParseString(R"(
// @CODETOOLIGNORE

class NotSeen {};
)");

    EXPECT_EQ(FindClass(*file, "NotSeen"), nullptr);
}
