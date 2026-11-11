#pragma once

#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"
#include "o2Editor/Pipeline/UiDoc/UiUnityExport.h"

// ------------------------------------------------------------------------------------------------
// Internals of the Unity export (UiUnityExport.h): the structure as the importer reads it, the scene
// graph it builds and the assets it makes. Numbers are doubles rounded to float32 where the web calls
// Math.fround, so every value is the web's to the bit; errors are the web's messages, thrown and turned
// into the result's error by ExportUnity
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc::Unity
{
    // An error of the export: the web's message
    struct ExportError: std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    // Math.fround
    inline double F(double x) { return (double)(float)x; }

    // A float32 pair
    struct V2
    {
        double x = 0, y = 0;
    };

    // A float32 quadruple: nine-slice borders as Unity's Vector4 (left, bottom, right, top)
    struct V4
    {
        double x = 0, y = 0, z = 0, w = 0;
    };

    // Returns the pair rounded to float32
    inline V2 MakeV2(double x, double y) { return V2{ F(x), F(y) }; }

    // Vector arithmetic in float32
    V2 Add(const V2& a, const V2& b);
    V2 Sub(const V2& a, const V2& b);
    V2 Scale(const V2& a, const V2& b);
    bool Eq2(const V2& a, const V2& b);
    bool IsZero4(const V4& b);
    bool Eq4(const V4& a, const V4& b);

    // .NET Math.Round(x): to the even neighbour
    double RoundEven(double x);

    // Both coordinates to the even neighbour
    V2 Round2(const V2& a);

    // Mathf.Approximately
    bool Approximately(double a, double b);

    // Mathf.Repeat
    double Repeat(double t, double length);

    // Number.prototype.toPrecision of a finite value (ties to the larger)
    std::string ToPrecision(double n, int precision);

    // A colour in float32 channels
    struct Col
    {
        double r = 1, g = 1, b = 1, a = 1;
    };

    // ColorUtility.TryParseHtmlString: #rgb, #rgba, #rrggbb, #rrggbbaa or a colour name; throws for anything else
    Col ParseColor(const String& value);

    // ---- reading the structure (UiStructureData) -------------------------------------------------

    // A member as text: a string as it is, any other value as compact JSON, fallback when it is missing
    String TextOf(const Json* obj, const char* key, const String& fallback = "");

    // A member as a number: fallback when missing or null (a NaN fallback makes it required); throws for another type
    double NumOf(const Json* obj, const char* key, double fallback = 0);

    // A member as a boolean: fallback when missing or null; throws for another type
    bool BoolOf(const Json* obj, const char* key, bool fallback = false);

    // {x, y} as a float32 pair (missing members 0)
    V2 PairOf(const Json* obj);

    // {width, height}: both required and positive, rounded to the even neighbour
    V2 SizeOf(const Json* obj, const String& label);

    // The children array of a node (none for null), every child an object
    Vector<const Json*> ChildrenOf(const Json& node);

    // True when a nineSlice value asks for slicing (not undefined, null, false or 0)
    bool HasSlice(const Json* value);

    // A box in the schema's top-left space
    struct UBox
    {
        double x = 0, y = 0, w = 0, h = 0;
    };

    // A ui_structure.json with its nodes by path (root, root/panel, ...), in document order
    class Doc
    {
    public:
        Json   json;
        V2     ref; // Canvas size

        // Indexes the structure; throws when it is not one
        explicit Doc(const Json& structure);

        // Returns the paths in document order
        const Vector<String>& Order() const { return mOrder; }

        // Returns the node of a path; throws for an unknown one
        Json& Node(const String& path);

        // Returns the node a relative path of an owner names; throws when there is none
        Json& Relative(const String& owner, const String& rel);

        // Returns the document with each state's current variant applied and progress labels filled in
        Doc Snapshot() const;

        // Every node and every state variant's patched node, for the assets they name
        Vector<std::pair<String, Json>> Candidates();

        // Boxes in the schema's top-left space by path
        std::map<std::string, UBox> Resolve();

    private:
        std::map<std::string, std::vector<int>> mAddress; // Child indices from the root by path
        Vector<String>                          mOrder;

        void Index(const Json& node, const String& path, const std::vector<int>& address);
        void ResolveChildren(const String& path, std::map<std::string, UBox>& boxes);
    };

    // Returns the path without its last part
    String ParentPath(const String& path);

    // Alignment factor of a word: 0, 0.5 or 1 / 0, 1 or 2
    double FactorF(const String& v);
    int FactorI(const String& v);

    // ---- the scene graph ---------------------------------------------------------------------------

    // A reference to an asset: fileID, guid and type as the YAML writes them
    struct AssetRef
    {
        String fileID;
        String guid;
        int    type = 0;
    };

    // A sprite the prefab uses: its asset, texture size, borders, pixels per unit, texture path and density
    struct SpriteInfo: AssetRef
    {
        String                file;
        double                w = 0, h = 0;
        V4                    border;
        double                ppu = 100;
        String                texture;
        std::optional<double> density;
    };

    // A font asset: file empty for Unity's built-in font
    struct FontInfo: AssetRef
    {
        String file;
    };

    struct GO;

    // A component of a GameObject: every kind's fields in one struct
    struct Comp
    {
        String kind; // Canvas, CanvasScaler, GraphicRaycaster, CanvasRenderer, Image, Text, Outline, Button, Grid, HLG, VLG, RectMask2D, ScrollRect
        String id;
        GO*    go = nullptr;

        Col                       color;
        bool                      raycast = false;
        std::optional<SpriteInfo> sprite;
        int                       imageType = 0;
        double                    ppuMul = 1;
        FontInfo                  font;
        int                       fontSize = 0;
        int                       alignment = 0;
        double                    lineSpacing = 1;
        String                    text;
        V2                        distance;
        Comp*                     target = nullptr;
        V2                        cell;
        V2                        gridSpacing;
        double                    spacing = 0;
        int                       columns = 0;
        int                       padding[4] = { 0, 0, 0, 0 };
        V2                        reference;
        GO*                       viewport = nullptr;
        GO*                       content = nullptr;
        bool                      horizontal = false, vertical = false;
    };

    // What a layout group or the Canvas drives: Unity saves those as zeros
    struct Drive
    {
        bool anchors = false, x = false, y = false, sx = false, sy = false, all = false;
    };

    // A GameObject with its RectTransform
    struct GO
    {
        String                             name;
        std::vector<GO*>                   children;
        GO*                                parent = nullptr;
        std::vector<std::unique_ptr<Comp>> comps;
        bool                               active = true;
        double                             sc[3] = { 1, 1, 1 };
        Drive                              drive;
        String                             goId, rtId;

        explicit GO(const String& name): name(name) {}

        // RectTransform values; a setter keeps the stored value when the new one is approximately the same
        const V2& Am() const { return mAm; }
        const V2& Ax() const { return mAx; }
        const V2& Ap() const { return mAp; }
        const V2& Sd() const { return mSd; }
        const V2& Pv() const { return mPv; }
        void SetAm(const V2& v);
        void SetAx(const V2& v);
        void SetAp(const V2& v);
        void SetSd(const V2& v);
        void SetPv(const V2& v);

        V2 OffsetMin() const;
        void SetOffsetMin(const V2& value);
        V2 OffsetMax() const;
        void SetOffsetMax(const V2& value);

        void SetParent(GO* p, int index = -1);
        int SiblingIndex() const;
        void SetSiblingIndex(int i);
        bool ActiveSelf() const { return active; }
        bool ActiveInHierarchy() const;

        // Returns the first component of a kind
        Comp* Get(const String& kind) const;

        // Returns the first Image or Text
        Comp* Graphic() const;

    private:
        V2 mAm = { 0.5, 0.5 }, mAx = { 0.5, 0.5 }, mAp = { 0, 0 }, mSd = { 100, 100 }, mPv = { 0.5, 0.5 };
    };

    // ---- deterministic ids -------------------------------------------------------------------------

    // FNV-1a over the UTF-8 bytes with a final mix
    UInt64 Fnv64(const String& text);

    // 24 upper-case hex digits of a key
    String Hash24(const String& key);

    // FileIDs: a hash of the seed and a key, unique within one export
    class Ids
    {
    public:
        explicit Ids(const String& seed): mSeed(seed) {}

        String Next(const String& key);

    private:
        String                mSeed;
        std::set<std::string> mUsed;
    };

    // ---- assets (UiPrefabAssets) -------------------------------------------------------------------

    // A file the export writes besides the prefab
    struct Pending
    {
        String path;
        String data;
    };

    // The sprites and fonts of an export, and the sprite assets / baked hues it makes
    class UnityAssets
    {
    public:
        std::map<std::string, SpriteInfo> sprites; // Layout key (lower case) -> source sprite
        std::map<std::string, FontInfo>   fonts;   // fontFamily -> font
        FontInfo                          defaultFont;
        Vector<Pending>                   files;
        std::map<std::string, SpriteInfo> variants;
        std::set<std::string>&            warnings;
        std::map<std::string, String>&    pngs;
        String                            spritesFolder, seed;
        int                               generated = 0, baked = 0;

        UnityAssets(const String& spritesFolder, const String& seed, std::set<std::string>& warnings,
               std::map<std::string, String>& pngs);

        // Returns the font of a family, the default one when it is not packed
        const FontInfo& Font(const String& family) const;

        // Returns the sprite a node draws: hue baked, borders of its nineSlice (a sprite asset when they differ)
        SpriteInfo SpriteFor(const String& key, const Json* slice, double hue);

    private:
        SpriteInfo SpriteAsset(const SpriteInfo& source, const V4& border);
        SpriteInfo BakeHue(const SpriteInfo& source, double degrees);
    };

    // Font.lineHeight and Font.fontSize of a bundled Roboto face as Unity 6 imports it; false for another font
    bool FontMetrics(const String& file, double& lineHeight, double& fontSize);

    // Unity's RGB -> HSV -> RGB of one pixel with the hue turned
    void HueShiftPixel(int r8, int g8, int b8, double degrees, int out[3]);

    // ---- the assembler (UiPrefabAssembler + UiPrefabLayout) ----------------------------------------

    // Builds the GameObjects of a structure
    class Assembler
    {
    public:
        Doc&                        doc;
        UnityAssets&                assets;
        std::map<std::string, UBox> boxes;
        GO*                         root = nullptr;
        int                         images = 0, texts = 0;

        Assembler(Doc& doc, UnityAssets& assets);

        // Builds the prefab root and everything under it
        GO* Build(const String& name);

        // Every GameObject made, for the lifetime of the assembler
        std::vector<std::unique_ptr<GO>> pool;

        GO* NewGO(const String& name);
        GO* NewRect(const String& name, GO* parent);
        Comp* AddComp(GO* go, const String& kind);

    private:
        const UBox& BoxOf(const String& path);
        GO* BuildNode(Doc& source, Doc& snapshot, const String& path, GO* parent);
        void BuildBody(Doc& source, Doc& snapshot, const String& path, GO* rect);
        void AddText(GO* parent, const Json& node, const Col& color, bool separateVisual);
        void AddScroll(GO* owner, GO* content, const Json& spec, const String& path);
        void SetRectangle(GO* rect, const Json& node, const UBox& box, const V2& parent, const Json* parentNode, bool stretchRoot);
        void AddProgressClip(GO* fill, double ratio, const String& direction, const V2& ref);
        void ApplyLayout(GO* owner, const Json& node, const std::vector<std::pair<String, GO*>>& kids);
    };

    // Marks what layout groups and the Canvas drive
    void MarkDriven(GO* root);

    // Every GameObject, parents first
    std::vector<GO*> Collect(GO* root);

    // ---- YAML ------------------------------------------------------------------------------------

    // The prefab's YAML: ids assigned by path, then every GameObject, its RectTransform and components
    String PrefabYaml(GO* root, Ids& ids);

    // A YAML scalar the way Unity writes a string field
    String YamlString(const String& text);

    // Metas
    String FolderMeta(const String& guid);
    String PrefabMeta(const String& guid);
    String TextMeta(const String& guid);
    String NativeMeta(const String& guid, const String& mainObject);
    String FontMeta(const String& guid);
    String TextureMeta(const String& guid, const V4& border);

    // A Sprite asset over a whole texture with its own borders
    String SpriteAssetYaml(const String& name, const SpriteInfo& source, const V4& border);
}
