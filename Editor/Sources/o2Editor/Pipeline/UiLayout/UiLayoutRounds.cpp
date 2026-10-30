#include "o2Editor/stdafx.h"
#include "UiLayoutRunSteps.h"

#include "o2/Utils/Coroutines/Awaiters.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Threading/Mutex.h"
#include "o2/Utils/Threading/ScopeLock.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutAgent.h"

// Rounds of edits after a run, AssetsLine's run.ts runLocked / Rounds: <work>/rounds.json says what the task folder's states
// are - the run without edits (0) and the result of each round after it - each kept as the files the model writes
// (rounds/<k>/: ui_structure.json, alignment_review.md, component_review.md, trace.json). A run continues from the newest
// state its edits share with the stored ones: adding a round runs that round only; removing the last one is the previous
// round's cached result, or its files back in the task folder and one check
namespace Editor::UiLayout
{
    namespace
    {
        const char* authored[] = { "ui_structure.json", "alignment_review.md", "component_review.md" };

        String Hex(const String& bytes)
        {
            static const char* digits = "0123456789abcdef";
            String out;
            for (int i = 0; i < bytes.Length(); i++)
            {
                UInt8 c = (UInt8)bytes[i];
                out += digits[c >> 4];
                out += digits[c & 15];
            }
            return out;
        }

        ImageToUiResult Fail(const String& error)
        {
            ImageToUiResult res;
            res.error = error;
            return res;
        }

        bool IsFile(const String& path)
        {
            return o2FileSystem.IsFileExist(path);
        }

        // The agent's summary as rounds.json keeps it
        Json AgentSummaryOf(const Json& summary)
        {
            auto num = [&](const char* key) { const Json* v = summary.Get(key); return v && v->IsNumber() ? v->Num() : 0.0; };
            Json out = Json::Object();
            out["complete"] = summary.Get("complete") && summary.Get("complete")->Bool();
            out["turns"] = num("turns");
            out["check_count"] = num("check_count");
            out["structure_revision"] = num("structure_revision");
            out["target_count"] = num("target_count");
            if (const Json* status = summary.Get("status"))
                out["status"] = *status;
            out["blockers"] = summary.Get("blockers") && summary.Get("blockers")->IsArray() ? *summary.Get("blockers") : Json::Array();
            return out;
        }

        // ^(edit\d+-)?output$
        bool IsClosingStep(const String& id)
        {
            if (id == "output")
                return true;
            if (!id.StartsWith("edit") || !HasSuffix(id, "-output"))
                return false;

            String number = id.SubStr(4, id.Length() - 7);
            if (number.IsEmpty())
                return false;
            for (int i = 0; i < number.Length(); i++)
            {
                if (number[i] < '0' || number[i] > '9')
                    return false;
            }
            return true;
        }

        struct KeptRound
        {
            String id;
            String hash;
            Json   summary;
        };

        struct RoundsState
        {
            String            base;    // ImageToUiRunOptions::baseKey of the run without edits
            String            note;    // The --sprites-note of that run (the rounds pass it again)
            Json              summary; // Its agent summary
            Vector<KeptRound> rounds;
        };

        class Rounds
        {
        public:
            explicit Rounds(const String& work): mWork(work) {}

            bool Read(RoundsState& state) const
            {
                Json json;
                if (!Json::Parse(PipelineUtils::ReadFileBytes(File()), json) || !json.IsObject())
                    return false;

                const Json* base = json.Get("base");
                const Json* rounds = json.Get("rounds");
                if (!base || !base->IsString() || !rounds || !rounds->IsArray())
                    return false;

                state = RoundsState();
                state.base = base->Str();
                state.note = json.Get("note") ? json.Get("note")->ToJsString() : String();
                state.summary = json.Get("summary") ? *json.Get("summary") : Json::Object();
                for (auto& r : rounds->Items())
                {
                    KeptRound round;
                    round.id = r.Get("id") ? r.Get("id")->ToJsString() : String();
                    round.hash = r.Get("hash") ? r.Get("hash")->ToJsString() : String();
                    round.summary = r.Get("summary") ? *r.Get("summary") : Json::Object();
                    state.rounds.Add(round);
                }
                return true;
            }

            void Write(const RoundsState& state) const
            {
                Json rounds = Json::Array();
                for (auto& r : state.rounds)
                {
                    Json round = Json::Object();
                    round["id"] = r.id;
                    round["hash"] = r.hash;
                    round["summary"] = r.summary;
                    rounds.Add(round);
                }
                Json json = Json::Object();
                json["base"] = state.base;
                json["note"] = state.note;
                json["summary"] = state.summary;
                json["rounds"] = rounds;

                String tmp = File() + ".tmp";
                if (PipelineUtils::WriteFileBytes(tmp, json.Dump(1)))
                {
                    o2FileSystem.FileDelete(File());
                    o2FileSystem.FileMove(tmp, File());
                }
            }

            // State k is kept, and the skill's inputs it was made from are still there
            bool Usable(int k, const WorkDirs& d) const
            {
                return IsFile(Dir(k) + "/ui_structure.json") && IsFile(d.design) && o2FileSystem.IsFolderExist(d.assets);
            }

            void Snapshot(int k, const String& task) const
            {
                String dir = Dir(k);
                if (o2FileSystem.IsFolderExist(dir))
                    o2FileSystem.FolderRemove(dir, true);
                o2FileSystem.FolderCreate(dir, true);
                for (auto file : authored)
                {
                    if (IsFile(task + "/" + file))
                        o2FileSystem.FileCopy(task + "/" + file, dir + "/" + file);
                }
            }

            // The debug trace as it was at state k (its pictures stay in the trace folder: a round's are named after it, so no
            // other state overwrites them)
            void SaveTrace(int k, const Ref<Trace>& trace) const
            {
                o2FileSystem.FolderCreate(Dir(k), true);
                PipelineUtils::WriteFileBytes(Dir(k) + "/trace.json", trace->GetFile().Dump());
            }

            void LoadTrace(int k, const Ref<Trace>& trace) const
            {
                Json kept;
                if (Json::Parse(PipelineUtils::ReadFileBytes(Dir(k) + "/trace.json"), kept) && kept.IsObject() && kept.Get("steps") &&
                    kept.Get("steps")->IsArray())
                {
                    trace->Adopt(kept);
                }
                else
                    trace->Reload();
            }

            // Puts state k's files into the task folder; whether anything changed
            bool Restore(int k, const String& task) const
            {
                o2FileSystem.FolderCreate(task, true);
                bool changed = false;
                for (auto file : authored)
                {
                    String from = Dir(k) + "/" + file, to = task + "/" + file;
                    bool want = IsFile(from), have = IsFile(to);
                    if (!want && !have)
                        continue;
                    if (want && have && PipelineUtils::ReadFileBytes(from) == PipelineUtils::ReadFileBytes(to))
                        continue;

                    changed = true;
                    if (want)
                        PipelineUtils::WriteFileBytes(to, PipelineUtils::ReadFileBytes(from));
                    else
                        o2FileSystem.FileDelete(to);
                }

                // A task folder that lost its state (or its reconstruction) is rebuilt by the check
                bool state = !PipelineUtils::ReadFileBytes(task + "/workflow_state.json").IsEmpty();
                return changed || !state || !IsFile(task + "/reconstruction.png");
            }

            // A node run before rounds existed: its finished task folder is the run without edits, if it was made from this
            // very design
            bool Adopt(const WorkDirs& d, const String& designFlatPng, const String& base, const Ref<Trace>& trace, RoundsState& state) const
            {
                Json st;
                if (!Json::Parse(PipelineUtils::ReadFileBytes(d.task + "/workflow_state.json"), st) || !st.IsObject())
                    return false;

                String status = st.Get("status") ? st.Get("status")->ToJsString() : String();
                if (status != "completed" && status != "checked" && status != "finalize_failed")
                    return false;
                if (!IsFile(d.design) || PipelineUtils::ReadFileBytes(d.design) != designFlatPng)
                    return false;
                if (!IsFile(d.task + "/ui_structure.json") || !IsFile(d.task + "/reconstruction.png"))
                    return false;

                trace->Reload();
                Json file = trace->GetFile();
                state = RoundsState();
                state.base = base;
                state.note = NoteFromFolder(d.assets, file);
                state.summary = AgentSummaryOf(file.Get("summary") ? *file.Get("summary") : Json::Object());
                Snapshot(0, d.task);
                SaveTrace(0, trace);
                Write(state);
                return true;
            }

        private:
            String mWork;

        private:
            String File() const { return mWork + "/rounds.json"; }
            String Dir(int k) const { return mWork + "/rounds/" + (String)k; }

            // The sprites note of a run made before rounds.json kept it
            static String NoteFromFolder(const String& assets, const Json& trace)
            {
                Vector<String> names;
                if (o2FileSystem.IsFolderExist(assets))
                {
                    for (auto& f : o2FileSystem.GetFolderInfo(assets).files)
                        names.Add(o2FileSystem.GetPathWithoutDirectories(f.path));
                }

                Vector<AssetFile> files;
                for (auto& name : names)
                {
                    if (!HasSuffix(name.ToLowerCase(), ".png"))
                        continue;

                    AssetFile file;
                    file.file = name;
                    file.look = name;
                    if (names.Contains(name + ".meta"))
                        file.nine = NineSlice{ 1, 1, 1, 1 };
                    files.Add(file);
                }

                String color;
                if (const Json* steps = trace.Get("steps"))
                {
                    for (auto& step : steps->Items())
                    {
                        if (step.Get("id") && step.Get("id")->ToJsString() == "background")
                        {
                            const Json* data = step.Get("data");
                            if (data && data->Get("color") && data->Get("color")->IsString())
                                color = data->Get("color")->Str();
                            break;
                        }
                    }
                }

                UiBackdrop background;
                if (names.Contains("bg_backdrop.png"))
                {
                    background.kind = "image";
                    background.key = "background";
                }
                else if (!color.IsEmpty())
                {
                    background.kind = "color";
                    background.color = color;
                }
                return SpritesNote(files, background);
            }
        };

        // One of the skill's commands, run by the host (a restore needs a check); the error, empty when it passed
        Coroutine<String> SkillCommand(WorkDirs d, String command, String configuredPython, Ref<PipelineExecContext> exec)
        {
            String home = ImageToUiHome();
            if (home.IsEmpty())
                co_return String("imageToUi: the Image-To-UI Python is missing: o2/Tools/ImageToUi (tools/sync-image-to-ui.sh copies it)");

            String python = co_await FindPython(configuredPython, exec);
            if (python.IsEmpty())
                co_return String("imageToUi: no Python 3 found - install Python 3 with Pillow and numpy, or set it in the pipeline settings");

            String workflow = home + (HasSuffix(home, "/") ? "" : "/") + "upstream/image-to-ui/scripts/workflow.py";
            bool state = IsFile(d.task + "/workflow_state.json");

            // prepare takes an empty folder or one with its own state: a task folder that lost its state keeps only the files
            // the model wrote, set aside
            Vector<Pair<String, String>> kept;
            if (!state)
            {
                for (auto file : authored)
                {
                    if (IsFile(d.task + "/" + file))
                        kept.Add(Pair<String, String>(file, PipelineUtils::ReadFileBytes(d.task + "/" + file)));
                }
                if (o2FileSystem.IsFolderExist(d.task))
                    o2FileSystem.FolderRemove(d.task, true);
            }

            Vector<Vector<String>> steps;
            if (!state)
                steps.Add(Vector<String>({ "prepare", "--design", d.design, "--assets", d.assets, "--output", d.task }));
            if (command == "check")
                steps.Add(Vector<String>({ "check", "--output", d.task }));

            for (auto& args : steps)
            {
                Vector<String> full = { "-B", workflow };
                full.Add(args);
                ScriptResult res = co_await RunScript(python, full, d.work, exec);
                if (!res.failure.IsEmpty())
                    co_return res.failure;

                if (res.code != 0)
                {
                    Vector<String> lines = res.output.Trimed(" \t\r\n").Split("\n");
                    String tail;
                    for (int i = Math::Max(0, lines.Count() - 4); i < lines.Count(); i++)
                        tail += (tail.IsEmpty() ? "" : " | ") + lines[i];
                    co_return "imageToUi: workflow.py " + args[0] + " failed (exit " + (String)res.code + "): " + tail;
                }

                if (args[0] == "prepare")
                {
                    for (auto& file : kept)
                        PipelineUtils::WriteFileBytes(d.task + "/" + file.first, file.second);
                }
            }
            co_return String();
        }

        String AgentFailure(const AgentResult& agent)
        {
            String why = agent.error;
            const Json* status = agent.summary.Get("status");
            if (why.IsEmpty() && status && status->IsString())
                why = status->Str();
            return why.IsEmpty() ? String("no check passed") : why;
        }

        Mutex             busyMutex;
        Map<String, bool> busyFolders; // Work folders a run holds

        // Holds a work folder for one run, released when the run's coroutine ends however it ends
        struct FolderHold
        {
            String folder;

            ~FolderHold()
            {
                if (folder.IsEmpty())
                    return;

                ScopeLock<Mutex> lock(busyMutex);
                busyFolders.Remove(folder);
            }
        };

        Coroutine<ImageToUiResult> RunLocked(Ref<AiCtx> ai, String designPng, ImageToUiConfig cfg, Ref<Trace> trace, WorkDirs dirs,
                                             ImageToUiRunOptions opts)
        {
            double t0 = NowMs();
            auto st = mmake<RunState>();
            st->ai = ai;
            st->trace = trace;
            st->cfg = cfg;

            UiImage decoded;
            if (!UiImage::Decode(designPng, decoded))
                co_return Fail("imageToUi: the input is not a decodable image");

            st->design = decoded.Flatten(255, 255, 255);
            st->designPng = st->design.Png();
            Rounds rounds(dirs.work);

            // Where to start: the newest kept state this run continues - the run without edits (0) or the result of its first
            // k rounds - or from scratch
            RoundsState state;
            bool hasState = !opts.fresh && rounds.Read(state);
            if (!hasState && !opts.fresh)
                hasState = rounds.Adopt(dirs, st->designPng, opts.baseKey, trace, state);

            int k = -1;
            if (hasState && state.base == opts.baseKey && rounds.Usable(0, dirs))
            {
                k = 0;
                while (k < cfg.edits.Count() && k < state.rounds.Count() && state.rounds[k].hash == ImageToUiRoundHash(cfg.edits[k]) &&
                       rounds.Usable(k + 1, dirs))
                {
                    k++;
                }
            }

            Json summary;
            if (k < 0)
            {
                trace->Reset();
                BaseRun base = co_await RunBase(st, dirs);
                if (!base.ok)
                    co_return Fail(base.error);

                summary = base.summary;
                rounds.Snapshot(0, dirs.task);
                rounds.SaveTrace(0, trace);
                state = RoundsState();
                state.base = opts.baseKey;
                state.note = base.note;
                state.summary = summary;
                rounds.Write(state);
                k = 0;
            }
            else
            {
                // Continue: the trace as it was at that state, the task folder's files too
                rounds.LoadTrace(k, trace);
                // Its closing step is written again below when no round follows
                if (k == cfg.edits.Count() && IsClosingStep(trace->LastStepId()))
                    trace->PopLastStep();
                summary = k == 0 ? state.summary : state.rounds[k - 1].summary;
                bool changed = rounds.Restore(k, dirs.task);
                if (changed && k == cfg.edits.Count())
                {
                    // No round to run: the outputs need the restored layout's reconstruction
                    trace->Flush();
                    String error = co_await SkillCommand(dirs, "check", cfg.python, ai->exec);
                    if (!error.IsEmpty())
                        co_return Fail(error);
                }
                ai->Log(k == 0 ? String("imageToUi: continuing from the run without edits") : "imageToUi: continuing from edit round " + (String)k);
            }

            for (int j = k; j < cfg.edits.Count(); j++)
            {
                if (ai->exec && ai->exec->IsCancelled())
                    co_return Fail("Cancelled");

                int n = j + 1;
                double t = NowMs();
                const PipelineImageToUiEditRound& round = cfg.edits[j];
                Json earlier = Json::Array();
                for (int i = 0; i < j; i++)
                {
                    Json r = Json::Object();
                    r["round"] = i + 1;
                    r["items"] = PipelineImageToUi::EditItemsJson(cfg.edits[i].items);
                    earlier.Add(r);
                }
                Json request = Json::Object();
                request["round"] = n;
                request["items"] = PipelineImageToUi::EditItemsJson(round.items);
                request["earlier"] = earlier;
                String file = dirs.work + "/edits/round-" + (String)n + ".json";
                o2FileSystem.FolderCreate(dirs.work + "/edits", true);
                PipelineUtils::WriteFileBytes(file, request.Dump(1));
                trace->Flush();
                ai->Log("imageToUi: edit round " + (String)n + " \xE2\x80\x94 " + (String)round.items.Count() + " note(s), at most " +
                        (String)cfg.editTurns + " agent turns");

                AgentRun run = BaseAgentRun(dirs, trace, cfg, state.note);
                run.maxTurns = cfg.editTurns;
                run.revise = file;
                AgentResult agent = co_await RunAgent(ai, run);
                trace->Reload();
                if (!agent.failure.IsEmpty())
                    co_return Fail(agent.failure);
                if (!agent.ok)
                    co_return Fail("imageToUi: edit round " + (String)n + " ended without a checked layout (" + AgentFailure(agent) + ")");

                // A round cut short by its model (credits, an outage) is half a change: the node fails with why, and the result
                // before the round stays. One that ran out of turns is delivered, not completed, as a run is
                if (!agent.error.IsEmpty())
                    co_return Fail("imageToUi: edit round " + (String)n + " stopped \xE2\x80\x94 the model call failed: " + agent.error);

                summary = agent.summary.IsObject() ? agent.summary : Json::Object();
                rounds.Snapshot(n, dirs.task);
                rounds.SaveTrace(n, trace);
                state.rounds.Resize(j);
                KeptRound kept;
                kept.id = round.id;
                kept.hash = ImageToUiRoundHash(round);
                kept.summary = summary;
                state.rounds.Add(kept);
                rounds.Write(state);
                bool complete = summary.Get("complete") && summary.Get("complete")->Bool();
                ai->Log("imageToUi: edit round " + (String)n + (complete ? " completed" : " NOT completed") + " in " +
                        (String)(int)JsRound(Since(t)/1000.0) + " s");
            }

            // Outputs: the reconstruction, ui_structure.json as the skill left it, and every sprite the layout names
            ImageToUiResult res;
            res.layout = PipelineUtils::ReadFileBytes(dirs.task + "/ui_structure.json");
            res.preview = PipelineUtils::ReadFileBytes(dirs.task + "/reconstruction.png");
            Json structure;
            if (res.layout.IsEmpty() || !Json::Parse(res.layout, structure))
                co_return Fail("imageToUi: the agent left no readable ui_structure.json");
            if (res.preview.IsEmpty())
                co_return Fail("imageToUi: the agent left no reconstruction.png");

            // In the order the layout first uses them; a name with no file is skipped
            for (auto& f : AssetRefs(structure))
            {
                if (!HasSuffix(f.ToLowerCase(), ".png") || f.Contains("/") || !IsFile(dirs.assets + "/" + f))
                    continue;

                ImageToUiSprite sprite;
                sprite.key = f.SubStr(0, f.Length() - 4);
                sprite.png = PipelineUtils::ReadFileBytes(dirs.assets + "/" + f);
                if (IsFile(dirs.assets + "/" + f + ".meta"))
                    sprite.nine = ParseNineSliceMeta(PipelineUtils::ReadFileBytes(dirs.assets + "/" + f + ".meta"));
                res.sprites.Add(sprite);
            }

            int edits = cfg.edits.Count();
            Json full = summary.IsObject() ? summary : Json::Object();
            full["sprites"] = res.sprites.Count();
            full["edits"] = edits;
            full["textCalls"] = ai->textCalls.Load();
            full["imageCalls"] = ai->imageCalls.Load();
            full["replayed"] = ai->replayed.Load();
            full["ms"] = Since(t0);
            res.complete = full.Get("complete") && full.Get("complete")->Bool();
            if (const Json* blockers = full.Get("blockers"))
            {
                for (auto& b : blockers->Items())
                    res.blockers.Add(b.ToJsString());
            }
            int checks = full.Get("check_count") ? (int)full.Get("check_count")->Num() : 0;
            int turns = full.Get("turns") ? (int)full.Get("turns")->Num() : 0;

            trace->SetSummary(full);
            {
                String done = res.complete
                    ? "Done: the skill completed (" + (String)checks + " checks, " + (String)res.sprites.Count() + " sprites)"
                    : "Done, NOT completed: " + (String)res.blockers.Count() + " blocker(s) after " + (String)turns + " turns";
                Json step = Json::Object();
                step["id"] = edits ? "edit" + (String)edits + "-output" : String("output");
                step["kind"] = "output";
                step["status"] = res.complete ? "ok" : "warn";
                step["title"] = edits ? "Edit " + (String)edits + " \xC2\xB7 " + done : done;
                step["data"] = full;
                step["text"] = res.layout;
                trace->Step(step);
            }
            trace->Save();
            // The trace of this state ends with that step
            rounds.SaveTrace(edits, trace);

            res.ok = true;
            res.summary = full;
            co_return res;
        }
    }

    String ImageToUiRoundHash(const PipelineImageToUiEditRound& round)
    {
        return Hex(PipelineUtils::Sha256(PipelineImageToUi::EditItemsJson(round.items).Dump())).SubStr(0, 16);
    }

    String ImageToUiBaseKey(const String& designPng, const Json& settings)
    {
        return Hex(PipelineUtils::Sha256(designPng + settings.Dump()));
    }

    Coroutine<ImageToUiResult> RunImageToUi(Ref<AiCtx> ai, String designPng, ImageToUiConfig cfg, Ref<Trace> trace, String workDir,
                                            ImageToUiRunOptions opts)
    {
        if (workDir.IsEmpty())
            co_return Fail("imageToUi: the node needs its work folder");

        // One run at a time in a work folder: two signatures of one node (a round added while the previous one still runs)
        // would share the task folder
        WorkDirs dirs = MakeWorkDirs(workDir);
        FolderHold hold;
        for (;;)
        {
            {
                ScopeLock<Mutex> lock(busyMutex);
                if (!busyFolders.ContainsKey(dirs.work))
                {
                    busyFolders[dirs.work] = true;
                    hold.folder = dirs.work;
                    break;
                }
            }

            if (ai->exec && ai->exec->IsCancelled())
                co_return Fail("Cancelled");

            co_await WaitNextFrame();
        }

        ImageToUiResult res = co_await RunLocked(ai, designPng, cfg, trace, dirs, opts);
        co_return res;
    }
}
