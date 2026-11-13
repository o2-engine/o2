#include "o2Editor/stdafx.h"
#include "UiWsContext.h"

#include "o2Editor/Pipeline/PipelinePngMeta.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeBody.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"

namespace Editor
{
    using UiDoc::Json;
    using UiDoc::UiKit;
    using UiDoc::UiOp;
    using UiDoc::UiScreen;
    using UiDoc::UiKitReport;
    using UiDoc::UI_BASE_DPI;
    using UiDoc::FromDataValue;
    using UiDoc::ParseKit;
    using UiDoc::SolveLayout;
    using UiDoc::KitReport;
    using UiDoc::RedoOf;
    using UiDoc::JsGet;
    using UiDoc::JsStringOr;
    using UiDoc::SetConfigJson;
    namespace Ws = UiDoc::Ws;

    const UiDoc::UiDoc& UiWsContext::Doc() const
    {
        return peek >= 0 && peek < timeline.Count() ? timeline.DocAfter(peek) : current;
    }

    bool UiWsContext::Editable() const
    {
        return peek < 0;
    }

    bool UiWsContext::Busy() const
    {
        return UiAgentState::IsBusy(nodeId);
    }

    const UiDoc::DocDiff& UiWsContext::DiffOf(int step)
    {
        static UiDoc::DocDiff none;
        int count = timeline.GetSteps().Count();
        if (step < 0 || step >= count)
            return none;

        if (mDiffs.Count() != count)
        {
            mDiffs.Clear();
            mDiffReady.Clear();
            mDiffs.Resize(count);
            mDiffReady.Resize(count);
            for (int i = 0; i < count; i++)
                mDiffReady[i] = false;
        }
        if (!mDiffReady[step])
        {
            mDiffs[step] = UiDoc::DiffDocs(timeline.DocAfter(step), timeline.DocAfter(step + 1));
            mDiffReady[step] = true;
        }
        return mDiffs[step];
    }

    const String& UiWsContext::OpsTextOf(int step)
    {
        static String none;
        if (step < 0 || step >= timeline.GetSteps().Count())
            return none;

        if (!mOpsTexts.ContainsKey(step))
            mOpsTexts[step] = UiDoc::OpsToText(timeline.GetSteps()[step].Ops());
        return mOpsTexts[step];
    }

    Ref<PipelineNode> UiWsContext::Node() const
    {
        auto e = editor.Lock();
        auto graph = e ? e->GetGraph() : nullptr;
        return graph ? graph->FindNode(nodeId) : nullptr;
    }

    // ------------------------------------------------------------------------------
    // The first reference picture wired into the node: its bytes, size and density
    // ------------------------------------------------------------------------------
    struct UiWsPicture
    {
        String                png;
        int                   w = 0, h = 0;
        std::optional<double> dpi;

        bool operator==(const UiWsPicture& other) const { return w == other.w && h == other.h && dpi == other.dpi && png == other.png; }
    };

    // The kit value wired into the node's kit input and the first reference picture
    static void ReadInputs(PipelineEditor& e, const Ref<PipelineNode>& node, PipelineValue& kitValue, UiWsPicture& picture)
    {
        kitValue = PipelineValue();
        for (auto& port : node->inputs)
        {
            if (port.portType == PipelinePortType::UiKit)
            {
                kitValue = e.GetInputValueById(node, port.id);
                break;
            }
        }

        picture = UiWsPicture();
        auto graph = e.GetGraph();
        for (auto& port : node->inputs)
        {
            if (port.portType != PipelinePortType::Image)
                continue;

            bool linked = graph && graph->edges.Any([&](const Ref<PipelineEdge>& edge) { return edge->toNodeId == node->id && edge->toPortId == port.id; });
            if (!linked)
                continue;

            PipelineValue value = e.GetInputValueById(node, port.id);
            if (value.IsImage())
            {
                int w = 0, h = 0;
                if (!PipelinePngMeta::Size(value.data, w, h))
                {
                    if (auto bitmap = value.PeekBitmap())
                    {
                        w = bitmap->GetSize().x;
                        h = bitmap->GetSize().y;
                    }
                }
                if (w > 0 && h > 0)
                {
                    picture.w = w;
                    picture.h = h;
                    picture.dpi = PipelinePngMeta::ReadDpi(value.data);
                    picture.png = value.data; // PNG or JPEG as it came (decoded by the compare views)
                }
            }
            break;
        }
    }

    void UiWsContext::Refresh()
    {
        auto e = editor.Lock();
        auto node = Node();
        if (!e || !node)
            return;

        mSeen = node->config;
        mSeenValid = true;

        // The view keys every time; the steps and the redo list only when they changed (they can be large)
        config = Json::Object();
        if (node->config.IsObject())
        {
            for (auto it = node->config.BeginMember(); it != node->config.EndMember(); ++it)
            {
                String name = it->name.GetString();
                if (name != "steps" && name != "uiRedo")
                    config[name] = FromDataValue(it->value);
            }
        }

        const DataValue* stepsValue = node->config.FindMember("steps");
        bool stepsChanged = !mStepsSeenValid || (stepsValue != nullptr) != mStepsPresent || (stepsValue && !(*stepsValue == mStepsSeen)) ||
            JsStringOr(JsGet(config, "screenName")) != mScreenNameSeen;
        if (stepsChanged)
        {
            Json timelineConfig = Json::Object();
            if (const Json* screenName = JsGet(config, "screenName"))
                timelineConfig["screenName"] = *screenName;
            if (stepsValue)
                timelineConfig["steps"] = FromDataValue(*stepsValue);
            timeline.Update(timelineConfig);
            DataValue& seenSteps = mStepsSeen;
            if (stepsValue)
                seenSteps = *stepsValue;
            else
                mStepsSeen.SetNull();
            mStepsPresent = stepsValue != nullptr;
            mStepsSeenValid = true;
            mScreenNameSeen = JsStringOr(JsGet(config, "screenName"));
            mDiffs.Clear();
            mDiffReady.Clear();
            mOpsTexts.Clear();
        }
        current = timeline.DocAfter(timeline.Count() - 1);
        redo = UiDoc::RedoOf(node->config);
        if (peek >= timeline.Count())
            peek = -1;

        PipelineValue kitValue;
        UiWsPicture picture;
        ReadInputs(*e, node, kitValue, picture);
        String text = kitValue.IsValid() && kitValue.type == PipelinePortType::UiKit ? kitValue.data : String();
        if (text != kitText)
        {
            kitText = text;
            kit = text.IsEmpty() ? UiKit() : ParseKit(text);
            kitVersion++;
        }
        hasKit = !text.IsEmpty() && (!kit.elements.IsEmpty() || !kit.textStyles.IsEmpty());
        undescribed = Ws::KitUndescribed(hasKit ? &kit : nullptr);

        const UiDoc::UiDoc& doc = Doc();
        target = Ws::LayoutTarget(hasKit || kit.device ? &kit : nullptr);
        referencePng = picture.png;
        referencePicture = UiDoc::UiSize{ (double)picture.w, (double)picture.h };
        if (picture.w > 0 && picture.h > 0)
            reference = Ws::ReferenceScreenOf(doc, target, picture.w, picture.h, picture.dpi);
        else
            reference.reset();
        ref = Ws::RefSettingOf(config, reference.has_value());
        setting = Ws::PreviewSettingOf(config);
        bool comparing = ref.on && (!compareNeedsBlocks || !doc.root.children.empty());
        preview = Ws::PreviewOf(doc, setting, reference ? &*reference : nullptr, target, comparing);
        rects = SolveLayout(doc, preview.frame.w, preview.frame.h);
        report = hasKit ? KitReport(doc, &kit) : UiKitReport();

        selected = Ws::SelectedOf(config, doc);
        tab = Ws::TabOf(config);
        view = Ws::ViewOf(config, hasKit);
        showIds = JsGet(config, "uiShowIds") && JsGet(config, "uiShowIds")->IsBool() && JsGet(config, "uiShowIds")->Bool();
        const Json* reviewValue = JsGet(config, "agentReview");
        review = !(reviewValue && reviewValue->IsBool() && !reviewValue->Bool());
        draft = JsStringOr(JsGet(config, "uiDraft"));
        points = UiDoc::CleanPoints(JsGet(config, "uiPoints"));
        const Json* exportValue = JsGet(config, "exportFor");
        exportFor = exportValue && exportValue->IsString() && (exportValue->Str() == "o2" || exportValue->Str() == "json") ? exportValue->Str() : String("unity");

        version++;
        if (onRefreshed)
            onRefreshed();
    }

    bool UiWsContext::RefreshIfChanged()
    {
        auto e = editor.Lock();
        auto node = Node();
        if (!e || !node)
            return false;

        bool changed = !mSeenValid || !(node->config == mSeen);
        if (!changed)
        {
            PipelineValue kitValue;
            UiWsPicture picture;
            ReadInputs(*e, node, kitValue, picture);
            String text = kitValue.IsValid() && kitValue.type == PipelinePortType::UiKit ? kitValue.data : String();
            bool sameRef = picture.w == (int)referencePicture.w && picture.h == (int)referencePicture.h && picture.png == referencePng;
            changed = text != kitText || !sameRef;
        }

        if (changed)
            Refresh();
        return changed;
    }

    bool UiWsContext::Write(const String& key, bool undoable, const Function<bool(DataValue&)>& change)
    {
        auto e = editor.Lock();
        auto node = Node();
        if (!e || !node)
            return false;

        auto widget = e->GetNodeWidget(nodeId);
        if (undoable && widget)
            e->OnNodeConfigChanged(widget, key, false);

        bool changed = change(node->config);
        if (widget)
        {
            if (changed)
            {
                e->OnNodeConfigChanged(widget, key, true);
                if (auto body = widget->GetBody())
                    body->OnConfigChanged();
            }
            else if (undoable)
                e->EndContinuousEdit();
        }
        else if (changed)
        {
            if (auto asset = e->GetAsset())
                asset->SetDirty();
        }

        Refresh();
        return changed;
    }

    Vector<UiOp> UiWsContext::WithFirstScreen(const Vector<UiOp>& ops) const
    {
        auto first = Ws::FirstScreenOp(current, target, reference ? &*reference : nullptr);
        if (!first)
            return ops;

        Vector<UiOp> all = { *first };
        all.Add(ops);
        return all;
    }

    void UiWsContext::Op(const Vector<UiOp>& ops, const String& label)
    {
        if (!Editable() || ops.IsEmpty())
            return;

        Vector<UiOp> all = WithFirstScreen(ops);
        Write("steps", true, [&](DataValue& c) { return Ws::CommitUser(c, all, label, Ws::NowMs(), true); });
    }

    void UiWsContext::Gesture(const Vector<UiOp>& ops, const String& label)
    {
        if (!Editable() || ops.IsEmpty())
            return;

        Vector<UiOp> all = WithFirstScreen(ops);
        Write("steps", true, [&](DataValue& c) { return Ws::CommitUser(c, all, label, Ws::NowMs(), false); });
    }

    bool UiWsContext::Bring()
    {
        if (!Editable() || Busy())
            return false;

        double scale = 1;
        auto op = Ws::BringOp(current, target, &scale);
        if (!op)
            return false;

        // The step, then the points (design pixels) scaled with everything else
        Write("steps", true, [&](DataValue& c) { return Ws::CommitUser(c, { *op }, Ws::BringLabel(target), Ws::NowMs(), false); });
        if (!points.IsEmpty())
        {
            Vector<UiDoc::UiStepPoint> moved = points;
            for (auto& p : moved)
            {
                p.x = UiDoc::JsRound(p.x*scale);
                p.y = UiDoc::JsRound(p.y*scale);
            }
            SetUi("uiPoints", UiDoc::PointsToJson(moved));
        }
        return true;
    }

    void UiWsContext::Undo()
    {
        if (!Editable() || Busy())
            return;

        Write("steps", true, [](DataValue& c) { return Ws::UndoStep(c); });
    }

    void UiWsContext::Redo()
    {
        if (!Editable() || Busy())
            return;

        Write("steps", true, [](DataValue& c) { return Ws::RedoStep(c); });
    }

    void UiWsContext::Revert(int count)
    {
        peek = -1;
        Write("steps", true, [count](DataValue& c) { return Ws::RevertTo(c, count); });
    }

    void UiWsContext::Select(const String& id)
    {
        if (id == selected)
            return;

        SetUi("uiSelected", id.IsEmpty() ? Json() : Json(id));
    }

    void UiWsContext::SetPeek(int k)
    {
        peek = k >= 0 && k < timeline.Count() ? k : -1;
        Refresh();
    }

    void UiWsContext::SetUi(const String& key, const Json& value)
    {
        SetUi({ Pair<String, Json>(key, value) });
    }

    void UiWsContext::SetUi(const Vector<Pair<String, Json>>& values)
    {
        if (values.IsEmpty())
            return;

        Write(values[0].first, false, [&](DataValue& c)
        {
            bool changed = false;
            for (auto& kv : values)
            {
                const DataValue* old = c.FindMember(kv.first.Data());
                Json before = old ? FromDataValue(*old) : Json();
                if (old && before.Dump() == kv.second.Dump())
                    continue;

                SetConfigJson(c, kv.first.Data(), kv.second);
                changed = true;
            }
            return changed;
        });
    }

    namespace UiAgentState
    {
        static Map<String, bool>& BusyMap() { static auto* m = new Map<String, bool>(); return *m; }
        static Map<String, String>& Errors() { static auto* m = new Map<String, String>(); return *m; }

        void SetBusy(const String& nodeId, bool busy)
        {
            BusyMap()[nodeId] = busy;
        }

        bool IsBusy(const String& nodeId)
        {
            bool busy = false;
            return BusyMap().TryGetValue(nodeId, busy) && busy;
        }

        void SetError(const String& nodeId, const String& error)
        {
            Errors()[nodeId] = error;
        }

        String GetError(const String& nodeId)
        {
            String error;
            Errors().TryGetValue(nodeId, error);
            return error;
        }

        static Map<String, LiveRun>& Runs() { static auto* m = new Map<String, LiveRun>(); return *m; }
        static Map<String, bool>& Stops() { static auto* m = new Map<String, bool>(); return *m; }

        void SetRun(const String& nodeId, const LiveRun& run)
        {
            LiveRun had;
            bool known = Runs().TryGetValue(nodeId, had);
            LiveRun next = run;
            next.version = known ? had.version + 1 : 1;
            if (!known || had.runId != run.runId)
                Stops().Remove(nodeId);
            Runs()[nodeId] = next;
        }

        bool GetRun(const String& nodeId, LiveRun& run)
        {
            return Runs().TryGetValue(nodeId, run);
        }

        void ClearRun(const String& nodeId)
        {
            Runs().Remove(nodeId);
            Stops().Remove(nodeId);
        }

        void RequestStop(const String& nodeId)
        {
            if (Runs().ContainsKey(nodeId))
                Stops()[nodeId] = true;
        }

        bool IsStopRequested(const String& nodeId)
        {
            bool stop = false;
            return Stops().TryGetValue(nodeId, stop) && stop;
        }
    }
}
