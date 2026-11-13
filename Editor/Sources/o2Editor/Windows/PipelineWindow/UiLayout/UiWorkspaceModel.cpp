#include "o2Editor/stdafx.h"
#include "UiWorkspaceModel.h"

#include <chrono>
#include <cmath>
#include <ctime>

#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc::Ws
{
    static const char* const minusSign = "\xE2\x88\x92";  // −
    static const char* const arrow = " \xE2\x86\x92 ";    // →
    static const char* const middleDot = " \xC2\xB7 ";    // ·

    // --- the node's view keys -----------------------------------------------------------------

    String ViewOf(const Json& config, bool hasKit)
    {
        String v = JsStringOr(JsGet(config, "uiView"));
        const Json* raw = JsGet(config, "uiView");
        if (raw && raw->IsString() && (v == "lines" || v == "kit"))
            return v;

        return hasKit ? "kit" : "lines";
    }

    static bool IsElementN(const String& name)
    {
        String lower = name.ToLowerCase();
        if (!lower.StartsWith("element_") || lower.Length() <= 8)
            return false;

        for (int i = 8; i < lower.Length(); i++)
        {
            if (lower[i] < '0' || lower[i] > '9')
                return false;
        }
        return true;
    }

    int KitUndescribed(const UiKit* kit)
    {
        if (!kit)
            return 0;

        int count = 0;
        for (auto& e : kit->elements)
        {
            bool noDescription = !e.description || e.description->IsEmpty();
            if (IsElementN(e.name) || (e.type == "other" && noDescription))
                count++;
        }
        return count;
    }

    String AgentModelOf(const Json& config)
    {
        String model = JsStringOr(JsGet(config, "agentModel")).Trimed(" \t\r\n");
        return model.IsEmpty() ? String("gpt-6.1-sol") : model;
    }

    String SelectedOf(const Json& config, const UiDoc& doc)
    {
        const Json* raw = JsGet(config, "uiSelected");
        if (!raw || !raw->IsString() || raw->Str().IsEmpty())
            return "";

        return FindNode(doc, raw->Str()) ? raw->Str() : String();
    }

    String TabOf(const Json& config)
    {
        const Json* raw = JsGet(config, "uiTab");
        if (raw && raw->IsString())
        {
            const String& v = raw->Str();
            if (v == "agent" || v == "inspect" || v == "text" || v == "history")
                return v;
        }
        return "agent";
    }

    // --- the step queue in the config -----------------------------------------------------------

    Int64 NowMs()
    {
        using namespace std::chrono;
        return (Int64)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }

    bool CommitUser(DataValue& config, const Vector<UiOp>& ops, const String& label, Int64 nowMs, bool fold /*= true*/)
    {
        if (ops.IsEmpty())
            return false;

        auto steps = PushUserAction(StepsOf(config), ops, label, nowMs, fold);
        WriteSteps(config, steps, {});
        return true;
    }

    bool UndoStep(DataValue& config)
    {
        UiUndoResult r;
        if (!UndoAction(StepsOf(config), RedoOf(config), r))
            return false;

        WriteSteps(config, r.steps, r.redo);
        return true;
    }

    bool RedoStep(DataValue& config)
    {
        UiUndoResult r;
        if (!RedoAction(StepsOf(config), RedoOf(config), r))
            return false;

        WriteSteps(config, r.steps, r.redo);
        return true;
    }

    bool RevertTo(DataValue& config, int count)
    {
        auto steps = StepsOf(config);
        if (count < 0 || count >= steps.Count())
            return false;

        Vector<UiStep> kept, redo = RedoOf(config);
        for (int i = 0; i < count; i++)
            kept.Add(steps[i]);
        for (int i = steps.Count() - 1; i >= count; i--)
            redo.Add(steps[i]);

        WriteSteps(config, kept, redo);
        return true;
    }

    void UiTimeline::Update(const Json& config)
    {
        auto steps = StepsOf(config);
        String base = JsStringOr(JsGet(config, "screenName"));

        Vector<String> texts;
        texts.Reserve(steps.Count());
        for (auto& s : steps)
            texts.Add(s.data.Dump());

        // The documents up to the first step that changed stay
        int same = 0;
        if (base == mBaseText && !mDocs.IsEmpty())
        {
            while (same < texts.Count() && same < mStepTexts.Count() && texts[same] == mStepTexts[same])
                same++;
        }
        else
        {
            mDocs.Clear();
            mErrors.Clear();
            mDocs.Add(BaseDoc(config));
            mErrors.Add(Vector<String>());
        }

        while (mDocs.Count() > same + 1)
        {
            mDocs.RemoveAt(mDocs.Count() - 1);
            mErrors.RemoveAt(mErrors.Count() - 1);
        }

        for (int i = same; i < steps.Count(); i++)
        {
            auto r = ApplyOps(mDocs.Last(), steps[i].Ops());
            mDocs.Add(r.doc);
            mErrors.Add(r.errors);
        }

        mSteps = steps;
        mStepTexts = texts;
        mBaseText = base;
    }

    // --- preview screens ------------------------------------------------------------------------

    UiPreviewSetting PreviewSettingOf(const Json& config)
    {
        UiPreviewSetting s;
        const Json* raw = JsGet(config, "uiDeviceCustom");
        const Json* custom = raw && raw->IsObject() ? raw : nullptr;
        auto side = [&](const char* key, double def)
        {
            double v = custom ? JsNumber(JsGet(custom, key)) : NAN;
            if (std::isnan(v) || v == 0.0)
                v = def;
            return std::max(64.0, std::min(8192.0, JsRound(v)));
        };
        s.custom.w = side("w", 1080);
        s.custom.h = side("h", 1920);
        auto dpi = custom ? CleanDpi(JsGet(custom, "dpi")) : std::nullopt;
        s.custom.dpi = dpi ? *dpi : UI_BASE_DPI;

        // Nothing stored: the kit's device. A stored 'design' is the design itself, as it always was
        const Json* device = JsGet(config, "uiDevice");
        s.device = device && device->IsString() && !device->Str().IsEmpty() ? device->Str() : String("kit");
        s.landscape = JsTruthy(JsGet(config, "uiLandscape"));
        return s;
    }

    static UiScreen Turned(const UiScreen& s)
    {
        return UiScreen{ s.h, s.w, s.dpi };
    }

    UiPreview PreviewOf(const UiDoc& doc, const UiPreviewSetting& setting, const UiScreen* reference, const UiTarget& target,
                        bool comparing /*= false*/)
    {
        UiScreen design{ doc.width, doc.height, DocDpi(doc) };
        String device = comparing && reference ? String("reference") : setting.device;
        UiPreview p;
        if (device == "reference" && reference)
        {
            p.id = "reference";
            p.kind = "reference";
            p.screen = *reference;
            p.comparing = comparing && setting.device != "reference";
        }
        else if (device == "design")
        {
            p.id = "design";
            p.kind = "design";
            p.screen = design;
        }
        else if (device == "custom")
        {
            p.id = "custom";
            p.kind = "custom";
            p.screen = setting.landscape ? Turned(setting.custom) : setting.custom;
        }
        else if (const UiDevice* known = DeviceById(device))
        {
            p.id = known->id;
            p.kind = known->kind;
            p.name = known->name;
            p.screen = setting.landscape ? Turned(known->Screen()) : known->Screen();
        }
        else
        {
            p.id = "kit";
            p.kind = "kit";
            p.name = target.name;
            p.screen = setting.landscape ? Turned(target.Screen()) : target.Screen();
            p.fallback = device == "reference";
        }

        p.frame = p.kind == "design" ? UiFrame{ doc.width, doc.height, 1.0 } : PreviewFrame(doc.width, doc.height, doc.dpi, p.screen, "fit");
        return p;
    }

    Vector<String> SoftGraphics(const UiDoc& doc, const UiKit* kit, const UiFrame& frame)
    {
        Vector<String> out;
        if (!kit)
            return out;

        for (auto& e : kit->elements)
        {
            if (GraphicZoom(doc.dpi, e.dpi, frame) > 1.05)
                out.Add(e.name);
        }
        return out;
    }

    String PreviewName(const UiPreview& preview)
    {
        if (preview.kind == "design")
            return "Design";
        if (preview.kind == "reference")
            return "Reference";
        if (preview.kind == "custom")
            return "Custom";
        return preview.name.IsEmpty() ? preview.id : preview.name;
    }

    String ScreenText(const UiScreen& screen)
    {
        return JsNum(screen.w) + "\xC3\x97" + JsNum(screen.h) + middleDot + JsNum(JsRound(screen.dpi)) + " dpi";
    }

    // --- the hierarchy --------------------------------------------------------------------------

    const Vector<String>& AddableWords()
    {
        static const Vector<String> words = { "button", "text", "icon", "image", "panel", "background", "row", "column", "grid",
                                              "group", "progress", "toggle", "slider", "input", "tab", "badge", "divider", "slot" };
        return words;
    }

    Json AddableNode(const String& word, const String& id)
    {
        Json n = Json::Object();
        n["id"] = id;
        auto size = [&](double w, double h) { n["w"] = w; n["h"] = h; };
        if (word == "button") { n["type"] = "button"; size(360, 110); n["text"] = "Button"; }
        else if (word == "text") { n["type"] = "text"; size(480, 64); n["text"] = "Text"; }
        else if (word == "icon") { n["type"] = "icon"; size(96, 96); }
        else if (word == "image") { n["type"] = "image"; size(300, 300); }
        else if (word == "panel") { n["type"] = "panel"; n["anchor"] = "center"; size(760, 560); }
        else if (word == "background") { n["type"] = "background"; n["anchor"] = "stretch"; }
        else if (word == "row") { n["type"] = "group"; n["layout"] = "row"; size(600, 120); n["gap"] = 16; }
        else if (word == "column") { n["type"] = "group"; n["layout"] = "column"; n["anchor"] = "center"; size(600, 500); n["gap"] = 24; }
        else if (word == "grid") { n["type"] = "group"; n["layout"] = "grid"; size(640, 420); n["cols"] = 3; n["gap"] = 16; }
        else if (word == "group") { n["type"] = "group"; size(400, 300); }
        else if (word == "progress") { n["type"] = "progress"; size(480, 44); n["value"] = 0.6; }
        else if (word == "toggle") { n["type"] = "toggle"; size(120, 64); n["on"] = true; }
        else if (word == "slider") { n["type"] = "slider"; size(480, 48); n["value"] = 0.5; }
        else if (word == "input") { n["type"] = "input"; size(560, 96); n["placeholder"] = "Type here"; }
        else if (word == "tab") { n["type"] = "tab"; size(220, 88); n["text"] = "Tab"; }
        else if (word == "badge") { n["type"] = "badge"; size(52, 52); n["text"] = "3"; }
        else if (word == "divider") { n["type"] = "divider"; n["h"] = 6; }
        else if (word == "slot") { n["type"] = "slot"; size(160, 160); }
        else n["type"] = "group";
        return n;
    }

    String TypeLabel(const String& word)
    {
        static const Map<String, String> labels = {
            { "screen", "Screen" }, { "group", "Group" }, { "panel", "Panel" }, { "background", "Background" },
            { "image", "Image" }, { "icon", "Icon" }, { "button", "Button" }, { "text", "Text" },
            { "progress", "Progress bar" }, { "toggle", "Toggle" }, { "slider", "Slider" }, { "input", "Input field" },
            { "tab", "Tab" }, { "badge", "Badge" }, { "divider", "Divider" }, { "slot", "Slot" },
            { "row", "Row" }, { "column", "Column" }, { "grid", "Grid" }
        };
        auto it = labels.find(word);
        return it != labels.end() ? it->second : word;
    }

    String TypeWord(const UiNode& node)
    {
        return node.type == "group" && node.layout ? *node.layout : node.type;
    }

    bool IsLeaf(const UiNode& node)
    {
        static const Vector<String> leaves = { "text", "icon", "image", "divider", "progress", "slider", "toggle", "badge", "input",
                                               "background" };
        return leaves.Contains(node.type);
    }

    bool CanHold(const UiNode& node)
    {
        return node.type == "screen" || !IsLeaf(node);
    }

    bool Holds(const UiNode& node, const String& id)
    {
        if (node.id == id)
            return true;

        for (auto& c : node.children)
        {
            if (Holds(c, id))
                return true;
        }
        return false;
    }

    UiOp AddBlockOp(const UiDoc& doc, const String& selected, const String& word, String& label, String& newId)
    {
        const UiNode* sel = selected.IsEmpty() ? nullptr : FindNode(doc, selected);
        const UiNode* into = &doc.root;
        String after;
        if (sel && IsLeaf(*sel))
        {
            const UiNode* parent = FindParent(doc, sel->id);
            into = parent ? parent : &doc.root;
            after = sel->id;
        }
        else if (sel)
            into = sel;

        int k = 1;
        newId = word + (String)k;
        while (FindNode(doc, newId))
            newId = word + (String)(++k);

        // {op, parent, after?, nodes}: the web's key order
        Json op = Json::Object();
        op["op"] = "add";
        op["parent"] = into->id;
        if (!after.IsEmpty())
            op["after"] = after;
        Json nodes = Json::Array();
        nodes.Add(AddableNode(word, newId));
        op["nodes"] = nodes;
        label = "+ " + newId;
        return op;
    }

    bool ZoneAt(const UiDoc& doc, const String& dragId, const String& targetId, float f, bool targetFolded, UiDropAt& out)
    {
        const UiNode* dragged = FindNode(doc, dragId);
        if (!dragged)
            return false;

        // Below the last row: the end of the screen
        if (targetId.IsEmpty())
        {
            out = UiDropAt{ doc.root.id, "inside" };
            return true;
        }

        const UiNode* target = targetId == doc.root.id ? &doc.root : FindNode(doc, targetId);
        if (!target || Holds(*dragged, target->id))
            return false;

        if (target == &doc.root)
        {
            out = UiDropAt{ target->id, "inside" };
            return true;
        }

        String zone = CanHold(*target) ? (f < 0.3f ? "before" : f > 0.7f ? "after" : "inside") : (f < 0.5f ? "before" : "after");

        // Below an open container is above its first child
        if (zone == "after" && !target->children.empty() && !targetFolded)
        {
            const UiNode& first = target->children[0];
            if (first.id == dragId)
                return false;

            out = UiDropAt{ first.id, "before" };
            return true;
        }

        out = UiDropAt{ target->id, zone };
        return true;
    }

    bool DropOps(const UiDoc& doc, const UiRects& rects, const String& dragId, const UiDropAt& at, Vector<UiOp>& ops,
                 String& label)
    {
        const UiNode* target = at.id == doc.root.id ? &doc.root : FindNode(doc, at.id);
        if (!target)
            return false;

        const UiNode* parent = at.zone == "inside" ? target : FindParent(doc, target->id);
        if (!parent)
            return false;

        ops = ReparentOps(doc, rects, dragId, parent->id, at.zone == "before" ? target->id : String(),
                          at.zone == "after" ? target->id : String());

        // Dropped where it was: nothing to record
        if (NodeToJson(ApplyOps(doc, ops).doc.root).Dump() == NodeToJson(doc.root).Dump())
            return false;

        const UiNode* from = FindParent(doc, dragId);
        bool same = from && from->id == parent->id;
        label = same ? dragId + ": order" : dragId + arrow + (parent == &doc.root ? doc.name : parent->id);
        return true;
    }

    bool RenameOp(const UiDoc& doc, const String& id, const String& name, UiOp& op, String& label)
    {
        String clean = CleanId(name);
        bool isRoot = id == doc.root.id;
        String current = isRoot ? doc.name : id;
        if (clean.IsEmpty() || clean == current)
            return false;

        if (isRoot)
        {
            op = MakeScreenOp(clean, std::nullopt, std::nullopt, std::nullopt);
            label = doc.name + arrow + clean;
            return true;
        }

        if (FindNode(doc, clean))
            return false;

        Json set = Json::Object();
        set["id"] = clean;
        op = MakeUpdateOp(id, set);
        label = id + arrow + clean;
        return true;
    }

    UiOp VisibilityOp(const UiNode& node, String& label)
    {
        bool hidden = !node.IsShown();
        label = node.id + (hidden ? ": Show" : ": Hide");
        if (hidden)
            return MakeUpdateOp(node.id, Json::Object(), { "visible" });

        Json set = Json::Object();
        set["visible"] = false;
        return MakeUpdateOp(node.id, set);
    }

    // --- the inspector -------------------------------------------------------------------------

    UiOp SetFieldOp(const UiNode& node, const String& key, const Json& value, String& label)
    {
        label = node.id + ": " + key;
        bool empty = value.IsNull() || (value.IsString() && value.Str().IsEmpty());
        if (empty)
            return MakeUpdateOp(node.id, Json::Object(), { key });

        Json set = Json::Object();
        set[key] = value;
        return MakeUpdateOp(node.id, set);
    }

    UiOp SetBoxOp(const UiNode& node, const String& key, int side, std::optional<double> value, String& label)
    {
        Box4 box = { 0, 0, 0, 0 };
        if (key == "margin" && node.margin)
            box = *node.margin;
        else if (key == "pad" && node.pad)
            box = *node.pad;

        box[Math::Clamp(side, 0, 3)] = value ? *value : 0.0;
        bool zero = box[0] == 0 && box[1] == 0 && box[2] == 0 && box[3] == 0;
        Json arr = Json::Array();
        for (double v : box)
            arr.Add(v);
        return SetFieldOp(node, key, zero ? Json() : arr, label);
    }

    bool AnchorOp(const UiDoc& doc, const UiRects& rects, const String& id, const String& anchor, bool snap, UiOp& op,
                  String& label)
    {
        const UiNode* n = FindNode(doc, id);
        if (!n)
            return false;

        label = id + ": anchor";
        if (snap)
        {
            Vector<String> unset;
            for (auto key : { "x", "y", "margin" })
            {
                if (n->Has(key))
                    unset.Add(key);
            }
            Json set = Json::Object();
            set["anchor"] = anchor;
            op = MakeUpdateOp(id, set, unset);
            return true;
        }

        const UiNode* parent = FindParent(doc, id);
        const UiRect* r = rects.Find(id);
        const UiRect* p = parent ? rects.Find(parent->id) : nullptr;
        if (r && p)
        {
            Anchor a = ParseAnchor(anchor);
            return PatchOp(id, PlaceAt(*n, *p, *r, &a), op);
        }

        Json set = Json::Object();
        set["anchor"] = anchor;
        op = MakeUpdateOp(id, set);
        return true;
    }

    String AnchorWords(const Anchor& a)
    {
        if (a.h == "stretch" && a.v == "stretch")
            return "Fills the parent";
        if (a.h == "center" && a.v == "middle")
            return "Centre";

        auto vWord = a.v == "top" ? "top" : a.v == "middle" ? "middle" : a.v == "bottom" ? "bottom" : "full height";
        auto hWord = a.h == "left" ? "left" : a.h == "center" ? "centred" : a.h == "right" ? "right" : "full width";
        String s = String(vWord) + ", " + hWord;
        return s.SubStr(0, 1).ToUpperCase() + s.SubStr(1);
    }

    String AnchorColumnName(const String& h)
    {
        return h == "left" ? "left" : h == "center" ? "centre" : h == "right" ? "right" : "\xE2\x86\x94";
    }

    String AnchorRowName(const String& v)
    {
        return v == "top" ? "top" : v == "middle" ? "middle" : v == "bottom" ? "bottom" : "\xE2\x86\x95";
    }

    // --- history and chat ------------------------------------------------------------------------

    static String FirstLine(const String& text)
    {
        int at = text.Find("\n");
        return at < 0 ? text : text.SubStr(0, at);
    }

    String StepTitle(const UiStep& step)
    {
        auto prompt = step.Prompt();
        if (step.IsAgent())
            return prompt && !prompt->IsEmpty() ? *prompt : FirstLine(OpsToText(step.Ops()));

        auto parts = StepParts(step);
        if (parts.Count() > 1)
            return "Hand edits" + String(middleDot) + (String)parts.Count();

        if (!parts.IsEmpty() && !parts[0].label.IsEmpty())
            return parts[0].label;

        return prompt && !prompt->IsEmpty() ? *prompt : FirstLine(OpsToText(step.Ops()));
    }

    String LocalTime(const String& iso)
    {
        double ms = ParseIsoTime(iso);
        if (std::isnan(ms))
            return "";

        std::time_t seconds = (std::time_t)(ms/1000.0);
        std::tm local = *std::localtime(&seconds);
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%02d:%02d", local.tm_hour, local.tm_min);
        return buf;
    }

    String AlignLabel(const String& v)
    {
        return v == "start" ? "Start" : v == "center" ? "Center" : v == "end" ? "End" : v == "stretch" ? "Stretch" : v;
    }

    String JustifyLabel(const String& v)
    {
        return v == "start" ? "From the start" : v == "center" ? "Centered" : v == "end" ? "To the end" :
            v == "between" ? "Space between" : v == "around" ? "Space around" : v == "evenly" ? "Even spacing" : v;
    }

    String FitLabel(const String& v)
    {
        return v == "contain" ? "Contain" : v == "cover" ? "Cover" : v == "stretch" ? "Stretch" : v;
    }
}
