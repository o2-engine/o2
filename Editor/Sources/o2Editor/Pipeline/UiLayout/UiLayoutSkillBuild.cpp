#include "o2Editor/stdafx.h"
#include "UiLayoutSkillBuild.h"

#include <filesystem>
#include <regex>

#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFonts.h"
#include "o2Editor/Pipeline/UiDoc/UiDocStructure.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutCompare.h"

namespace Editor::UiLayout
{
    namespace D = ::Editor::UiDoc;
    namespace fs = std::filesystem;

    static String LowerAscii(const String& s)
    {
        String out = s;
        for (int i = 0; i < out.Length(); i++)
        {
            if (out[i] >= 'A' && out[i] <= 'Z')
                out[i] = (char)(out[i] - 'A' + 'a');
        }
        return out;
    }

    // True when the text starts with the prefix in any letter case
    static bool StartsWithNoCase(const String& text, const String& prefix)
    {
        return text.Length() >= prefix.Length() && LowerAscii(text.SubStr(0, prefix.Length())) == LowerAscii(prefix);
    }

    // The picture of a kit graphic: its PNG, else its upload (an own graphic)
    static String KitPicture(const D::UiKitElement& el)
    {
        if (!el.png.IsEmpty())
            return PipelineUtils::Base64Decode(el.png);
        if (!el.uploadId.IsEmpty())
            return PipelineUtils::ReadFileBytes(PipelineUtils::GetUploadPath(el.uploadId));
        return String();
    }

    String WriteKitSprite(const String& dir, const D::UiKitElement& el, double designDpi)
    {
        String png = KitPicture(el);
        if (png.IsEmpty())
            return String();

        String file = D::KitFileName(el);
        double k = designDpi/D::GraphicDpi(el.dpi);
        std::optional<D::UiNine> nine = el.assetType == "sliced" ? el.nineSlice : std::nullopt;
        if (std::abs(k - 1) > 0.02)
        {
            UiImage img;
            if (UiImage::Decode(png, img))
                png = img.Resize(std::max(1, (int)JsRound(el.width*k)), std::max(1, (int)JsRound(el.height*k))).Png();
            if (nine)
                nine = D::UiNine{ JsRound(nine->left*k), JsRound(nine->top*k), JsRound(nine->right*k), JsRound(nine->bottom*k) };
        }
        PipelineUtils::WriteFileBytes(dir + "/" + file, png);
        if (nine && (nine->left || nine->top || nine->right || nine->bottom))
        {
            PipelineUtils::WriteFileBytes(dir + "/" + file + ".meta", "fileFormatVersion: 2\nTextureImporter:\n  spriteBorder: {x: " +
                Json::FormatNumber(nine->left) + ", y: " + Json::FormatNumber(nine->bottom) + ", z: " + Json::FormatNumber(nine->right) +
                ", w: " + Json::FormatNumber(nine->top) + "}\n");
        }
        return file;
    }

    SkillInputs LayoutSkillInputs(const String& workDir, const String& designFlatPng, double designDpi, const D::UiKit* kit,
                                  const Vector<D::TextStyle>& styles, const D::UiDoc& draft)
    {
        SkillInputs in;
        in.dirs = MakeWorkDirs(workDir);
        const WorkDirs& d = in.dirs;
        std::error_code ec;
        for (auto& dir : { d.input, d.assets, d.fonts, d.task, d.scratch, d.work + "/rounds", d.work + "/edits", d.work + "/rounds.json" })
            fs::remove_all(fs::path((const char*)dir.Data()), ec);
        for (auto& dir : { d.input, d.assets, d.fonts })
            fs::create_directories(fs::path((const char*)dir.Data()), ec);
        PipelineUtils::WriteFileBytes(d.design, designFlatPng);

        if (kit)
        {
            for (auto& el : kit->elements)
            {
                String file = WriteKitSprite(d.assets, el, designDpi);
                if (file.IsEmpty())
                    continue;
                in.assetNames[LowerAscii(file)] = el.name;
                (el.own ? in.ownFiles : in.kitFiles).Add(file);
            }
        }

        Vector<String> fonts = D::FontFilesOf(styles);
        for (auto& f : { D::FaceOf("Roboto", 400).file, D::FaceOf("Roboto", 700).file })
        {
            if (!fonts.Contains(f))
                fonts.Add(f);
        }
        String home = D::UiFontsHome();
        for (auto& f : fonts)
        {
            String bytes = PipelineUtils::ReadFileBytes(home + "/" + f);
            if (bytes.IsEmpty())
                continue;
            PipelineUtils::WriteFileBytes(d.fonts + "/" + f, bytes);
            PipelineUtils::WriteFileBytes(d.input + "/" + f, bytes);
            in.fonts.Add(f);
        }
        in.seed = SeedStructure(draft, kit, designDpi);
        return in;
    }

    String SeedStructure(const D::UiDoc& draft, const D::UiKit* kit, double designDpi)
    {
        D::UiStructureResult made = D::ToUiStructure(draft, kit);
        Json st = made.structure;
        Map<String, double> scale;
        if (kit)
        {
            for (auto& el : kit->elements)
                scale[LowerAscii(D::KitFileName(el))] = designDpi/D::GraphicDpi(el.dpi);
        }
        std::function<void(Json&)> walk = [&](Json& n)
        {
            double k = 1;
            if (const Json* asset = n.Get("asset"))
            {
                if (asset->IsString())
                    scale.TryGetValue(LowerAscii(asset->Str()), k);
            }
            Json* nine = n.Get("nineSlice");
            if (nine && nine->IsObject() && std::abs(k - 1) > 0.02)
            {
                for (auto key : { "left", "top", "right", "bottom" })
                {
                    if (Json* v = nine->Get(key))
                        *v = JsRound(v->Num()*k);
                }
            }
            Json* font = n.Get("fontFamily");
            if (font && font->IsString() && !font->Str().Contains("/"))
                *font = "fonts/" + font->Str();
            if (Json* kids = n.Get("children"))
            {
                for (auto& c : kids->Items())
                    walk(c);
            }
        };
        if (Json* root = st.Get("root"))
            walk(*root);
        st["metadata"]["notes"] = "First draft: a markup of the design in terms of the UI kit.";
        return st.Dump(2);
    }

    String SkillNote(const SkillInputs& inp, const D::UiKit* kit, const Vector<D::TextStyle>& styles)
    {
        Vector<String> purposes;
        if (kit)
        {
            for (auto& e : kit->elements)
            {
                String what = e.purpose && !e.purpose->IsEmpty() ? *e.purpose : e.description ? *e.description : String();
                if (e.own || what.IsEmpty())
                    continue;
                purposes.Add("`" + D::KitFileName(e) + "` \xE2\x80\x94 " + e.type + (e.assetType == "sliced" ? ", nine-sliced" : "") + ": " +
                             D::JsSlice(what, 0, 120));
            }
        }
        String own;
        for (int i = 0; i < inp.ownFiles.Count(); i++)
            own += (i ? ", " : "") + ("`" + inp.ownFiles[i] + "`");
        String styleList;
        for (int i = 0; i < styles.Count(); i++)
        {
            auto& s = styles[i];
            styleList += (i ? "; " : "") + s.name + " = fonts/" + D::FaceOf(s.font, s.weight).file + " " + s.color +
                (s.stroke ? " stroke " + s.stroke->color + " " + Json::FormatNumber(s.stroke->width) + "px at size " + Json::FormatNumber(s.size) : String());
        }

        Vector<String> lines = {
            "The sliced-asset directory is this project's UI KIT: finished, reusable graphics the game is drawn with. The goal is twofold \xE2\x80\x94 the reconstruction must match the design, AND it must be built from the kit: every element a kit sprite can draw uses that sprite (chosen by its role and shape; a sprite whose colours or details differ a little from the design is still the right one), a nine-sliced sprite is stretched to the element's size, a simple one keeps its proportions.",
            !inp.ownFiles.IsEmpty() ?
                "Only these sprites were cut out of this very design, for artwork the kit lacks: " + own + ". No other sprite can be made: do not ask for one." :
                String("No sprite can be added to the folder: what the kit has no graphic for is drawn with a rect, or left out and named in metadata.approximations."),
            !purposes.IsEmpty() ? "What the kit sprites are for: " + D::Join(purposes, "; ") + "." : String(),
            "`ui_structure.json` ALREADY EXISTS in the task folder \xE2\x80\x94 a first draft made from a markup of the design in terms of the kit: every element found on the design is there at its roughly measured box, drawn by the kit sprite chosen for it, labels with their fonts. Do not start over and do not write the file from scratch. Your work is the skill's loop on that draft: run `check` first, read its reports and pictures (comparison, visual audit, risk review), measure the regions that are off, fix the structure with `patch` (positions, sizes, order, texts, font sizes, a wrong or missing sprite, missing elements), check again, crop risky elements with `target`, write the reviews, finalize.",
            "Differences that come only from a kit sprite looking a little different from the design's artwork are expected: do not chase them by replacing kit sprites with cuts or rects \xE2\x80\x94 say so in the reviews (an approximation by the kit), and judge alignment by position, size and hierarchy.",
            !styles.IsEmpty() ?
                "Text styles of this design \xE2\x80\x94 a label keeps the fontFamily, color and stroke of its style (fontSize from your measurement): " + styleList + "." :
                String(),
        };
        Vector<String> kept;
        for (auto& l : lines)
        {
            if (!l.IsEmpty())
                kept.Add(l);
        }
        return D::Join(kept, "\n");
    }

    // --- the skill's steps as acts -----------------------------------------------------------------------------

    SkillActs::SkillActs(const String& traceDir, const String& taskDir, const Ref<RunLog>& log, const UiImage& design, double w, double h):
        mTraceDir(traceDir), mTaskDir(taskDir), mLog(log), mDesign(design), mW(w), mH(h)
    {}

    void SkillActs::Sync(bool final /*= false*/)
    {
        double now = NowMs();
        if (!final && now - mLastSync < 1500)
            return;
        mLastSync = now;

        Json file;
        if (!Json::Parse(PipelineUtils::ReadFileBytes(mTraceDir + "/trace.json"), file) || !file.IsObject())
            return;
        const Json* steps = file.Get("steps");
        if (!steps || !steps->IsArray())
            return;

        static const std::regex stepIds("^(prepare|turn-\\d+|check|agent-done)$");
        static const std::regex keep("(comparison|risk_review|reconstruction|visual_audit|design_grid|element|target|preview|contact_sheet)",
                                     std::regex::icase);
        for (auto& s : steps->Items())
        {
            String id = s.Get("id") ? s.Get("id")->ToJsString() : String();
            if (!std::regex_match((const char*)id.Data(), stepIds) || mSeen.Contains(id))
                continue;
            mSeen.Add(id);
            String title = s.Get("title") ? s.Get("title")->ToJsString() : String();
            String kind = s.Get("kind") ? s.Get("kind")->ToJsString() : String();
            const Json* data = s.Get("data");
            const Json* actionObj = data ? data->Get("action") : nullptr;
            String action = actionObj && actionObj->Get("action") ? actionObj->Get("action")->ToJsString() : String();
            bool checkKind = kind == "check" || action == "check" || action == "target" || action == "finalize";
            int act = mLog->Begin("skill-" + id, checkKind ? "check" : "turn", title);

            const Json* images = s.Get("images");
            int shown = 0;
            String reconFile;
            if (images && images->IsArray())
            {
                for (auto& im : images->Items())
                {
                    String imTitle = im.Get("title") ? im.Get("title")->ToJsString() : String();
                    String imFile = im.Get("file") ? im.Get("file")->ToJsString() : String();
                    if (reconFile.IsEmpty() && (StartsWithNoCase(imTitle, "reconstruction") || StartsWithNoCase(imFile, "reconstruction")))
                        reconFile = imFile;
                    if (shown >= 4 || !(std::regex_search((const char*)imTitle.Data(), keep) || std::regex_search((const char*)imFile.Data(), keep)))
                        continue;
                    UiImage pic;
                    if (UiImage::Decode(PipelineUtils::ReadFileBytes(mTraceDir + "/" + imFile), pic))
                    {
                        mLog->Picture(act, imTitle, pic, 1400);
                        shown++;
                    }
                }
            }

            // A step that rendered the layout: its reconstruction against the design, pixel by pixel (a check or a target
            // renders the screen at the design's size; a preview is another size or state)
            std::optional<double> score;
            bool rendered = id == "check" || action == "check" || action == "target";
            if (rendered)
            {
                // A check that failed its audit keeps no copy of the reconstruction in the trace: the task folder has it
                String path = !reconFile.IsEmpty() ? mTraceDir + "/" + reconFile : mTaskDir + "/reconstruction.png";
                UiImage recon;
                if (UiImage::Decode(PipelineUtils::ReadFileBytes(path), recon))
                {
                    score = CompareShots(mDesign, recon, mW, mH).score;
                    mLog->Picture(act, "pixel difference", DiffPicture(mDesign, recon));
                    if (!scoreFrom)
                        scoreFrom = score;
                    this->score = score;
                }
            }

            String report = D::JsTrim(s.Get("text") ? s.Get("text")->ToJsString() : String());
            String note = D::JsTrim(s.Get("note") && !s.Get("note")->IsNull() ? s.Get("note")->ToJsString() : String());
            Vector<String> noteParts;
            if (!note.IsEmpty())
                noteParts.Add(note);
            if (!report.IsEmpty())
                noteParts.Add(D::JsSlice(report, 0, 700));
            String joined = D::JsSlice(D::Join(noteParts, "\n\n"), 0, 1200);

            Json patch = Json::Object();
            patch["status"] = s.Get("status") && s.Get("status")->IsString() ? s.Get("status")->Str() : String("ok");
            if (s.Get("ms") && s.Get("ms")->IsNumber())
                patch["ms"] = *s.Get("ms");
            if (!joined.IsEmpty())
                patch["note"] = joined;
            Json d = Json::Object();
            d["skill"] = 1;
            if (!action.IsEmpty())
                d["action"] = action;
            d["step"] = id;
            d["label"] = D::JsSlice(title, 0, 140);
            if (score)
                d["score"] = *score;
            patch["data"] = d;
            mLog->End(act, patch);
        }
    }

    Coroutine<SkillBuildResult> RunSkillBuild(SkillBuildInput b)
    {
        SkillBuildResult res;
        const WorkDirs& d = b.inputs.dirs;
        String traceDir = d.work + "/trace";
        std::error_code ec;
        fs::create_directories(fs::path((const char*)traceDir.Data()), ec);
        UiImage design;
        UiImage::Decode(b.design, design);
        SkillActs acts(traceDir, d.task, b.log, design, b.w, b.h);

        AgentRun run;
        run.python = b.python;
        run.workDir = d.work;
        run.design = d.design;
        run.assets = d.assets;
        run.task = d.task;
        run.scratch = d.scratch;
        run.traceDir = traceDir;
        run.maxTurns = b.maxTurns;
        run.name = b.name;
        run.hint = b.hint;
        run.note = SkillNote(b.inputs, b.kit, b.styles);
        for (auto& f : b.inputs.fonts)
            run.fonts.Add("fonts/" + f);
        run.seed = b.inputs.seed;
        run.onTick = [&]() { acts.Sync(); };

        String salt = b.ai->salt;
        b.ai->salt = b.salt;
        AgentResult agent = co_await RunAgent(b.ai, run);
        b.ai->salt = salt;
        res.cancelled = agent.failure == "Cancelled";
        acts.Sync(true);
        res.score = acts.score;
        res.scoreFrom = acts.scoreFrom;

        Json state;
        Json::Parse(PipelineUtils::ReadFileBytes(d.task + "/workflow_state.json"), state);
        if (agent.summary.IsObject())
            res.summary = agent.summary;
        else
        {
            res.summary = Json::Object();
            res.summary["complete"] = false;
            res.summary["turns"] = 0;
            res.summary["check_count"] = state.Get("check_count") ? *state.Get("check_count") : Json(0);
            res.summary["structure_revision"] = 0;
            res.summary["target_count"] = state.Get("target_count") ? *state.Get("target_count") : Json(0);
            res.summary["blockers"] = Json::Array();
        }

        Json structure;
        String layout = PipelineUtils::ReadFileBytes(d.task + "/ui_structure.json");
        bool parsed = !layout.IsEmpty() && Json::Parse(layout, structure) && structure.IsObject();
        if (!parsed || (!res.cancelled && !agent.ok))
        {
            res.error = !agent.failure.IsEmpty() ? agent.failure : !agent.error.IsEmpty() ? agent.error : !agent.detail.IsEmpty() ? agent.detail :
                res.summary.Get("status") ? res.summary.Get("status")->ToJsString() : String("no checked layout");
            co_return res;
        }

        // The boxes the skill's verifier resolved at its last check; a layout stopped mid-way may have moved on since - then
        // its own positions stand
        D::FromStructureInput conv;
        conv.structure = structure;
        if (!res.cancelled)
        {
            Json legend;
            if (Json::Parse(PipelineUtils::ReadFileBytes(d.task + "/all_elements_legend.json"), legend))
                conv.boxes = D::LegendBoxes(legend);
        }
        conv.kit = b.kit;
        conv.assetNames = b.inputs.assetNames;
        conv.dpi = b.designDpi;
        D::FromStructureResult made = D::FromUiStructure(conv);
        res.doc = made.doc;
        res.styles = made.styles;
        res.warnings = made.warnings;
        res.ok = true;
        co_return res;
    }
}
