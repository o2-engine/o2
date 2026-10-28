#include "o2Editor/stdafx.h"
#include "EditorTheme.h"

#include "o2Editor/UI/Style/EditorThemeDark.h"

#include "o2/Assets/Assets.h"
#include "o2/Assets/AssetsTree.h"
#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Assets/Types/VectorImageAsset.h"
#include "o2/EngineSettings.h"
#include "o2/Scene/ActorRefResolver.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Dialogs/YesNoCancelDlg.h"

#include <tuple>


namespace Editor
{
    namespace
    {
        namespace Dark = EditorThemeDark;
        using EditorThemeDark::Hex;

        UInt32 RgbOf(const Color4& c) { return ((UInt32)c.r << 16) | ((UInt32)c.g << 8) | (UInt32)c.b; }

        EditorPalette MakeLight()
        {
            EditorPalette p;
            p.dark = false;
            p.text = Color4(96, 125, 139);
            p.textOnHeader = Color4(235, 255, 253);
            p.ink = Color4(44, 62, 80);
            p.inkSoft = Color4(126, 149, 160);
            p.accent = Color4(0, 156, 141);
            p.viewBack = Color4(225, 232, 232);
            p.viewGrid = Color4(190, 190, 190);
            p.scaleLine = Color4(16, 20, 23, 64);
            p.scaleLineSoft = Color4(16, 20, 23, 38);
            p.frame = Color4(44, 62, 80);
            p.guide = Color4(190, 190, 190);
            p.selectionOutline = Color4(10, 165, 150);
            p.frameObject = Color4(44, 62, 80, 150);
            p.frameParent = Color4(44, 62, 80, 100);
            p.warning = Color4(226, 198, 83);
            p.error = Color4(248, 94, 72);
            p.checkerA = Color4(255, 255, 255);
            p.checkerB = Color4(178, 178, 178);
            p.prototypes[0] = Color4(0, 130, 220);
            p.prototypes[1] = Color4(225, 105, 0);
            p.prototypes[2] = Color4(140, 60, 200);
            p.prototypes[3] = Color4(0, 150, 70);
            p.prototypes[4] = Color4(210, 30, 110);
            return p;
        }

        EditorPalette MakeDark()
        {
            EditorPalette p;
            p.dark = true;
            p.text = Dark::text;
            p.textOnHeader = Dark::text;
            p.ink = Dark::text;
            p.inkSoft = Dark::ink;
            p.accent = Dark::accent;
            p.viewBack = Dark::viewBack;
            p.viewGrid = Dark::viewGrid;
            p.scaleLine = Color4(255, 255, 255, 46);
            p.scaleLineSoft = Color4(255, 255, 255, 24);
            p.frame = Dark::text;
            p.guide = Hex(0x5A5F6E);
            p.selectionOutline = Dark::accent;
            p.iconOff = 0.55f;
            p.frameObject = Color4(Dark::text.r, Dark::text.g, Dark::text.b, 150);
            p.frameParent = Color4(Dark::text.r, Dark::text.g, Dark::text.b, 100);
            p.warning = Dark::yellow;
            p.error = Dark::red;
            p.checkerA = Hex(0x1B1D25);
            p.checkerB = Hex(0x22252E);
            p.prototypes[0] = Hex(0x7FB2FF);
            p.prototypes[1] = Hex(0xFFB070);
            p.prototypes[2] = Hex(0xB79CFF);
            p.prototypes[3] = Hex(0x62D59A);
            p.prototypes[4] = Hex(0xFF7AB6);
            return p;
        }

        // Returns where the editor art is loaded from: the built path of the editor assets tree
        String EditorArtPath()
        {
            if (!Assets::IsSingletonInitialzed())
                return GetEditorBuiltAssetsPath();

            auto tree = o2Assets.GetAssetsTrees().FindOrDefault([](const Ref<AssetsTree>& x) {
                return x.Get() != &o2Assets.GetAssetsTree();
            });

            return tree ? tree->builtAssetsPath : String(GetEditorBuiltAssetsPath());
        }

        EditorThemeKind& CurrentKind()
        {
            static EditorThemeKind kind = EditorThemeKind::Light;
            return kind;
        }

        int& CurrentVersion()
        {
            static int version = 0;
            return version;
        }

        bool IsColorObject(const DataValue& v)
        {
            if (!v.IsObject() || v.GetMembersCount() != 4)
                return false;

            for (const char* key : { "r", "g", "b", "a" })
            {
                auto member = v.FindMember(key);
                if (!member || !member->IsNumber())
                    return false;
            }
            return true;
        }

        void RecolorDocument(DataValue& v)
        {
            if (IsColorObject(v))
            {
                Color4 c((int)(float)v["r"], (int)(float)v["g"], (int)(float)v["b"], (int)(float)v["a"]);
                Color4 to = EditorTheme::StyleColor(c);
                if (to != c)
                {
                    v["r"] = to.r;
                    v["g"] = to.g;
                    v["b"] = to.b;
                    v["a"] = to.a;
                }
                return;
            }

            if (v.IsObject())
            {
                for (auto it = v.BeginMember(); it != v.EndMember(); ++it)
                    RecolorDocument(it->value);
            }
            else if (v.IsArray())
            {
                for (auto& element : v)
                    RecolorDocument(element);
            }
        }
    }

    namespace EditorTheme
    {
        EditorThemeKind GetKind()
        {
            return CurrentKind();
        }

        bool IsDark()
        {
            return CurrentKind() == EditorThemeKind::Dark;
        }

        void SetKind(EditorThemeKind kind)
        {
            bool changed = CurrentKind() != kind;
            CurrentKind() = kind;

            if (kind == EditorThemeKind::Dark)
            {
                // The images of the project keep their colours: only the editor art is restyled
                String editorArt = EditorArtPath();
                VectorImageAsset::SetImageRemap([editorArt](VectorImage& image, const String& path) {
                    if (!editorArt.IsEmpty() && path.StartsWith(editorArt))
                        EditorThemeDark::RestyleImage(image, path);
                });
            }
            else
                VectorImageAsset::SetImageRemap(Function<void(VectorImage&, const String&)>());

            if (changed)
            {
                CurrentVersion()++;
                ReloadVectorArt();
            }
        }

        const EditorPalette& Get()
        {
            return Of(CurrentKind());
        }

        const EditorPalette& Of(EditorThemeKind kind)
        {
            static EditorPalette light = MakeLight();
            static EditorPalette dark = MakeDark();
            return kind == EditorThemeKind::Dark ? dark : light;
        }

        int GetVersion()
        {
            return CurrentVersion();
        }

        Color4 StyleColor(const Color4& c)
        {
            if (!IsDark())
                return c;

            using namespace Dark;
            auto keep = [&](const Color4& to) { return Color4(to.r, to.g, to.b, c.a); };
            switch (RgbOf(c))
            {
            case 0x607D8B: return keep(text);    // (96, 125, 139): labels, values, tree rows, log lines
            case 0xEBFFFD: return keep(text);    // text over the teal heads
            case 0x2C3E50: return keep(text);    // dark ink texts
            case 0x009C8D: return keep(accent);  // text selection, with its own alpha
            case 0x009688: return keep(accent);
            case 0xE1E8E8: return keep(viewBack);
            case 0xFBFBFB: return keep(panel);
            case 0x7E95A0: return keep(ink);
            case 0x1D6969: return keep(ink);
            case 0x186868: return keep(ink);
            case 0xDCDCDC: return keep(border);
            case 0xF85E48: return keep(red);
            case 0xF85D48: return keep(red);
            case 0xF95D48: return keep(red);     // finished state frame
            case 0xE2C653: return keep(yellow);
            case 0xB5CBCC: return keep(knob);    // track of the state progress bar
            case 0x000000:
                if (c.a == 255)
                    return text;                 // carets of the fields

                return c.a > 0 && c.a < 40 ? Color4(255, 255, 255, c.a/2) : c; // zebra and hover washes
            default: return c;
            }
        }

        void RecolorStyles()
        {
            if (!IsDark())
                return;

            PushEditorScopeOnStack scope;
            for (auto& style : o2UI.GetWidgetStyles())
            {
                auto actor = style->GetActor();
                if (!actor)
                    continue;

                DataDocument doc;
                doc = actor;
                RecolorDocument(doc);

                Ref<Actor> copy;
                doc.Get(copy);
                ActorRefResolver::ResolveRefs(true);
                if (copy)
                {
                    copy->RemoveFromScene();

                    // A widget just read places its carets and texts on the next layout pass: until then they are NaN,
                    // and a NaN makes the saved style unreadable
                    if (auto widget = DynamicCast<Widget>(copy))
                        widget->UpdateTransform();

                    const_cast<ActorAsset*>(style.Get())->SetActor(copy);
                }
            }
        }

        void ReloadVectorArt()
        {
            for (auto& asset : Vector<AssetRef<Asset>>(o2Assets.GetCachedAssets()))
            {
                auto image = DynamicCast<VectorImageAsset>(asset.GetRef());
                if (image && image->IsEditorAsset())
                    image->Reload();
            }
        }

        String StylesFolder()
        {
            return IsDark() ? "Editor UI styles dark" : "Editor UI styles";
        }

        TimeStamp RulesDate()
        {
            auto key = [](const TimeStamp& x) {
                return std::make_tuple(x.mYear, x.mMonth, x.mDay, x.mHour, x.mMinute, x.mSecond);
            };

            TimeStamp res;
            for (auto source : { "EditorTheme.cpp", "EditorThemeDark.h" })
            {
                TimeStamp date = o2FileSystem.GetFileInfo(String("../../o2/Editor/Sources/o2Editor/UI/Style/") + source).editDate;
                if (key(res) < key(date))
                    res = date;
            }
            return res;
        }

        EditorThemeKind KindByName(const String& name)
        {
            return name == "dark" ? EditorThemeKind::Dark : EditorThemeKind::Light;
        }

        String NameOf(EditorThemeKind kind)
        {
            return kind == EditorThemeKind::Dark ? "dark" : "light";
        }
    }

    void EditorThemeChoice::Reset(EditorThemeKind next)
    {
        mNext = next;
        preferences.theme = next;
    }

    void EditorThemeChoice::Request(EditorThemeKind kind)
    {
        preferences.theme = kind;
        if (kind == mNext || mAsking)
            return;

        mAsking = true;
        YesNoCancelDlg::ShowYesNo("Switch to the " + EditorTheme::NameOf(kind) + " theme after a restart?",
            [this, kind]()
            {
                mAsking = false;
                mNext = kind;
                preferences.theme = kind;
                onConfirmed(kind);
            },
            [this]()
            {
                mAsking = false;
                preferences.theme = mNext;
            });
    }

    void EditorThemeChoice::Update()
    {
        if (preferences.theme != mNext && !mAsking)
            Request(preferences.theme);
    }

    EditorThemeKind EditorThemeChoice::GetNext() const
    {
        return mNext;
    }

    bool EditorThemeChoice::IsAsking() const
    {
        return mAsking;
    }
}
// --- META ---

ENUM_META(Editor::EditorThemeKind, Editor__EditorThemeKind)
{
    ENUM_ENTRY(Dark);
    ENUM_ENTRY(Light);
}
END_ENUM_META;

DECLARE_CLASS(Editor::EditorPreferences, Editor__EditorPreferences);
// --- END META ---
