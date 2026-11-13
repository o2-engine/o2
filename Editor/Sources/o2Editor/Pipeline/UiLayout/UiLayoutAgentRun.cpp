#include "o2Editor/stdafx.h"
#include "UiLayoutAgentRun.h"

#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRender.h"
#include "o2Editor/Pipeline/UiDoc/UiDocSolve.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutCompare.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutMarkup.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutOwnAsset.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutSkillBuild.h"

namespace Editor::UiLayout
{
    namespace D = ::Editor::UiDoc;

    // The long side of the whole-screen pictures the agent gets
    static const int SHOT_PX = 1024;

    // The render the comparison and the enlargements are made from
    static const int FULL_PX = 1600;

    // A picture for the model: at most max px on its longer side, PNG
    static String ForModel(const UiImage& img, int max = SHOT_PX)
    {
        return FitInside(img, max).Png();
    }

    // The first line of an error (the providers append their stack as "Cause:"), at most n characters
    static String FirstLine(const String& error, int n)
    {
        int at = error.Find("\nCause:");
        return D::JsSlice(at >= 0 ? error.SubStr(0, at) : error, 0, n);
    }

    // Runs of white space as one space
    static String OneLine(const String& text)
    {
        return D::ReplaceSpaceRuns(text, " ");
    }

    D::UiDoc KeepTurns(const D::UiDoc& doc, const D::UiDoc& draft)
    {
        Map<String, double> turns;
        D::Walk(draft.root, [&](const D::UiNode& n, const D::UiNode*, int) {
            if (n.rotation && *n.rotation)
                turns[n.id] = *n.rotation;
        });
        if (turns.IsEmpty())
            return doc;

        D::UiDoc out = doc;
        std::function<void(D::UiNode&)> put = [&](D::UiNode& n) {
            double t;
            if (&n != &out.root && !n.rotation && turns.TryGetValue(n.id, t))
                n.Set("rotation", Json(t));
            for (auto& c : n.children)
                put(c);
        };
        put(out.root);
        return out;
    }

    Vector<String> CheckFindings(const D::UiDoc& doc, const D::UiKit* kit)
    {
        Vector<String> out;
        if (kit && !kit->elements.IsEmpty())
        {
            D::UiKitReport r = D::KitReport(doc, kit);
            if (!r.unused.IsEmpty() && (!r.missing.IsEmpty() || !r.glyphs.IsEmpty()))
            {
                if (!r.missing.IsEmpty())
                {
                    Vector<String> list;
                    for (int i = 0; i < r.missing.Count() && i < 40; i++)
                        list.Add(r.missing[i].first + " (" + r.missing[i].second + ")");
                    out.Add("Blocks without a kit graphic (drawn dashed): " + D::Join(list, ", "));
                }
                if (!r.glyphs.IsEmpty())
                    out.Add("Blocks that draw an icon with characters: " + D::Join(r.glyphs, ", ") + " \xE2\x80\x94 make each an icon block with its kit graphic");
                Vector<String> unused;
                for (int i = 0; i < r.unused.Count() && i < 60; i++)
                    unused.Add(r.unused[i]);
                out.Add("Kit graphics not used: " + D::Join(unused, ", ") + " \xE2\x80\x94 give a block the graphic made for its role (kit=<name>); where nested blocks rebuild what one kit graphic shows, replace them with one block; leave a block a placeholder only when the kit really has nothing for it");
            }
        }

        D::UiRects rects = D::SolveLayout(doc);
        Vector<String> off;
        for (auto& id : rects.Ids())
        {
            if (id == "root")
                continue;
            D::UiRect r = rects.Get(id);
            double iw = std::min(r.x + r.w, doc.width) - std::max(r.x, 0.0);
            double ih = std::min(r.y + r.h, doc.height) - std::max(r.y, 0.0);
            if (r.w > 0 && r.h > 0 && (iw <= 0 || ih <= 0 || iw*ih < 0.5*r.w*r.h))
                off.Add(id);
        }
        if (!off.IsEmpty())
        {
            while (off.Count() > 20)
                off.PopBack();
            out.Add("Blocks that lie mostly outside the screen: " + D::Join(off, ", "));
        }
        return out;
    }

    // Everything one run keeps between its turns
    struct AgentRunState
    {
        LayoutAgentInput                       a;
        D::UiDoc                               doc;
        std::optional<D::UiKit>                kit;
        int                                    kitVersion = 0;
        Vector<D::UiOp>                        ops;
        Vector<D::UiOwnAsset>                  assets;
        Vector<D::TextStyle>                   styles;   // By name, case aside; a later one replaces an earlier one
        Vector<String>                         errors;
        Vector<String>                         said;
        bool                                   built = false, laid = false;
        int                                    turns = 0, drawn = 0;
        std::optional<Json>                    skillInfo;
        String                                 reply;
        std::optional<double>                  scoreFrom, score;
        String                                 stopped;
        const LayoutAgentRef*                  refScreen = nullptr;
        Vector<LayoutAgentRef>                 pictures;
        Vector<String>                         notes;
        int                                    sheetVersion = -1;
        String                                 sheet;

        // Applies edits to the layout and keeps them
        Vector<String> Apply(const Vector<D::UiOp>& more)
        {
            D::UiApplyResult r = D::ApplyOps(doc, more);
            doc = r.doc;
            ops.Add(more);
            return r.errors;
        }

        // Keeps a style, replacing one of the same name
        void PutStyle(const D::TextStyle& s)
        {
            for (auto& have : styles)
            {
                if (D::JsToLower(have.name) == D::JsToLower(s.name))
                {
                    have = s;
                    return;
                }
            }
            styles.Add(s);
        }

        // The node's kit with what this run added to it
        void WithOwn()
        {
            D::UiKit own;
            own.name = "own";
            for (auto& m : assets)
            {
                String png = PipelineUtils::ReadFileBytes(PipelineUtils::GetUploadPath(m.uploadId));
                if (!png.IsEmpty())
                    own.elements.Add(D::OwnElement(m, PipelineUtils::Base64Encode(png)));
            }
            own.textStyles = styles;
            Vector<const D::UiKit*> parts = { a.kit ? &*a.kit : nullptr };
            if (!own.elements.IsEmpty() || !own.textStyles.IsEmpty())
                parts.Add(&own);
            kit = D::MergeKits(parts);
            kitVersion++;
        }

        // The kit as a picture for the model, made again when the kit changed
        const String& Sheet()
        {
            if (sheetVersion != kitVersion)
            {
                sheetVersion = kitVersion;
                sheet = kit && !kit->elements.IsEmpty() ? ForModel(D::RenderKitSheet(*kit, 60, D::DocDpi(doc)), 1500) : String();
            }
            return sheet;
        }

        // Anything made that is worth a step
        bool Progress() const
        {
            for (auto& op : ops)
            {
                if (D::OpKind(op) != "screen")
                    return true;
            }
            return !assets.IsEmpty();
        }

        bool Cancelled() const { return a.ai->exec && a.ai->exec->IsCancelled(); }
    };

    // The reference read into a draft and laid out by the skill; false with the reason when it failed (cancelled too)
    static Coroutine<bool> Markup(AgentRunState* s, String* error)
    {
        auto& a = s->a;
        const LayoutAgentRef& ref = *s->refScreen;
        Vector<String> hintParts = { a.prompt };
        hintParts.Add(s->notes);
        String hint = D::JsSlice(D::Join(hintParts, "\n"), 0, 4000);

        MarkupInput m;
        m.ai = a.ai;
        m.design = ref.png;
        m.designDpi = ref.dpi;
        m.kit = a.kit ? &*a.kit : nullptr;
        m.hint = hint;
        m.name = s->doc.name.IsEmpty() ? String("Screen") : s->doc.name;
        m.canDraw = a.canDraw;
        m.workDir = a.workDir;
        m.log = a.log;
        MarkupResult res = co_await MarkupReference(m);
        if (!res.ok)
        {
            *error = res.error;
            co_return false;
        }

        s->Apply({ D::MakeDocOp(res.draft.doc) });
        s->assets = res.assets;
        s->styles = res.styles;
        s->kit = res.kit;
        s->kitVersion++;
        s->built = true;
        s->laid = false;

        if (a.skill)
        {
            Vector<D::TextStyle> all = res.kit ? res.kit->textStyles : Vector<D::TextStyle>();
            const D::UiKit* kit = res.kit ? &*res.kit : nullptr;
            SkillBuildInput b;
            b.ai = a.ai;
            b.inputs = LayoutSkillInputs(a.workDir, res.designFlatPng, ref.dpi, kit, all, res.draft.doc);
            b.kit = kit;
            b.styles = all;
            b.designDpi = ref.dpi;
            b.maxTurns = a.maxTurns;
            b.hint = hint;
            b.name = m.name;
            b.workDir = a.workDir;
            b.python = a.python;
            b.design = res.designFlatPng;
            b.w = ref.w;
            b.h = ref.h;
            b.salt = a.runId;
            b.log = a.log;
            SkillBuildResult sk = co_await RunSkillBuild(b);

            const Json& sum = sk.summary;
            s->turns += (int)(sum.Get("turns") ? sum.Get("turns")->Num() : 0);
            Json info = Json::Object();
            info["complete"] = sum.Get("complete") && sum.Get("complete")->Bool();
            info["turns"] = sum.Get("turns") ? sum.Get("turns")->Num() : 0;
            info["checks"] = sum.Get("check_count") ? sum.Get("check_count")->Num() : 0;
            info["targets"] = sum.Get("target_count") ? sum.Get("target_count")->Num() : 0;
            Json open = Json::Array();
            if (const Json* blockers = sum.Get("blockers"))
            {
                for (int i = 0; i < blockers->Count() && i < 12; i++)
                    open.Add(D::JsSlice(blockers->At(i)->IsString() ? blockers->At(i)->Str() : blockers->At(i)->Dump(), 0, 300));
            }
            info["open"] = open;
            s->skillInfo = info;
            if (sk.ok)
            {
                // The skill measures upright boxes and knows no turns: a block it kept takes its turn back from the draft
                s->Apply({ D::MakeDocOp(KeepTurns(sk.doc, res.draft.doc)) });
                for (auto& st : sk.styles)
                    s->PutStyle(st);
                if (!sk.styles.IsEmpty())
                    s->WithOwn();
                s->errors.Add(sk.warnings);
                if (sk.scoreFrom)
                    s->scoreFrom = sk.scoreFrom;
                s->laid = true;
            }
            else if (!sk.cancelled)
                s->errors.Add("The layout pass did not end with a checked layout (" + FirstLine(sk.error, 300) + "); the first draft is kept");
            if (sk.cancelled)
            {
                *error = "Cancelled";
                co_return false;
            }
        }

        String madeStyles;
        for (auto& st : res.styles)
            madeStyles += (madeStyles.IsEmpty() ? "" : ", ") + st.name;
        s->said.Add("The reference was read into " + (String)res.draft.blocks + " blocks: " + (String)res.fromKit + " drawn by kit graphics, " +
                    (String)res.cut + " by graphics cut out of the reference" +
                    (res.bare ? ", " + (String)res.bare + " left without a graphic" : String()) + "." +
                    (res.styles.IsEmpty() ? String() : " Text styles made: " + madeStyles + "."));
        co_return true;
    }

    // A graphic drawn for a request, remembered under the run: a run started again draws nothing twice
    static Coroutine<OwnDrawResult> DrawOnce(AgentRunState* s, OwnDrawContext ctx, D::UiAssetRequest r)
    {
        auto& a = s->a;
        Json keyParts = Json::Array();
        keyParts.Add(a.runId);
        keyParts.Add(a.ai->imageModel);
        keyParts.Add(D::AssetRequestToJson(r));
        String key = "uil-a-" + D::JsSlice(PipelineUtils::Sha256(keyParts.Dump()), 0, 40);
        String hit;
        if (a.ai->memo.get && a.ai->memo.get(key, ".txt", hit))
        {
            Json raw;
            if (Json::Parse(hit, raw))
            {
                if (auto asset = D::CleanOwnAsset(raw))
                {
                    if (o2FileSystem.IsFileExist(PipelineUtils::GetUploadPath(asset->uploadId)))
                    {
                        OwnDrawResult res;
                        res.ok = true;
                        res.asset = *asset;
                        co_return res;
                    }
                }
            }
        }
        OwnDrawResult res = co_await DrawOwnAsset(ctx, r, r.name);
        if (res.ok && a.ai->memo.put)
            a.ai->memo.put(key, ".txt", D::OwnAssetToJson(res.asset).Dump());
        co_return res;
    }

    Coroutine<LayoutAgentResult> RunLayoutAgent(LayoutAgentInput input)
    {
        LayoutAgentResult result;
        double t0 = NowMs();
        AgentRunState state;
        AgentRunState* s = &state;
        s->a = input;
        auto& a = s->a;
        auto& log = a.log;
        String model = a.ai->textModel;
        int textCalls0 = a.ai->textCalls.Load(), imageCalls0 = a.ai->imageCalls.Load(), replayed0 = a.ai->replayed.Load();

        Vector<D::UiAgentReference> references;
        for (auto& r : a.refs)
        {
            D::UiAgentReference ref;
            ref.name = r.name;
            if (r.image)
            {
                ref.kind = "image";
                ref.w = r.w;
                ref.h = r.h;
                s->pictures.Add(r);
            }
            else
            {
                ref.kind = "text";
                ref.text = r.text;
                String note = D::JsTrim(r.text);
                if (!note.IsEmpty())
                    s->notes.Add(note);
            }
            references.Add(ref);
        }
        s->refScreen = s->pictures.IsEmpty() ? nullptr : &s->pictures[0];

        // Graphics are made for what a reference picture shows and the kit lacks, or when no kit is connected at all. A
        // screen described in words with a kit connected is laid out from the kit alone
        bool kitHasGraphics = false;
        if (a.kit)
        {
            for (auto& e : a.kit->elements)
                kitHasGraphics = kitHasGraphics || !e.own;
        }
        bool canDraw = a.canDraw && (s->refScreen || !kitHasGraphics);

        s->doc = a.doc;
        s->kit = a.kit;

        // An empty layout starts in its device's pixels: the reference's area on the device's screen - the reference comes
        // here in those pixels already - or, with nothing to rebuild, the device's whole screen
        std::optional<D::UiOp> startOp;
        if (s->doc.root.children.empty())
        {
            D::UiScreen start;
            if (s->refScreen)
                start = D::UiScreen{ s->refScreen->w, s->refScreen->h, s->refScreen->dpi };
            else
                start = (a.kit && a.kit->device ? *a.kit->device : D::DefaultTarget()).Screen();
            if (start.w != s->doc.width || start.h != s->doc.height || start.dpi != D::DocDpi(s->doc))
            {
                startOp = D::MakeScreenOp("", start.w, start.h, start.dpi);
                s->Apply({ *startOp });
            }
        }

        String failure;
        bool failed = false;
        Vector<String> findings;
        if (s->doc.root.children.empty() && s->refScreen)
        {
            if (!co_await Markup(s, &failure))
                failed = true;
        }

        for (int turn = 1; !failed; turn++)
        {
            if (s->Cancelled())
            {
                failure = "Cancelled";
                failed = true;
                break;
            }
            if (s->laid)
                break;
            int limit = s->built ? std::min(a.maxTurns, D::UI_AGENT_FIX_TURNS) : std::min(a.maxTurns, 2);
            if (turn > limit)
            {
                if (s->built)
                    s->stopped = "budget";
                break;
            }

            int act = log->Begin("turn-" + (String)turn, "turn", "Turn " + (String)turn);
            bool empty = s->doc.root.children.empty();
            const D::UiKit* kit = s->kit ? &*s->kit : nullptr;
            UiImage full = empty ? UiImage() : RenderDocShot(s->doc, kit, FULL_PX);
            UiImage shot = empty ? UiImage() : FitInside(full, SHOT_PX);
            Vector<UiImage> refImages;
            for (auto& r : s->pictures)
            {
                UiImage img;
                UiImage::Decode(r.png, img);
                refImages.Add(FitInside(img, SHOT_PX));
            }

            std::optional<D::UiAgentCompare> compare;
            UiImage zoom;
            bool hasZoom = false;
            if (s->built && s->refScreen && !empty)
            {
                UiImage reference;
                UiImage::Decode(s->refScreen->png, reference);
                ShotCompare cmp = CompareShots(reference, full, s->doc.width, s->doc.height);
                if (!s->scoreFrom)
                    s->scoreFrom = cmp.score;
                s->score = cmp.score;
                double step = GridStep(s->doc.width, s->doc.height);
                hasZoom = SideBySide(reference, full, s->doc.width, s->doc.height, cmp.regions, zoom);
                D::UiAgentCompare c;
                c.score = cmp.score;
                c.gridStep = step;
                for (auto& r : cmp.regions)
                {
                    D::UiAgentCompareRegion region;
                    region.box = D::UiRect(r.box.x, r.box.y, r.box.w, r.box.h);
                    region.share = r.share;
                    region.blocks = BlocksIn(s->doc, r.box);
                    c.regions.Add(region);
                }
                c.zoom = hasZoom;
                compare = c;
                shot = WithGrid(shot, s->doc.width, s->doc.height, step);
                if (!refImages.IsEmpty())
                    refImages[0] = WithGrid(refImages[0], s->doc.width, s->doc.height, step);
                log->SetData(act, "score", cmp.score);
            }
            bool pinned = turn == 1 && !s->built && !empty && !a.points.IsEmpty();
            if (pinned)
            {
                shot = WithPins(shot, a.points, s->doc.width, s->doc.height);
                if (!refImages.IsEmpty())
                    refImages[0] = WithPins(refImages[0], a.points, s->doc.width, s->doc.height);
            }
            if (!empty)
                log->Picture(act, "the screen before this turn", shot);
            if (hasZoom)
                log->Picture(act, "where it differs: reference | now", zoom);
            String sheet = s->Sheet();

            D::UiAgentContext ctx;
            ctx.doc = s->doc;
            ctx.kit = kit;
            ctx.steps = a.steps;
            ctx.prompt = a.prompt;
            ctx.references = references;
            ctx.selected = turn == 1 && !s->built ? a.selected : String();
            ctx.withScreenshot = !empty;
            ctx.withKitSheet = !sheet.IsEmpty();
            if (turn == 1 && !s->built)
                ctx.points = a.points;
            ctx.pinned = pinned;
            ctx.canDraw = canDraw;
            ctx.lang = a.lang;
            D::UiAgentRunState run;
            run.turn = turn;
            run.maxTurns = limit;
            run.log = s->said;
            run.built = s->built;
            run.compare = compare;
            run.findings = findings;
            run.canMarkup = turn == 1 && !s->built && s->refScreen && !empty;
            ctx.run = run;
            String text = D::BuildAgentPrompt(ctx);

            Vector<String> images;
            if (!empty)
                images.Add(shot.Png());
            if (!sheet.IsEmpty())
                images.Add(sheet);
            for (auto& r : refImages)
                images.Add(r.Png());
            if (hasZoom)
                images.Add(zoom.Png());
            a.ai->Log("ui agent turn " + (String)turn + ": " + (String)text.Length() + " chars, " + (String)images.Count() + " picture(s)");

            AiAnswer answerText = co_await AiText(a.ai, text, images, "turn " + (String)turn, a.runId);
            if (!answerText.ok)
            {
                String why = FirstLine(answerText.error, 400);
                Json patch = Json::Object();
                if (s->Cancelled())
                    patch["status"] = "warn";
                else
                {
                    patch["status"] = "error";
                    patch["note"] = why;
                }
                log->End(act, patch);
                // What was made before the failure stays: the step says where it stopped
                if (!s->Progress() || s->Cancelled())
                {
                    failure = s->Cancelled() ? String("Cancelled") : why;
                    failed = true;
                    break;
                }
                s->errors.Add("Turn " + (String)turn + " failed: " + why);
                break;
            }
            if (s->Cancelled())
            {
                log->End(act, Json::Object());
                failure = "Cancelled";
                failed = true;
                break;
            }
            s->turns++;

            D::UiAgentAnswer answer = D::ParseAgentAnswer(answerText.data, s->doc);
            if (answer.markup && turn == 1 && !s->built && s->refScreen)
            {
                Json patch = Json::Object();
                if (!answer.reply.IsEmpty())
                    patch["note"] = answer.reply;
                Json data = Json::Object();
                data["markup"] = 1;
                patch["data"] = data;
                log->End(act, patch);
                if (!co_await Markup(s, &failure))
                {
                    failed = true;
                    break;
                }
                continue;
            }

            // A whole new document keeps the screen's density unless it names one
            for (auto& op : answer.ops)
            {
                if (D::OpKind(op) == "doc" && op.Has("doc") && !D::JsTruthy(D::JsGet(op.Get("doc"), "dpi")) && s->doc.dpi)
                    (*op.Get("doc"))["dpi"] = *s->doc.dpi;
            }
            String before = D::DocToJson(s->doc).Dump();
            s->errors.Add(answer.errors);
            if (!answer.ops.IsEmpty())
                s->Apply(answer.ops);
            for (int i = 0; i < answer.styles.Count() && i < 16; i++)
                s->PutStyle(answer.styles[i]);

            // The graphics it asked for: cut from the reference where it points into it, else drawn
            Vector<D::UiAssetRequest> wanted;
            if (canDraw)
            {
                for (auto& r : answer.assets)
                {
                    if (!D::KitElement(s->kit ? &*s->kit : nullptr, r.name))
                        wanted.Add(r);
                }
            }
            if (!answer.assets.IsEmpty() && !canDraw)
                s->errors.Add((String)answer.assets.Count() + " graphic(s) asked for were not drawn: " +
                              (a.canDraw ? "without a reference picture the screen is laid out from the kit alone" : "drawing is off"));
            int room = std::max(0, D::AGENT_ASSETS_MAX - s->drawn);
            if (wanted.Count() > room)
            {
                Vector<String> over;
                for (int i = room; i < wanted.Count(); i++)
                    over.Add(wanted[i].name);
                s->errors.Add((String)(wanted.Count() - room) + " graphic(s) over the limit of " + (String)D::AGENT_ASSETS_MAX +
                              " were not drawn: " + D::Join(over, ", "));
            }
            Vector<D::UiOwnAsset> made;
            if (room > 0 && !wanted.IsEmpty())
            {
                OwnDrawContext dc;
                dc.exec = a.ai->exec;
                dc.imageModel = a.ai->imageModel;
                dc.layoutDpi = D::DocDpi(s->doc);
                if (s->refScreen)
                {
                    dc.hasReference = true;
                    dc.referencePng = s->refScreen->png;
                    dc.refW = s->refScreen->w;
                    dc.refH = s->refScreen->h;
                    dc.refDpi = s->refScreen->dpi;
                }
                if (!sheet.IsEmpty())
                    dc.styleRefs.Add(sheet);
                if (s->refScreen && !refImages.IsEmpty())
                {
                    UiImage img;
                    UiImage::Decode(s->refScreen->png, img);
                    dc.styleRefs.Add(ForModel(img));
                }
                // One at a time (the web draws three at once)
                for (int i = 0; i < wanted.Count() && i < room && !s->Cancelled(); i++)
                {
                    OwnDrawResult got = co_await DrawOnce(s, dc, wanted[i]);
                    if (got.ok)
                        made.Add(got.asset);
                    else
                        s->errors.Add(wanted[i].name + ": not drawn (" + D::JsSlice(got.error, 0, 200) + ")");
                }
                s->drawn += made.Count();
                s->assets.Add(made);
            }
            if (!answer.styles.IsEmpty() || !made.IsEmpty())
                s->WithOwn();
            if (s->Cancelled())
            {
                log->End(act, Json::Object());
                failure = "Cancelled";
                failed = true;
                break;
            }
            if (!answer.reply.IsEmpty())
                s->reply = answer.reply;
            bool changed = !answer.ops.IsEmpty() && before != D::DocToJson(s->doc).Dump();

            Json patch = Json::Object();
            patch["status"] = answer.errors.IsEmpty() ? "ok" : "warn";
            if (!answer.reply.IsEmpty())
                patch["note"] = answer.reply;
            Json data = log->Act(act).Get("data") ? *log->Act(act).Get("data") : Json::Object();
            data["edits"] = answer.ops.Count();
            String names;
            for (auto& m : made)
                names += (names.IsEmpty() ? "" : ", ") + m.name;
            if (!made.IsEmpty())
            {
                data["made"] = made.Count();
                data["names"] = names;
            }
            if (!answer.styles.IsEmpty())
                data["styles"] = answer.styles.Count();
            if (answer.done)
                data["done"] = 1;
            patch["data"] = data;
            log->End(act, patch);

            Vector<String> notApplied;
            for (int i = 0; i < answer.errors.Count() && i < 6; i++)
                notApplied.Add(answer.errors[i]);
            s->said.Add("Turn " + (String)turn + " (you): " + D::JsSlice(OneLine(answer.reply.IsEmpty() ? String("(no message)") : answer.reply), 0, 400) +
                        (answer.ops.IsEmpty() ? String(" \xE2\x80\x94 no edits") : " \xE2\x80\x94 " + (String)answer.ops.Count() + " edit(s) applied") +
                        (made.IsEmpty() ? String() : ", graphics made: " + names) +
                        (answer.errors.IsEmpty() ? String() : ". Not applied: " + D::JsSlice(D::Join(notApplied, "; "), 0, 500)));
            if (s->built)
            {
                if (answer.done || (!changed && made.IsEmpty()))
                    break;
                continue;
            }

            // An edit: one more turn only when the check of what it made finds something
            if (!a.review || !changed || turn >= limit)
                break;
            findings = CheckFindings(s->doc, s->kit ? &*s->kit : nullptr);
            if (findings.IsEmpty())
                break;
            findings.Add("Fix only what this check found; leave the rest of the layout as it is. Nothing to fix: @done.");
        }

        // How the rebuilt screen ended up against the reference
        if (!failed && s->built && s->refScreen && !s->doc.root.children.empty())
        {
            int act = log->Begin("check", "check", "Result against the reference");
            UiImage reference;
            UiImage::Decode(s->refScreen->png, reference);
            UiImage full = RenderDocShot(s->doc, s->kit ? &*s->kit : nullptr, FULL_PX);
            ShotCompare cmp = CompareShots(reference, full, s->doc.width, s->doc.height);
            if (!s->scoreFrom)
                s->scoreFrom = cmp.score;
            s->score = cmp.score;
            log->Picture(act, "the screen", FitInside(full, SHOT_PX));
            UiImage zoom;
            if (SideBySide(reference, full, s->doc.width, s->doc.height, cmp.regions, zoom))
                log->Picture(act, "what still differs: reference | now", zoom);
            Json patch = Json::Object();
            Json data = Json::Object();
            data["score"] = cmp.score;
            data["from"] = *s->scoreFrom;
            patch["data"] = data;
            log->End(act, patch);
        }

        if (failed)
        {
            // Stopped by the user with something made: what there is becomes the step
            if (!(failure == "Cancelled" && s->Progress()))
            {
                log->Close("error");
                result.error = failure.IsEmpty() ? String("the agent failed") : failure;
                co_return result;
            }
            s->stopped = "cancelled";
        }
        log->Close(s->stopped == "cancelled" ? "warn" : "ok");

        // A rebuilt screen is stored as the screen it ended up as, not as a draft and the edits after it. The screen an empty
        // layout was set to is part of what the agent made: with nothing made, nothing changes
        bool anyDoc = false;
        for (auto& op : s->ops)
            anyDoc = anyDoc || D::OpKind(op) == "doc";
        Vector<D::UiOp> finalOps;
        if (anyDoc)
            finalOps.Add(D::MakeDocOp(s->doc));
        else if (!(s->ops.Count() == 1 && startOp))
            finalOps = s->ops;

        int reused = 0;
        const D::UiKit* finalKit = s->kit ? &*s->kit : nullptr;
        D::Walk(s->doc.root, [&](const D::UiNode& n, const D::UiNode*, int)
        {
            if (n.type == "screen" || n.type == "group" || n.type == "text")
                return;
            auto el = D::SkinOf(n, finalKit).el;
            if (el && !el->own)
                reused++;
        });

        Json run = Json::Object();
        run["id"] = a.runId;
        run["ms"] = JsRound(NowMs() - t0);
        run["turns"] = s->turns;
        run["acts"] = log->GetActs();
        if (s->built)
        {
            run["built"] = true;
            run["reused"] = reused;
            run["made"] = s->assets.Count();
        }
        else if (!s->assets.IsEmpty())
            run["made"] = s->assets.Count();
        if (s->score)
        {
            run["score"] = *s->score;
            if (s->scoreFrom)
                run["scoreFrom"] = *s->scoreFrom;
        }
        if (!s->stopped.IsEmpty())
            run["stopped"] = s->stopped;
        if (s->skillInfo)
            run["skill"] = *s->skillInfo;

        // The layout pass says nothing to the user itself: the step says what it came to
        if (s->skillInfo && s->reply.IsEmpty())
        {
            const Json& sk = *s->skillInfo;
            bool complete = sk.Get("complete") && sk.Get("complete")->Bool();
            s->reply = "Rebuilt from the reference with the UI kit: " + (String)reused + " block(s) drawn by kit graphics, " + (String)s->assets.Count() +
                " graphic(s) cut out of the reference. " + sk.Get("turns")->ToJsString() + " turn(s), " + sk.Get("checks")->ToJsString() +
                " check(s) against the design, " + sk.Get("targets")->ToJsString() + " element(s) compared." +
                (complete ? String() : " Not everything was confirmed: " + (String)sk.Get("open")->Count() + " open issue(s).");
        }

        Int64 now = a.nowMs ? a.nowMs : D::NowMs();
        D::UiStep step = D::MakeAgentStep(a.prompt, !s->reply.IsEmpty() ? s->reply : finalOps.IsEmpty() ? String() : String("Done."), finalOps, now, model);
        step.data["id"] = a.runId;
        step.data["run"] = run;
        if (!a.points.IsEmpty())
            step.data["points"] = D::PointsToJson(a.points);
        if (!s->styles.IsEmpty())
        {
            Json list = Json::Array();
            for (auto& st : s->styles)
                list.Add(D::TextStyleToJson(st));
            step.data["styles"] = list;
        }
        if (!s->assets.IsEmpty())
        {
            Json list = Json::Array();
            for (auto& m : s->assets)
                list.Add(D::OwnAssetToJson(m));
            step.data["assets"] = list;
        }
        a.ai->Log("ui agent run " + a.runId + ": " + (String)s->turns + " turn(s), " + (String)(a.ai->textCalls.Load() - textCalls0) + " text + " +
                  (String)(a.ai->imageCalls.Load() - imageCalls0) + " image call(s), " + (String)(a.ai->replayed.Load() - replayed0) +
                  " replayed, " + run["ms"].ToJsString() + " ms");
        result.ok = true;
        result.step = step;
        result.errors = s->errors;
        co_return result;
    }
}
