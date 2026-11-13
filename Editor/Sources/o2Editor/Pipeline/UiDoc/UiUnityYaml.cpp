#include "o2Editor/stdafx.h"
#include "UiUnityInternal.h"

#include <cmath>
#include <cstring>
#include <functional>

// zlib's global Byte clashes with o2::Byte once the o2 namespace is open
#define Byte ZlibByte
#include "3rdPartyLibs/zlib/zlib.h"
#undef Byte
#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc
{
    using namespace Unity;

    String UnityFloat(double value)
    {
        double x = F(value);
        if (x == 0 && std::signbit(x))
            return "-0";
        if (!std::isfinite(x))
            return JsNum(x);
        if (std::floor(x) == x && std::fabs(x) < 1e15)
            return JsNum(x);
        for (int p = 1; p <= 9; p++)
        {
            std::string s = ToPrecision(x, p);
            if (F(std::strtod(s.c_str(), nullptr)) == x)
            {
                size_t e = s.find('e');
                if (e == std::string::npos)
                    return String(s.c_str());
                int exponent = std::stoi(s.substr(e + 1));
                std::string digits = std::to_string(std::abs(exponent));
                if (digits.size() < 2)
                    digits = "0" + digits;
                return String((s.substr(0, e) + "e" + (exponent < 0 ? "-" : "+") + digits).c_str());
            }
        }
        return JsNum(x);
    }

    String UnityGuid(const String& key)
    {
        char buffer[33];
        snprintf(buffer, sizeof(buffer), "%016llx%016llx", (unsigned long long)Fnv64("guid-a|" + key),
                 (unsigned long long)Fnv64("guid-b|" + key));
        String g = buffer;
        return g == String(std::string(32, '0').c_str()) ? String((std::string(31, '0') + "1").c_str()) : g;
    }

    Vector<String> UiAssetRefs(const Json& structure)
    {
        Vector<String> out;
        std::set<std::string> seen;
        std::function<void(const Json&)> walk = [&](const Json& v)
        {
            if (v.IsArray())
            {
                for (auto& x : v.Items())
                    walk(x);
                return;
            }
            if (!v.IsObject())
                return;
            for (auto& m : v.Members())
            {
                if (m.first == "asset" && m.second.IsString())
                {
                    String name = m.second.Str();
                    if (StartsWith(name, "./"))
                        name = name.SubStr(2);
                    else if (StartsWith(name, "/"))
                        name = name.SubStr(1);
                    if (seen.insert(name.Data()).second)
                        out.Add(name);
                }
                else
                    walk(m.second);
            }
        };
        walk(structure);
        return out;
    }
}

namespace Editor::UiDoc::Unity
{
    // --- YAML ---------------------------------------------------------------------------------------------

    static String Vec2(const V2& v)
    {
        return "{x: " + UnityFloat(v.x) + ", y: " + UnityFloat(v.y) + "}";
    }

    static String ColText(const Col& c)
    {
        return "{r: " + UnityFloat(c.r) + ", g: " + UnityFloat(c.g) + ", b: " + UnityFloat(c.b) + ", a: " + UnityFloat(c.a) + "}";
    }

    static String RefText(const AssetRef* r)
    {
        return r ? "{fileID: " + r->fileID + ", guid: " + r->guid + ", type: " + String(r->type) + "}" : String("{fileID: 0}");
    }

    static String Local(const String& id)
    {
        return "{fileID: " + (id.IsEmpty() ? String("0") : id) + "}";
    }

    static const char* kHead = "  m_ObjectHideFlags: 0\n  m_CorrespondingSourceObject: {fileID: 0}\n  m_PrefabInstance: {fileID: 0}\n  m_PrefabAsset: {fileID: 0}\n";

    static bool ScriptOf(const String& kind, const char*& guid, const char*& cls)
    {
        static const char* table[][3] = {
            { "Image", "fe87c0e1cc204ed48ad3b37840f39efc", "Image" },
            { "Text", "5f7201a12d95ffc409449d95f23cf332", "Text" },
            { "Button", "4e29b1a8efbd4b44bb3f3716e73f07ff", "Button" },
            { "HLG", "30649d3a9faa99c48a7b1166b86bf2a0", "HorizontalLayoutGroup" },
            { "VLG", "59f8146938fff824cb5fd77236b75775", "VerticalLayoutGroup" },
            { "Grid", "8a8695521f0d02e499659fee002a26c2", "GridLayoutGroup" },
            { "CanvasScaler", "0cd44c1031e13a943bb63640046fad76", "CanvasScaler" },
            { "GraphicRaycaster", "dc42784cf147c0c48a680349fa168899", "GraphicRaycaster" },
            { "RectMask2D", "3312d7739989d2b4e91e6319e9a96d76", "RectMask2D" },
            { "Outline", "e19747de3f5aca642ab2be37e372fb86", "Outline" },
            { "ScrollRect", "1aa08ab6e0800fa44ae55d278d1423e3", "ScrollRect" },
        };
        for (auto& row : table)
        {
            if (kind == row[0])
            {
                guid = row[1];
                cls = row[2];
                return true;
            }
        }
        return false;
    }

    static String GraphicHead(const Comp& c)
    {
        return "  m_Material: {fileID: 0}\n  m_Color: " + ColText(c.color) + "\n  m_RaycastTarget: " + String(c.raycast ? 1 : 0) + "\n" +
            "  m_RaycastPadding: {x: 0, y: 0, z: 0, w: 0}\n  m_Maskable: 1\n  m_OnCullStateChanged:\n    m_PersistentCalls:\n      m_Calls: []\n";
    }

    static String Padding(const int p[4])
    {
        return "  m_Padding:\n    m_Left: " + String(p[0]) + "\n    m_Right: " + String(p[1]) + "\n    m_Top: " + String(p[2]) +
            "\n    m_Bottom: " + String(p[3]) + "\n";
    }

    static String ComponentYaml(const Comp& c, const String& goId)
    {
        if (c.kind == "CanvasRenderer")
            return "--- !u!222 &" + c.id + "\nCanvasRenderer:\n" + kHead + "  m_GameObject: {fileID: " + goId + "}\n  m_CullTransparentMesh: 1\n";
        if (c.kind == "Canvas")
        {
            return "--- !u!223 &" + c.id + "\nCanvas:\n" + kHead + "  m_GameObject: {fileID: " + goId + "}\n  m_Enabled: 1\n  serializedVersion: 3\n  m_RenderMode: 0\n  m_Camera: {fileID: 0}\n" +
                "  m_PlaneDistance: 100\n  m_PixelPerfect: 0\n  m_ReceivesEvents: 1\n  m_OverrideSorting: 0\n  m_OverridePixelPerfect: 0\n" +
                "  m_SortingBucketNormalizedSize: 0\n  m_VertexColorAlwaysGammaSpace: 0\n  m_UseReflectionProbes: 0\n  m_AdditionalShaderChannelsFlag: 0\n" +
                "  m_UpdateRectTransformForStandalone: 0\n  m_SortingLayerID: 0\n  m_SortingOrder: 0\n  m_TargetDisplay: 0\n";
        }

        const char* guid = "";
        const char* cls = "";
        ScriptOf(c.kind, guid, cls);
        String s = "--- !u!114 &" + c.id + "\nMonoBehaviour:\n" + kHead + "  m_GameObject: {fileID: " + goId + "}\n  m_Enabled: 1\n  m_EditorHideFlags: 0\n" +
            "  m_Script: {fileID: 11500000, guid: " + guid + ", type: 3}\n  m_Name: \n  m_EditorClassIdentifier: UnityEngine.UI::UnityEngine.UI." + cls + "\n";

        if (c.kind == "Image")
        {
            s += GraphicHead(c) + "  m_Sprite: " + RefText(c.sprite ? &*c.sprite : nullptr) + "\n  m_Type: " + String(c.imageType) +
                "\n  m_PreserveAspect: 0\n  m_FillCenter: 1\n  m_FillMethod: 4\n" +
                "  m_FillAmount: 1\n  m_FillClockwise: 1\n  m_FillOrigin: 0\n  m_UseSpriteMesh: 0\n  m_PixelsPerUnitMultiplier: " + UnityFloat(c.ppuMul) + "\n";
        }
        else if (c.kind == "Text")
        {
            // FontData.OnBeforeSerialize clamps the sizes
            int size = std::min(std::max(c.fontSize, 0), 300);
            int minSize = std::min(10, size), maxSize = std::min(std::max(40, size), 300);
            s += GraphicHead(c) + "  m_FontData:\n    m_Font: " + RefText(&c.font) + "\n    m_FontSize: " + String(size) + "\n    m_FontStyle: 0\n    m_BestFit: 0\n" +
                "    m_MinSize: " + String(minSize) + "\n    m_MaxSize: " + String(maxSize) + "\n    m_Alignment: " + String(c.alignment) +
                "\n    m_AlignByGeometry: 0\n    m_RichText: 0\n" +
                "    m_HorizontalOverflow: 1\n    m_VerticalOverflow: 1\n    m_LineSpacing: " + UnityFloat(c.lineSpacing) + "\n  m_Text: " + YamlString(c.text) + "\n";
        }
        else if (c.kind == "Outline")
            s += "  m_EffectColor: " + ColText(c.color) + "\n  m_EffectDistance: " + Vec2(c.distance) + "\n  m_UseGraphicAlpha: 0\n";
        else if (c.kind == "Button")
        {
            s += String("  m_Navigation:\n    m_Mode: 3\n    m_WrapAround: 0\n    m_SelectOnUp: {fileID: 0}\n    m_SelectOnDown: {fileID: 0}\n") +
                "    m_SelectOnLeft: {fileID: 0}\n    m_SelectOnRight: {fileID: 0}\n  m_Transition: 0\n  m_Colors:\n" +
                "    m_NormalColor: {r: 1, g: 1, b: 1, a: 1}\n    m_HighlightedColor: {r: 0.9607843, g: 0.9607843, b: 0.9607843, a: 1}\n" +
                "    m_PressedColor: {r: 0.78431374, g: 0.78431374, b: 0.78431374, a: 1}\n    m_SelectedColor: {r: 0.9607843, g: 0.9607843, b: 0.9607843, a: 1}\n" +
                "    m_DisabledColor: {r: 0.78431374, g: 0.78431374, b: 0.78431374, a: 0.5019608}\n    m_ColorMultiplier: 1\n    m_FadeDuration: 0.1\n" +
                "  m_SpriteState:\n    m_HighlightedSprite: {fileID: 0}\n    m_PressedSprite: {fileID: 0}\n    m_SelectedSprite: {fileID: 0}\n" +
                "    m_DisabledSprite: {fileID: 0}\n  m_AnimationTriggers:\n    m_NormalTrigger: Normal\n    m_HighlightedTrigger: Highlighted\n" +
                "    m_PressedTrigger: Pressed\n    m_SelectedTrigger: Selected\n    m_DisabledTrigger: Disabled\n  m_Interactable: 1\n" +
                "  m_TargetGraphic: " + Local(c.target ? c.target->id : String()) + "\n  m_OnClick:\n    m_PersistentCalls:\n      m_Calls: []\n";
        }
        else if (c.kind == "Grid")
        {
            s += Padding(c.padding) + "  m_ChildAlignment: " + String(c.alignment) + "\n  m_StartCorner: 0\n  m_StartAxis: 0\n  m_CellSize: " + Vec2(c.cell) + "\n" +
                "  m_Spacing: " + Vec2(c.gridSpacing) + "\n  m_Constraint: 1\n  m_ConstraintCount: " + String(c.columns) + "\n";
        }
        else if (c.kind == "HLG" || c.kind == "VLG")
        {
            s += Padding(c.padding) + "  m_ChildAlignment: " + String(c.alignment) + "\n  m_Spacing: " + UnityFloat(c.spacing) + "\n  m_ChildForceExpandWidth: 0\n" +
                "  m_ChildForceExpandHeight: 0\n  m_ChildControlWidth: 0\n  m_ChildControlHeight: 0\n  m_ChildScaleWidth: 0\n  m_ChildScaleHeight: 0\n" +
                "  m_ReverseArrangement: 0\n";
        }
        else if (c.kind == "CanvasScaler")
        {
            s += "  m_UiScaleMode: 1\n  m_ReferencePixelsPerUnit: 100\n  m_ScaleFactor: 1\n  m_ReferenceResolution: " + Vec2(c.reference) + "\n" +
                "  m_ScreenMatchMode: " + String(c.expand ? 1 : 0) + "\n  m_MatchWidthOrHeight: 0.5\n  m_PhysicalUnit: 3\n  m_FallbackScreenDPI: 96\n  m_DefaultSpriteDPI: 96\n" +
                "  m_DynamicPixelsPerUnit: 1\n  m_PresetInfoIsWorld: 0\n";
        }
        else if (c.kind == "GraphicRaycaster")
            s += "  m_IgnoreReversedGraphics: 1\n  m_BlockingObjects: 0\n  m_BlockingMask:\n    serializedVersion: 2\n    m_Bits: 4294967295\n";
        else if (c.kind == "RectMask2D")
            s += "  m_Padding: {x: 0, y: 0, z: 0, w: 0}\n  m_Softness: {x: 0, y: 0}\n";
        else if (c.kind == "ScrollRect")
        {
            s += "  m_Content: " + Local(c.content ? c.content->rtId : String()) + "\n  m_Horizontal: " + String(c.horizontal ? 1 : 0) +
                "\n  m_Vertical: " + String(c.vertical ? 1 : 0) + "\n" +
                "  m_MovementType: 2\n  m_Elasticity: 0.1\n  m_Inertia: 1\n  m_DecelerationRate: 0.135\n  m_ScrollSensitivity: 20\n" +
                "  m_Viewport: " + Local(c.viewport ? c.viewport->rtId : String()) + "\n  m_HorizontalScrollbar: {fileID: 0}\n  m_VerticalScrollbar: {fileID: 0}\n" +
                "  m_HorizontalScrollbarVisibility: 0\n  m_VerticalScrollbarVisibility: 0\n  m_HorizontalScrollbarSpacing: 0\n" +
                "  m_VerticalScrollbarSpacing: 0\n  m_OnValueChanged:\n    m_PersistentCalls:\n      m_Calls: []\n";
        }
        return s;
    }

    String YamlString(const String& text)
    {
        std::string s = text.Data();
        if (s.empty())
            return "";

        bool printable = true;
        for (unsigned char c : s)
            printable = printable && c >= 0x20 && c <= 0x7e;
        if (!printable)
        {
            std::string out = "\"";
            for (char32_t c : Utf8ToCodePoints(text))
            {
                char buffer[16];
                if (c == '"')
                    out += "\\\"";
                else if (c == '\\')
                    out += "\\\\";
                else if (c == '\n')
                    out += "\\n";
                else if (c == '\t')
                    out += "\\t";
                else if (c == '\r')
                    out += "\\r";
                else if (c >= 0x20 && c <= 0x7e)
                    out += (char)c;
                else if (c > 0xffff)
                {
                    snprintf(buffer, sizeof(buffer), "\\U%08X", (unsigned)c);
                    out += buffer;
                }
                else
                {
                    snprintf(buffer, sizeof(buffer), "\\u%04X", (unsigned)c);
                    out += buffer;
                }
            }
            return String((out + "\"").c_str());
        }

        char c0 = s[0];
        bool quote = ((c0 == '-' || c0 == '?' || c0 == ':') && (s.size() == 1 || s[1] == ' ')) ||
            std::strchr(",[]{}#&*!|>'\"%@`", c0) != nullptr ||
            s.find(": ") != std::string::npos || s.find(" #") != std::string::npos || s.back() == ':' ||
            c0 == ' ' || s.back() == ' ';
        if (!quote)
            return text;
        std::string out = "'";
        for (char c : s)
            out += c == '\'' ? std::string("''") : std::string(1, c);
        return String((out + "'").c_str());
    }

    String PrefabYaml(GO* root, Ids& ids)
    {
        // Ids: the GameObject, its RectTransform, then each component, keyed by the path
        std::function<void(GO*, const String&)> assign = [&](GO* g, const String& path)
        {
            g->goId = ids.Next("go|" + path);
            g->rtId = ids.Next("rt|" + path);
            for (int i = 0; i < (int)g->comps.size(); i++)
                g->comps[i]->id = ids.Next("c|" + path + "|" + String(i) + "|" + g->comps[i]->kind);
            std::map<std::string, int> seen;
            for (auto ch : g->children)
            {
                int n = ++seen[ch->name.Data()];
                assign(ch, path + "/" + ch->name + (n > 1 ? "#" + String(n) : String()));
            }
        };
        assign(root, root->name);

        String out = "%YAML 1.1\n%TAG !u! tag:unity3d.com,2011:\n";
        for (GO* g : Collect(root))
        {
            out += "--- !u!1 &" + g->goId + "\nGameObject:\n" + kHead + "  serializedVersion: 6\n  m_Component:\n  - component: {fileID: " + g->rtId + "}\n";
            for (auto& c : g->comps)
                out += "  - component: {fileID: " + c->id + "}\n";
            out += "  m_Layer: 5\n  m_Name: " + YamlString(g->name) + "\n  m_TagString: Untagged\n  m_Icon: {fileID: 0}\n  m_NavMeshLayer: 0\n" +
                "  m_StaticEditorFlags: 0\n  m_IsActive: " + String(g->active ? 1 : 0) + "\n";

            const Drive& d = g->drive;
            V2 zero;
            V2 am = d.all || d.anchors ? zero : g->Am(), ax = d.all || d.anchors ? zero : g->Ax();
            V2 ap = MakeV2(d.all || d.x ? 0 : g->Ap().x, d.all || d.y ? 0 : g->Ap().y);
            V2 sd = MakeV2(d.all || d.sx ? 0 : g->Sd().x, d.all || d.sy ? 0 : g->Sd().y);
            V2 pv = d.all ? zero : g->Pv();
            double sc[3] = { d.all ? 0 : g->sc[0], d.all ? 0 : g->sc[1], d.all ? 0 : g->sc[2] };

            // A turned block: the quaternion of a turn about z and the editor's angle hint
            double half = (g->rotZ*3.141592653589793)/360;
            String rot = g->rotZ ? "{x: 0, y: 0, z: " + UnityFloat(F(std::sin(half))) + ", w: " + UnityFloat(F(std::cos(half))) + "}"
                                 : String("{x: 0, y: 0, z: 0, w: 1}");
            out += "--- !u!224 &" + g->rtId + "\nRectTransform:\n" + kHead + "  m_GameObject: {fileID: " + g->goId + "}\n  m_LocalRotation: " + rot + "\n" +
                "  m_LocalPosition: {x: 0, y: 0, z: 0}\n  m_LocalScale: {x: " + UnityFloat(sc[0]) + ", y: " + UnityFloat(sc[1]) + ", z: " + UnityFloat(sc[2]) +
                "}\n  m_ConstrainProportionsScale: 0\n";
            if (g->children.empty())
                out += "  m_Children: []\n";
            else
            {
                out += "  m_Children:\n";
                for (auto c : g->children)
                    out += "  - {fileID: " + c->rtId + "}\n";
            }
            out += "  m_Father: {fileID: " + (g->parent ? g->parent->rtId : String("0")) + "}\n  m_LocalEulerAnglesHint: {x: 0, y: 0, z: " + (g->rotZ ? UnityFloat(F(g->rotZ)) : String("0")) + "}\n" +
                "  m_AnchorMin: " + Vec2(am) + "\n  m_AnchorMax: " + Vec2(ax) + "\n  m_AnchoredPosition: " + Vec2(ap) + "\n  m_SizeDelta: " + Vec2(sd) +
                "\n  m_Pivot: " + Vec2(pv) + "\n";
            for (auto& c : g->comps)
                out += ComponentYaml(*c, g->goId);
        }
        return out;
    }

    // --- metas and generated assets ----------------------------------------------------------------------

    String FolderMeta(const String& guid)
    {
        return "fileFormatVersion: 2\nguid: " + guid + "\nfolderAsset: yes\nDefaultImporter:\n  externalObjects: {}\n  userData: \n  assetBundleName: \n  assetBundleVariant: \n";
    }

    String PrefabMeta(const String& guid)
    {
        return "fileFormatVersion: 2\nguid: " + guid + "\nPrefabImporter:\n  externalObjects: {}\n  userData: \n  assetBundleName: \n  assetBundleVariant: \n";
    }

    String TextMeta(const String& guid)
    {
        return "fileFormatVersion: 2\nguid: " + guid + "\nTextScriptImporter:\n  externalObjects: {}\n  userData: \n  assetBundleName: \n  assetBundleVariant: \n";
    }

    String NativeMeta(const String& guid, const String& mainObject)
    {
        return "fileFormatVersion: 2\nguid: " + guid + "\nNativeFormatImporter:\n  externalObjects: {}\n  mainObjectFileID: " + mainObject +
            "\n  userData: \n  assetBundleName: \n  assetBundleVariant: \n";
    }

    String FontMeta(const String& guid)
    {
        return "fileFormatVersion: 2\nguid: " + guid + "\nTrueTypeFontImporter:\n  externalObjects: {}\n  serializedVersion: 4\n  fontSize: 16\n" +
            "  forceTextureCase: -2\n  characterSpacing: 0\n  characterPadding: 1\n  includeFontData: 1\n  fontNames:\n  - Roboto\n" +
            "  fallbackFontReferences: []\n  customCharacters: \n  fontRenderingMode: 0\n  ascentCalculationMode: 1\n  useLegacyBoundsCalculation: 0\n" +
            "  shouldRoundAdvanceValue: 1\n  subsetRecipes: []\n  userData: \n  assetBundleName: \n  assetBundleVariant: \n";
    }

    // A Sprite texture as UiPrefabBuilder.ImportSprite imports it (single, full rect, uncompressed, no mipmaps)
    String TextureMeta(const String& guid, const V4& border)
    {
        auto platform = [](const char* target, int max, int compression)
        {
            return String("  - serializedVersion: 4\n    buildTarget: ") + target + "\n    maxTextureSize: " + String(max) + "\n    resizeAlgorithm: 0\n    textureFormat: -1\n" +
                "    textureCompression: " + String(compression) + "\n    compressionQuality: 50\n    crunchedCompression: 0\n    allowsAlphaSplitting: 0\n    overridden: 0\n" +
                "    ignorePlatformSupport: 0\n    androidETC2FallbackOverride: 0\n    forceMaximumCompressionQuality_BC6H_BC7: 0\n";
        };
        return "fileFormatVersion: 2\nguid: " + guid + "\nTextureImporter:\n  internalIDToNameTable: []\n  externalObjects: {}\n  serializedVersion: 13\n" +
            "  mipmaps:\n    mipMapMode: 0\n    enableMipMap: 0\n    sRGBTexture: 1\n    linearTexture: 0\n    fadeOut: 0\n    borderMipMap: 0\n" +
            "    mipMapsPreserveCoverage: 0\n    alphaTestReferenceValue: 0.5\n    mipMapFadeDistanceStart: 1\n    mipMapFadeDistanceEnd: 3\n" +
            "  bumpmap:\n    convertToNormalMap: 0\n    externalNormalMap: 0\n    heightScale: 0.25\n    normalMapFilter: 0\n    flipGreenChannel: 0\n" +
            "  isReadable: 0\n  streamingMipmaps: 0\n  streamingMipmapsPriority: 0\n  vTOnly: 0\n  ignoreMipmapLimit: 0\n  grayScaleToAlpha: 0\n" +
            "  generateCubemap: 6\n  cubemapConvolution: 0\n  seamlessCubemap: 0\n  textureFormat: 1\n  maxTextureSize: 2048\n" +
            "  textureSettings:\n    serializedVersion: 2\n    filterMode: 1\n    aniso: 1\n    mipBias: 0\n    wrapU: 1\n    wrapV: 1\n    wrapW: 1\n" +
            "  nPOTScale: 0\n  lightmap: 0\n  compressionQuality: 50\n  spriteMode: 1\n  spriteExtrude: 1\n  spriteMeshType: 0\n  alignment: 0\n" +
            "  spritePivot: {x: 0.5, y: 0.5}\n  spritePixelsToUnits: 100\n  spriteBorder: {x: " + UnityFloat(border.x) + ", y: " + UnityFloat(border.y) +
            ", z: " + UnityFloat(border.z) + ", w: " + UnityFloat(border.w) + "}\n" +
            "  spriteGenerateFallbackPhysicsShape: 1\n  alphaUsage: 1\n  alphaIsTransparency: 1\n  spriteTessellationMethod: 0\n" +
            "  spriteTessellationDetail: -1\n  spriteGeometrySubdivision: -1\n  textureType: 8\n  textureShape: 1\n  singleChannelComponent: 0\n" +
            "  flipbookRows: 1\n  flipbookColumns: 1\n  maxTextureSizeSet: 0\n  compressionQualitySet: 0\n  textureFormatSet: 0\n  ignorePngGamma: 0\n" +
            "  applyGammaDecoding: 0\n  swizzle: 50462976\n  cookieLightType: 0\n  platformSettings:\n" +
            platform("DefaultTexturePlatform", 16384, 0) + platform("Standalone", 2048, 1) + platform("WebGL", 2048, 1) +
            "  spriteSheet:\n    serializedVersion: 2\n    sprites: []\n    outline: []\n    customData: \n    physicsShape: []\n    bones: []\n" +
            "    spriteID: \n    internalID: 0\n    vertices: []\n    indices: \n    edges: []\n    weights: []\n    secondaryTextures: []\n" +
            "    spriteCustomMetadata:\n      entries: []\n    nameFileIdTable: {}\n  mipmapLimitGroupName: \n  pSDRemoveMatte: 0\n  userData: \n" +
            "  assetBundleName: \n  assetBundleVariant: \n";
    }

    // The little-endian bytes of a float32 as hex
    static String FloatHex(double v)
    {
        float f = (float)v;
        unsigned char b[4];
        std::memcpy(b, &f, 4);
        char buffer[9];
        snprintf(buffer, sizeof(buffer), "%02x%02x%02x%02x", b[0], b[1], b[2], b[3]);
        return String(buffer);
    }

    // A Sprite asset over a whole texture with its own borders, as Sprite.Create + AssetDatabase.CreateAsset save it
    String SpriteAssetYaml(const String& name, const SpriteInfo& source, const V4& border)
    {
        double ppu = source.ppu, w = source.w, h = source.h;
        double hx = F(F(w / 2) / ppu), hy = F(F(h / 2) / ppu);
        String vertices;
        double corners[4][2] = { { -hx, hy }, { hx, hy }, { -hx, -hy }, { hx, -hy } };
        for (auto& c : corners)
            vertices += FloatHex(c[0]) + FloatHex(c[1]) + FloatHex(0);
        vertices += String(std::string(64, '0').c_str());

        String channels;
        for (int i = 0; i < 14; i++)
        {
            channels += "      - stream: " + String(i == 4 ? 1 : 0) + "\n        offset: 0\n        format: 0\n        dimension: " +
                String(i == 0 ? 3 : i == 4 ? 2 : 0) + "\n";
        }

        auto rd = [&](const char* key)
        {
            return String("  ") + key + ":\n    serializedVersion: 4\n    texture: {fileID: 2800000, guid: " + source.guid + ", type: 3}\n    alphaTexture: {fileID: 0}\n" +
                "    secondaryTextures: []\n    m_SubMeshes:\n    - serializedVersion: 2\n      firstByte: 0\n      indexCount: 6\n      topology: 0\n      baseVertex: 0\n" +
                "      firstVertex: 0\n      vertexCount: 4\n      localAABB:\n        m_Center: {x: 0, y: 0, z: 0}\n        m_Extent: {x: 0, y: 0, z: 0}\n" +
                "    m_IndexBuffer: 000001000200020001000300y\n    m_VertexData:\n      serializedVersion: 3\n      m_VertexCount: 4\n      m_Channels:\n" + channels +
                "      m_DataSize: 80\n      _typelessdata: " + vertices + "\n    m_Bindpose: []\n    m_BlendShapes:\n      vertices: []\n      shapes: []\n" +
                "      channels: []\n      fullWeights: []\n    textureRect:\n      serializedVersion: 2\n      x: 0\n      y: 0\n" +
                "      width: " + UnityFloat(w) + "\n      height: " + UnityFloat(h) + "\n    textureRectOffset: {x: 0, y: 0}\n    atlasRectOffset: {x: -1, y: -1}\n    settingsRaw: 3072\n" +
                "    uvTransform: {x: " + UnityFloat(ppu) + ", y: " + UnityFloat(F(w / 2)) + ", z: " + UnityFloat(ppu) + ", w: " + UnityFloat(F(h / 2)) +
                "}\n    downscaleMultiplier: 1\n";
        };

        return String("%YAML 1.1\n%TAG !u! tag:unity3d.com,2011:\n--- !u!213 &21300000\nSprite:\n  serializedVersion: 2\n") + kHead +
            "  m_Name: " + name + "\n  m_Rect:\n    serializedVersion: 2\n    x: 0\n    y: 0\n    width: " + UnityFloat(w) + "\n    height: " + UnityFloat(h) +
            "\n  m_Offset: {x: 0, y: 0}\n" +
            "  m_Border: {x: " + UnityFloat(border.x) + ", y: " + UnityFloat(border.y) + ", z: " + UnityFloat(border.z) + ", w: " + UnityFloat(border.w) +
            "}\n  m_PixelsToUnits: " + UnityFloat(ppu) + "\n" +
            "  m_Pivot: {x: 0.5, y: 0.5}\n  m_Extrude: 0\n  m_PackingTag: \n  m_RenderDataKey:\n    00000000000000000000000000000000: 0\n" +
            "  m_AtlasTags: []\n  m_SpriteAtlas: {fileID: 0}\n" + rd("m_RD") + rd("m_AtlasRD") +
            "  m_PhysicsShape: []\n  m_Bones: []\n  m_ScriptableObjects: []\n  m_SpriteID: \n";
    }
}

namespace Editor::UiDoc
{
    using namespace Unity;

    // --- the export -------------------------------------------------------------------------------------------

    static String CleanName(const String& name)
    {
        String out;
        for (int i = 0; i < name.Length(); i++)
        {
            char c = name[i];
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')
                out += c;
        }
        return out;
    }

    static String CleanFolder(const String& folder)
    {
        String s = folder;
        s.ReplaceAll("\\", "/");
        Vector<String> parts;
        for (auto& p : JsSplit(s, "/"))
        {
            if (!p.IsEmpty() && p != "." && p != "..")
                parts.Add(p);
        }
        return Join(parts, "/");
    }

    static String LastSegment(const String& path)
    {
        String s = path;
        s.ReplaceAll("\\", "/");
        int slash = s.FindLast("/");
        return slash < 0 ? s : s.SubStr(slash + 1);
    }

    static bool HasFontExtension(const String& file)
    {
        String lower = JsToLower(file);
        return UiLayout::HasSuffix(lower, ".ttf") || UiLayout::HasSuffix(lower, ".otf");
    }

    UiExportFiles ExportUnity(const UiExportInput& input)
    {
        UiExportFiles result;
        try
        {
            String name = CleanName(input.name.IsEmpty() ? String("Screen") : input.name);
            if (name.IsEmpty())
                name = "Screen";
            String cleanFolder = CleanFolder(input.folder);
            String folder = "Assets/" + (cleanFolder.IsEmpty() ? String("Generated/UI") : cleanFolder);
            String prefabPath = folder + "/" + name + ".prefab";
            String resources = folder + "/" + name + "_Resources";
            String spritesFolder = resources + "/Sprites", fontsFolder = resources + "/Fonts";
            std::set<std::string> warnings;

            Json structure;
            String parseError;
            if (!Json::Parse(input.layout, structure, &parseError))
                throw ExportError(("the layout is not JSON: " + parseError).Data());
            Doc doc(structure);
            const String& seed = input.seed;
            Vector<Pending> files;
            auto guidOf = [&](const String& path) { return UnityGuid(seed + "|" + path); };

            // The sprites the layout names, saved as Sprite textures with their borders
            std::map<std::string, String> pngs;
            UnityAssets assets(spritesFolder, seed, warnings, pngs);
            std::map<std::string, const UiExportSprite*> byName;
            for (auto& s : input.sprites)
                byName[JsToLower(s.file).Data()] = &s;
            for (auto& r : UiAssetRefs(structure))
            {
                auto it = byName.find(JsToLower(r).Data());
                if (it == byName.end())
                    it = byName.find(JsToLower(LastSegment(r)).Data());
                if (it == byName.end())
                    throw ExportError(("no picture for the sprite " + r + " (run the node first)").Data());

                const UiExportSprite& s = *it->second;
                String path = spritesFolder + "/" + r;
                String guid = guidOf(path);
                V4 border;
                if (s.nineSlice)
                    border = V4{ F(s.nineSlice->left), F(s.nineSlice->bottom), F(s.nineSlice->right), F(s.nineSlice->top) };
                files.Add({ path, s.png });
                files.Add({ path + ".meta", TextureMeta(guid, border) });
                pngs[path.Data()] = s.png;

                SpriteInfo info;
                info.fileID = "21300000";
                info.guid = guid;
                info.type = 3;
                info.file = r;
                info.w = s.width;
                info.h = s.height;
                info.border = border;
                info.ppu = 100;
                info.texture = path;
                if (s.density && *s.density != 0 && *s.density != 1)
                    info.density = s.density;
                assets.sprites[JsToLower(r).Data()] = info;
            }

            // The bundled fonts, and which family names them
            std::map<std::string, FontInfo> fontByFile;
            for (auto& font : input.fonts)
            {
                String path = fontsFolder + "/" + font.file;
                String guid = guidOf(path);
                bool isFont = HasFontExtension(font.file);
                files.Add({ path, font.data });
                files.Add({ path + ".meta", isFont ? FontMeta(guid) : TextMeta(guid) });
                if (isFont)
                {
                    FontInfo info;
                    info.fileID = "12800000";
                    info.guid = guid;
                    info.type = 3;
                    info.file = font.file;
                    fontByFile[JsToLower(font.file).Data()] = info;
                }
            }
            auto regular = fontByFile.find("roboto-regular.ttf");
            if (regular != fontByFile.end())
                assets.defaultFont = regular->second;

            auto candidates = doc.Candidates();
            for (auto& candidate : candidates)
            {
                const Json& node = candidate.second;
                if (TextOf(&node, "type") != "text")
                    continue;
                String family = TextOf(&node, "fontFamily");
                if (assets.fonts.count(family.Data()))
                    continue;
                // The bundled Roboto faces, or any other font the caller packed in (the AI UI layout's text styles)
                auto found = fontByFile.find(JsToLower(LastSegment(family)).Data());
                if (found != fontByFile.end())
                    assets.fonts[family.Data()] = found->second;
                else if (!family.IsEmpty())
                {
                    warnings.insert((candidate.first + ": font '" + family + "' unavailable; using " +
                                     (regular != fontByFile.end() ? String("Roboto-Regular.ttf") : String("Unity LegacyRuntime font")) + ".").Data());
                }
            }
            for (auto& candidate : candidates)
            {
                const Json& node = candidate.second;
                const Json* slice = node.Get("nineSlice");
                String asset = TextOf(&node, "asset");
                bool inferred = slice && ((slice->IsBool() && slice->Bool()) || slice->IsString());
                if (asset.IsEmpty() || !inferred)
                    continue;
                auto sprite = assets.sprites.find(JsToLower(asset).Data());
                if (IsZero4(sprite != assets.sprites.end() ? sprite->second.border : V4{ 1, 0, 0, 0 }))
                    warnings.insert((candidate.first + ": nine-slice margins inferred because source spriteBorder metadata is absent.").Data());
            }

            Ids ids(seed + "|" + prefabPath);
            Assembler assembler(doc, assets);
            assembler.expand = input.canvasFit == "expand";
            GO* root = assembler.Build(name);
            MarkDriven(root);
            files.Add({ prefabPath, PrefabYaml(root, ids) });
            files.Add({ prefabPath + ".meta", PrefabMeta(guidOf(prefabPath)) });
            for (auto& f : assets.files)
                files.Add(f);

            // Folder metas for every folder below Assets (their guids from the path alone: exports share them)
            std::set<std::string> folders;
            for (auto& f : files)
            {
                auto parts = JsSplit(f.path, "/");
                for (int i = 2; i < parts.Count(); i++)
                {
                    Vector<String> head;
                    for (int k = 0; k < i; k++)
                        head.Add(parts[k]);
                    folders.insert(Join(head, "/").Data());
                }
            }
            for (auto& d : folders)
                files.Add({ String(d.c_str()) + ".meta", FolderMeta(UnityGuid("folder|" + String(d.c_str()))) });

            for (auto& f : files)
                result.files.Add({ f.path, f.data });
            for (auto& w : warnings)
                result.warnings.Add(String(w.c_str()));
            result.ok = true;
        }
        catch (const std::exception& e)
        {
            result = UiExportFiles();
            result.error = e.what();
        }
        return result;
    }

    // --- .unitypackage ------------------------------------------------------------------------------------

    static const long long kTarTime = 1735689600; // Fixed: the package does not change with the clock

    static void TarEntry(std::string& out, const std::string& name, const std::string* data)
    {
        size_t size = data ? data->size() : 0;
        size_t body = (size + 511) / 512 * 512;
        std::string header(512, '\0');
        auto put = [&](const std::string& s, size_t at, size_t len) { std::memcpy(&header[at], s.data(), std::min(len, s.size())); };
        auto oct = [](long long n, int len)
        {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%0*llo", len - 1, n);
            return std::string(buffer) + '\0';
        };
        put(name, 0, 100);
        put(oct(data ? 0644 : 0755, 8), 100, 8);
        put(oct(0, 8), 108, 8);
        put(oct(0, 8), 116, 8);
        put(oct((long long)size, 12), 124, 12);
        put(oct(kTarTime, 12), 136, 12);
        std::memset(&header[148], ' ', 8);
        header[156] = data ? '0' : '5';
        put(std::string("ustar\0", 6), 257, 6);
        put("00", 263, 2);
        unsigned sum = 0;
        for (unsigned char c : header)
            sum += c;
        char checksum[16];
        snprintf(checksum, sizeof(checksum), "%06o", sum);
        put(std::string(checksum) + '\0' + ' ', 148, 8);

        out += header;
        if (data)
        {
            out += *data;
            out.append(body - size, '\0');
        }
    }

    static std::string Gzip(const std::string& data)
    {
        z_stream zs;
        std::memset(&zs, 0, sizeof(zs));
        if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
            return std::string();
        std::string out(deflateBound(&zs, (uLong)data.size()) + 64, '\0');
        zs.next_in = (Bytef*)data.data();
        zs.avail_in = (uInt)data.size();
        zs.next_out = (Bytef*)&out[0];
        zs.avail_out = (uInt)out.size();
        int status = deflate(&zs, Z_FINISH);
        size_t written = zs.total_out;
        deflateEnd(&zs);
        if (status != Z_STREAM_END)
            return std::string();
        out.resize(written);
        return out;
    }

    String UnityPackage(const Vector<UiExportFile>& files, String* error /*= nullptr*/)
    {
        std::map<std::string, const UiExportFile*> byPath;
        Vector<const UiExportFile*> metas;
        for (auto& f : files)
        {
            byPath[f.path.Data()] = &f;
            if (UiLayout::HasSuffix(f.path, ".meta"))
                metas.Add(&f);
        }
        std::sort(metas.begin(), metas.end(), [](const UiExportFile* a, const UiExportFile* b) { return a->path < b->path; });

        std::string tar;
        for (auto meta : metas)
        {
            std::string text = meta->data.Data();
            std::string guid;
            bool folder = false;
            size_t at = 0;
            while (at < text.size())
            {
                size_t end = text.find('\n', at);
                std::string line = text.substr(at, end == std::string::npos ? std::string::npos : end - at);
                at = end == std::string::npos ? text.size() : end + 1;
                if (line.rfind("guid:", 0) == 0 && guid.empty())
                {
                    std::string value = line.substr(5);
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t\r") + 1);
                    bool hex = value.size() == 32;
                    for (char c : value)
                        hex = hex && ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
                    if (hex)
                        guid = value;
                }
                if (line == "folderAsset: yes")
                    folder = true;
            }
            if (guid.empty())
            {
                if (error)
                    *error = "no guid in " + meta->path;
                return String();
            }

            String asset = meta->path.SubStr(0, meta->path.Length() - 5);
            auto data = byPath.find(asset.Data());
            if (data == byPath.end() && !folder)
            {
                if (error)
                    *error = "no file for " + meta->path;
                return String();
            }

            std::string pathname = asset.Data(), metaData = meta->data.Data();
            TarEntry(tar, guid + "/", nullptr);
            TarEntry(tar, guid + "/pathname", &pathname);
            TarEntry(tar, guid + "/asset.meta", &metaData);
            if (data != byPath.end())
            {
                std::string bytes(data->second->data.Data(), data->second->data.Length());
                TarEntry(tar, guid + "/asset", &bytes);
            }
        }

        // Package Manager dependencies, merged into the project's manifest on import: the prefab is UGUI, which a new
        // Unity 6 project does not have
        std::string manifest = "{\n  \"dependencies\": {\n    \"com.unity.ugui\": \"2.0.0\"\n  }\n}\n";
        TarEntry(tar, "packagemanagermanifest/", nullptr);
        TarEntry(tar, "packagemanagermanifest/asset", &manifest);
        tar.append(1024, '\0');

        std::string packed = Gzip(tar);
        if (packed.empty())
        {
            if (error)
                *error = "could not compress the package";
            return String();
        }
        return String(packed);
    }
}
