#include "o2Editor/stdafx.h"
#include "UiDocOps.h"

#include <cmath>

#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

// The node's steps: reading and writing them, the layout after some of them, what a step changed, and the hand edits that
// make one step of many actions (undo / redo by action)
namespace Editor::UiDoc
{
    // The string value of a member, "" when it is no string
    static String StrOf(const Json& obj, const char* key)
    {
        const Json* v = JsGet(obj, key);
        return v && v->IsString() ? v->Str() : String();
    }

    // --- steps ----------------------------------------------------------------------------------------------

    String UiStep::Id() const { return StrOf(data, "id"); }
    String UiStep::By() const { return StrOf(data, "by"); }
    String UiStep::At() const { return StrOf(data, "at"); }

    static std::optional<String> OptionalText(const Json& obj, const char* key)
    {
        const Json* v = JsGet(obj, key);
        if (JsNullish(v))
            return std::nullopt;
        return v->ToJsString();
    }

    std::optional<String> UiStep::Prompt() const { return OptionalText(data, "prompt"); }
    std::optional<String> UiStep::Reply() const { return OptionalText(data, "reply"); }
    std::optional<String> UiStep::Model() const { return OptionalText(data, "model"); }
    std::optional<String> UiStep::PartOf() const { return OptionalText(data, "partOf"); }

    const Vector<UiOp>& UiStep::Ops() const
    {
        static const Vector<UiOp> none;
        const Json* ops = JsGet(data, "ops");
        return ops && ops->IsArray() ? ops->Items() : none;
    }

    bool UiStep::Review(String& reply, double& edits) const
    {
        const Json* review = JsGet(data, "review");
        if (!review || !review->IsObject())
            return false;
        reply = JsStringOr(JsGet(review, "reply"));
        edits = JsNumber(JsGet(review, "edits"));
        return true;
    }

    void UiStep::SetReview(const String& reply, double edits)
    {
        Json review = Json::Object();
        review["reply"] = reply;
        review["edits"] = edits;
        data["review"] = review;
    }

    static Json OpsArray(const Vector<UiOp>& ops)
    {
        Json out = Json::Array();
        for (auto& op : ops)
            out.Add(op);
        return out;
    }

    UiStep MakeAgentStep(const String& prompt, const String& reply, const Vector<UiOp>& ops, Int64 nowMs, const String& model)
    {
        UiStep step;
        step.data["id"] = NewStepId(nowMs);
        step.data["by"] = "agent";
        step.data["prompt"] = prompt;
        step.data["reply"] = reply;
        step.data["ops"] = OpsArray(ops);
        step.data["at"] = IsoTime((double)nowMs);
        step.data["model"] = model;
        return step;
    }

    const Vector<String>& UiLayoutUiOnly()
    {
        static const Vector<String> keys = { "uiView", "uiPreview", "uiSelected", "uiWorkspace", "uiTab", "uiRedo", "uiDraft",
                                             "uiShowIds", "agentModel", "uiDevice", "uiDeviceCustom", "uiLandscape", "uiScale" };
        return keys;
    }

    static Vector<UiStep> StepList(const Json* v)
    {
        Vector<UiStep> out;
        if (!v || !v->IsArray())
            return out;
        for (auto& s : v->Items())
        {
            const Json* ops = JsGet(s, "ops");
            if (s.IsObject() && ops && ops->IsArray())
                out.Add(UiStep(s));
        }
        return out;
    }

    Vector<UiStep> StepsOf(const Json& config) { return StepList(JsGet(config, "steps")); }
    Vector<UiStep> RedoOf(const Json& config) { return StepList(JsGet(config, "uiRedo")); }

    Json StepsToJson(const Vector<UiStep>& steps)
    {
        Json out = Json::Array();
        for (auto& s : steps)
            out.Add(s.data);
        return out;
    }

    UiDoc BaseDoc(const Json& config)
    {
        UiDoc doc = EmptyDoc();
        String name = CleanId(JsGet(config, "screenName"));
        if (!name.IsEmpty())
            doc.name = name;
        return doc;
    }

    UiApplyResult DocAtSteps(const Json& config, const Vector<UiStep>& steps, std::optional<int> count /*= std::nullopt*/)
    {
        int len = steps.Count();
        int end = count ? (*count < 0 ? std::max(0, len + *count) : std::min(len, *count)) : len;
        UiApplyResult result;
        result.doc = BaseDoc(config);
        for (int i = 0; i < end; i++)
        {
            for (auto& op : steps[i].Ops())
            {
                String error;
                if (!ApplyOp(result.doc, op, error))
                    result.errors.Add(error);
            }
        }
        return result;
    }

    UiApplyResult DocAt(const Json& config, std::optional<int> count /*= std::nullopt*/)
    {
        return DocAtSteps(config, StepsOf(config), count);
    }

    // --- what a step changed --------------------------------------------------------------------------------

    namespace
    {
        // A block's own fields (as JSON text), its parent and its place
        struct FlatEntry
        {
            String node, parent;
            int    index = 0;
        };

        // id -> entry in walk order (a later duplicate replaces the value in place, as Map#set does)
        struct FlatMap
        {
            Vector<String>        ids;
            Map<String, FlatEntry> entries;
        };

        FlatMap Flat(const UiDoc& doc)
        {
            FlatMap out;
            Walk(doc.root, [&](const UiNode& n, const UiNode* p, int) {
                int index = 0;
                if (p)
                {
                    index = -1;
                    for (int i = 0; i < (int)p->children.size(); i++)
                    {
                        if (p->children[i].id == n.id)
                        {
                            index = i;
                            break;
                        }
                    }
                }
                if (out.entries.find(n.id) == out.entries.end())
                    out.ids.Add(n.id);
                out.entries[n.id] = FlatEntry{ NodeToJson(n, false).Dump(), p ? p->id : String(), index };
            });
            return out;
        }
    }

    DocDiff DiffDocs(const UiDoc& a, const UiDoc& b)
    {
        FlatMap fa = Flat(a), fb = Flat(b);
        DocDiff d;
        Vector<String> moved;
        for (auto& id : fb.ids)
        {
            auto old = fa.entries.find(id);
            const FlatEntry& v = fb.entries[id];
            if (old == fa.entries.end())
            {
                d.added.Add(id);
                continue;
            }
            if (old->second.node != v.node)
                d.changed.Add(id);
            if (old->second.parent != v.parent || old->second.index != v.index)
                moved.Add(id);
        }
        for (auto& id : fa.ids)
        {
            if (fb.entries.find(id) == fb.entries.end())
                d.removed.Add(id);
        }
        // A sibling shifted by an insert above it is not "moved"
        for (auto& id : moved)
        {
            if (fa.entries[id].parent != fb.entries[id].parent)
                d.moved.Add(id);
        }
        d.screen = a.name != b.name || a.width != b.width || a.height != b.height || (a.dpi ? *a.dpi : 0) != (b.dpi ? *b.dpi : 0);
        return d;
    }

    String DiffSummary(const DocDiff& d)
    {
        Vector<String> parts;
        if (!d.added.IsEmpty()) parts.Add("+" + JsNum(d.added.Count()));
        if (!d.changed.IsEmpty()) parts.Add("~" + JsNum(d.changed.Count()));
        if (!d.moved.IsEmpty()) parts.Add("\xE2\x86\x95" + JsNum(d.moved.Count()));
        if (!d.removed.IsEmpty()) parts.Add("\xE2\x88\x92" + JsNum(d.removed.Count()));
        if (d.screen) parts.Add("screen");
        return parts.IsEmpty() ? String("\xE2\x80\x94") : Join(parts, " ");
    }

    String NewStepId(Int64 nowMs /*= 0*/)
    {
        return "s" + ToBase36(nowMs ? nowMs : NowMs()) + RandomBase36(4);
    }

    // --- hand edits: one step, many actions -------------------------------------------------------------------

    // The step's actions as stored ({label, n, at} objects), or the one action the whole step is
    static Vector<Json> PartsJson(const UiStep& s)
    {
        const Json* p = JsGet(s.data, "parts");
        int opsCount = s.Ops().Count();
        if (p && p->IsArray() && p->Count() > 0)
        {
            bool valid = true;
            double sum = 0;
            for (auto& x : p->Items())
            {
                const Json* n = JsGet(x, "n");
                if (!JsTruthy(&x) || !n || !n->IsNumber() || n->Num() != std::floor(n->Num()) || !std::isfinite(n->Num()) ||
                    n->Num() <= 0)
                {
                    valid = false;
                    break;
                }
                sum += n->Num();
            }
            if (valid && sum == opsCount)
                return p->Items();
        }

        Json part = Json::Object();
        const Json* prompt = JsGet(s.data, "prompt");
        part["label"] = JsNullish(prompt) ? Json(String()) : *prompt;
        part["n"] = opsCount;
        if (const Json* at = JsGet(s.data, "at"))
            part["at"] = *at;
        return { part };
    }

    Vector<UiStepPart> StepParts(const UiStep& step)
    {
        Vector<UiStepPart> out;
        for (auto& p : PartsJson(step))
            out.Add(UiStepPart{ JsStringOr(JsGet(p, "label")), JsNumber(JsGet(p, "n")), JsStringOr(JsGet(p, "at")) });
        return out;
    }

    String UserStepText(const UiStep& step)
    {
        Vector<String> labels;
        for (auto& p : PartsJson(step))
        {
            const Json* label = JsGet(p, "label");
            if (JsTruthy(label))
                labels.Add(label->ToJsString());
        }
        return Join(labels, "; ");
    }

    // Date.parse of a member (NaN when it is no date)
    static double TimeOf(const Json& obj, const char* key)
    {
        const Json* v = JsGet(obj, key);
        return v && v->IsString() ? ParseIsoTime(v->Str()) : NAN;
    }

    // Sets or (for a missing value: undefined) removes a member
    static void SetOrRemove(Json& obj, const char* key, const Json* value)
    {
        if (value)
            obj[key] = *value;
        else
            obj.Remove(key);
    }

    static Json PartJson(const String& label, double n, const String& at)
    {
        Json part = Json::Object();
        part["label"] = label;
        part["n"] = n;
        part["at"] = at;
        return part;
    }

    // Two updates of one block as one
    static UiOp MergeUpdates(const UiOp& prev, const UiOp& one)
    {
        Json set = Json::Object();
        for (const Json* src : { JsGet(prev, "set"), JsGet(one, "set") })
        {
            if (src && src->IsObject())
            {
                for (auto& m : src->Members())
                    set[m.first] = m.second;
            }
        }
        for (auto& k : JsonToStrings(JsGet(one, "unset")))
            set.Remove(k);

        Vector<String> unset;
        for (const Json* src : { JsGet(prev, "unset"), JsGet(one, "unset") })
        {
            for (auto& k : JsonToStrings(src))
            {
                if (!unset.Contains(k))
                    unset.Add(k);
            }
        }
        unset.RemoveAll([&](const String& k) { return set.Has(k); });

        Json op = Json::Object();
        op["op"] = "update";
        if (const Json* id = JsGet(prev, "id"))
            op["id"] = *id;
        if (set.Count() > 0)
            op["set"] = set;
        if (!unset.IsEmpty())
            op["unset"] = JsonStrings(unset);
        return op;
    }

    Vector<UiStep> PushUserAction(const Vector<UiStep>& steps, const Vector<UiOp>& ops, const String& label, Int64 nowMs,
                                  bool fold /*= true*/)
    {
        if (ops.IsEmpty())
            return steps;

        double now = (double)nowMs;
        String at = IsoTime(now);
        if (!steps.IsEmpty() && steps.Last().By() == "user" && now - TimeOf(steps.Last().data, "at") < HAND_STEP_IDLE_MS)
        {
            const UiStep& last = steps.Last();
            Vector<Json> parts = PartsJson(last);
            const Json& tail = parts.Last();
            const Vector<UiOp>& lastOps = last.Ops();
            const Json* tailN = JsGet(tail, "n");
            const UiOp* prev = tailN && tailN->IsNumber() && tailN->Num() == 1 && !lastOps.IsEmpty() ? &lastOps.Last() : nullptr;
            const UiOp* one = ops.Count() == 1 && OpKind(ops[0]) == "update" ? &ops[0] : nullptr;
            const Json* tailLabel = JsGet(tail, "label");

            Vector<UiStep> out = steps;
            out.PopBack();
            UiStep next = last;
            next.data["at"] = at;

            if (fold && one && prev && OpKind(*prev) == "update" && JsStrictEqual(JsGet(*prev, "id"), JsGet(*one, "id")) &&
                tailLabel && tailLabel->IsString() && tailLabel->Str() == label && !JsTruthy(JsGet(JsGet(*one, "set"), "id")) &&
                now - TimeOf(tail, "at") < HAND_FOLD_MS)
            {
                Vector<UiOp> nextOps = lastOps;
                nextOps.Last() = MergeUpdates(*prev, *one);
                next.data["ops"] = OpsArray(nextOps);
                Json nextTail = tail;
                nextTail["at"] = at;
                Json nextParts = Json::Array();
                for (int i = 0; i + 1 < parts.Count(); i++)
                    nextParts.Add(parts[i]);
                nextParts.Add(nextTail);
                next.data["parts"] = nextParts;
                out.Add(next);
                return out;
            }

            Vector<UiOp> nextOps = lastOps;
            nextOps.Add(ops);
            next.data["ops"] = OpsArray(nextOps);
            Json nextParts = Json::Array();
            for (auto& p : parts)
                nextParts.Add(p);
            nextParts.Add(PartJson(label, ops.Count(), at));
            next.data["parts"] = nextParts;
            out.Add(next);
            return out;
        }

        UiStep step;
        step.data["id"] = NewStepId(nowMs);
        step.data["by"] = "user";
        step.data["prompt"] = label;
        step.data["ops"] = OpsArray(ops);
        step.data["at"] = at;
        Json parts = Json::Array();
        parts.Add(PartJson(label, ops.Count(), at));
        step.data["parts"] = parts;

        Vector<UiStep> out = steps;
        out.Add(step);
        return out;
    }

    bool UndoAction(const Vector<UiStep>& stepsIn, const Vector<UiStep>& redoIn, UiUndoResult& out)
    {
        // Copies: out may be where the lists come from
        const Vector<UiStep> steps = stepsIn, redo = redoIn;
        if (steps.IsEmpty())
            return false;

        const UiStep& last = steps.Last();
        Vector<Json> parts = last.By() == "user" ? PartsJson(last) : Vector<Json>();
        if (parts.Count() > 1)
        {
            const Json tail = parts.Last();
            Vector<Json> rest = parts;
            rest.PopBack();
            const Vector<UiOp>& lastOps = last.Ops();
            int cut = lastOps.Count() - (int)JsNumber(JsGet(tail, "n"));

            UiStep kept = last;
            Vector<UiOp> keptOps, undoneOps;
            for (int i = 0; i < lastOps.Count(); i++)
                (i < cut ? keptOps : undoneOps).Add(lastOps[i]);
            kept.data["ops"] = OpsArray(keptOps);
            Json restJson = Json::Array();
            for (auto& p : rest)
                restJson.Add(p);
            kept.data["parts"] = restJson;
            SetOrRemove(kept.data, "at", JsGet(rest.Last(), "at"));

            UiStep undone;
            undone.data["id"] = NewStepId();
            undone.data["by"] = "user";
            SetOrRemove(undone.data, "prompt", JsGet(tail, "label"));
            undone.data["ops"] = OpsArray(undoneOps);
            SetOrRemove(undone.data, "at", JsGet(tail, "at"));
            Json tailParts = Json::Array();
            tailParts.Add(tail);
            undone.data["parts"] = tailParts;
            SetOrRemove(undone.data, "partOf", JsGet(last.data, "id"));

            out.steps = steps;
            out.steps.Last() = kept;
            out.redo = redo;
            out.redo.Add(undone);
            return true;
        }

        out.steps = steps;
        out.steps.PopBack();
        out.redo = redo;
        out.redo.Add(last);
        return true;
    }

    bool RedoAction(const Vector<UiStep>& stepsIn, const Vector<UiStep>& redoIn, UiUndoResult& out)
    {
        const Vector<UiStep> steps = stepsIn, redo = redoIn;
        if (redo.IsEmpty())
            return false;

        const UiStep& r = redo.Last();
        const Json* partOf = JsGet(r.data, "partOf");
        out.redo = redo;
        out.redo.PopBack();

        if (JsTruthy(partOf) && !steps.IsEmpty() && JsStrictEqual(JsGet(steps.Last().data, "id"), partOf) &&
            steps.Last().By() == "user")
        {
            const UiStep& last = steps.Last();
            UiStep merged = last;
            Vector<UiOp> ops = last.Ops();
            ops.Add(r.Ops());
            merged.data["ops"] = OpsArray(ops);
            Json parts = Json::Array();
            for (auto& p : PartsJson(last))
                parts.Add(p);
            for (auto& p : PartsJson(r))
                parts.Add(p);
            merged.data["parts"] = parts;
            SetOrRemove(merged.data, "at", JsGet(r.data, "at"));

            out.steps = steps;
            out.steps.Last() = merged;
            return true;
        }

        UiStep plain = r;
        plain.data.Remove("partOf");
        out.steps = steps;
        out.steps.Add(plain);
        return true;
    }
}

