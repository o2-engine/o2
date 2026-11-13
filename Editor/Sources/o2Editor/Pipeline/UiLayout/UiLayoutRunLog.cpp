#include "o2Editor/stdafx.h"
#include "UiLayoutRunLog.h"

#include <filesystem>

#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutTrace.h"

namespace Editor::UiLayout
{
    namespace fs = std::filesystem;

    // How many runs of a node keep their pictures
    static const int KEEP_RUNS = 8;

    RunLog::RunLog(const String& dir):
        mDir(dir)
    {}

    String RunLog::DirOf(const String& workDir, const String& runId)
    {
        String safe;
        for (int i = 0; i < runId.Length(); i++)
        {
            char c = runId[i];
            bool keep = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
            safe += keep ? c : '_';
        }
        String work = workDir;
        while (HasSuffix(work, "/"))
            work = work.SubStr(0, work.Length() - 1);
        return work + "/runs/" + safe;
    }

    void RunLog::Open()
    {
        std::error_code ec;
        fs::path dir((const char*)mDir.Data());
        fs::remove_all(dir, ec);
        fs::create_directories(dir, ec);

        Vector<Pair<String, fs::file_time_type>> runs;
        for (auto& entry : fs::directory_iterator(dir.parent_path(), ec))
        {
            if (entry.is_directory(ec))
                runs.Add({ String(entry.path().string()), entry.last_write_time(ec) });
        }
        StableSortBy(runs, [](const Pair<String, fs::file_time_type>& a, const Pair<String, fs::file_time_type>& b) { return a.second > b.second; });
        for (int i = KEEP_RUNS; i < runs.Count(); i++)
            fs::remove_all(fs::path((const char*)runs[i].first.Data()), ec);
    }

    int RunLog::Begin(const String& id, const String& kind, const String& title)
    {
        Json act = Json::Object();
        act["id"] = id;
        act["kind"] = kind;
        act["title"] = title;
        act["status"] = "run";
        mActs.Add(act);
        mStarted.Add(NowMs());
        Changed();
        return mActs.Count() - 1;
    }

    void RunLog::End(int act, const Json& patch /*= Json::Object()*/)
    {
        if (act < 0 || act >= mActs.Count())
            return;
        Json& a = mActs.Items()[act];
        a["status"] = "ok";
        a["ms"] = Since(mStarted[act]);
        for (auto& m : patch.Members())
            a[m.first] = m.second;
        Changed();
    }

    void RunLog::Close(const String& status)
    {
        for (int i = 0; i < mActs.Count(); i++)
        {
            if (mActs.Items()[i]["status"].Str() == "run")
            {
                Json patch = Json::Object();
                patch["status"] = status;
                End(i, patch);
            }
        }
    }

    void RunLog::Picture(int act, const String& title, const UiImage& img, int max /*= 1100*/)
    {
        if (act < 0 || act >= mActs.Count() || img.w <= 0 || img.h <= 0)
            return;
        double k = std::min(1.0, std::min((double)max/img.w, (double)max/img.h));
        UiImage small = k < 1 ? img.Resize(std::max(1, (int)JsRound(img.w*k)), std::max(1, (int)JsRound(img.h*k))) : img;
        String n = (String)(++mPictures);
        while (n.Length() < 3)
            n = "0" + n;
        String file = n + ".png";
        if (!PipelineUtils::WriteFileBytes(mDir + "/" + file, small.Flatten(0x2B, 0x2F, 0x3A).Png()))
            return;
        Json& a = mActs.Items()[act];
        if (!a.Has("images"))
            a["images"] = Json::Array();
        Json pic = Json::Object();
        pic["title"] = title;
        pic["file"] = file;
        a["images"].Add(pic);
        Changed();
    }

    void RunLog::SetData(int act, const String& key, const Json& value)
    {
        if (act < 0 || act >= mActs.Count())
            return;
        Json& a = mActs.Items()[act];
        if (!a.Has("data"))
            a["data"] = Json::Object();
        a["data"][key] = value;
        Changed();
    }

    Json& RunLog::Act(int act)
    {
        return mActs.Items()[act];
    }

    void RunLog::Changed()
    {
        if (onChange)
            onChange();
    }
}
