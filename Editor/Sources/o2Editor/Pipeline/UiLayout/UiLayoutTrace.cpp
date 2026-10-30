#include "o2Editor/stdafx.h"
#include "UiLayoutTrace.h"

#include <chrono>
#include <cstdio>
#include <ctime>

#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Threading/ScopeLock.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

namespace Editor::UiLayout
{
    String IsoNow()
    {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        std::time_t seconds = (std::time_t)(ms/1000);
        std::tm utc{};
#if defined(_WIN32)
        gmtime_s(&utc, &seconds);
#else
        gmtime_r(&seconds, &utc);
#endif
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
                 utc.tm_hour, utc.tm_min, utc.tm_sec, (int)(ms % 1000));
        return buffer;
    }

    double NowMs()
    {
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    Trace::Trace(const String& dir, int w, int h):
        mDir(dir)
    {
        mFile = Json::Object();
        mFile["version"] = 1;
        mFile["startedAt"] = IsoNow();
        Json design = Json::Object();
        design["w"] = w;
        design["h"] = h;
        design["file"] = "design.png";
        mFile["design"] = design;
        mSteps = Json::Array();
    }

    void Trace::Reset()
    {
        if (mDir.IsEmpty())
            return;

        if (o2FileSystem.IsFolderExist(mDir))
            o2FileSystem.FolderRemove(mDir, true);
        o2FileSystem.FolderCreate(mDir, true);
    }

    Json Trace::Image(const String& title, const UiImage& img, const String& name /*= ""*/)
    {
        String file = name;
        if (file.IsEmpty())
        {
            ScopeLock<Mutex> lock(mMutex);
            char buffer[16];
            snprintf(buffer, sizeof(buffer), "%03d.png", ++mImages);
            file = buffer;
        }

        if (!mDir.IsEmpty())
            PipelineUtils::WriteFileBytes(mDir + "/" + file, img.Png());

        Json entry = Json::Object();
        entry["title"] = title;
        entry["file"] = file;
        entry["w"] = img.w;
        entry["h"] = img.h;
        return entry;
    }

    void Trace::Step(const Json& fields)
    {
        Json step = Json::Object();
        const Json* images = fields.Get("images");
        step["images"] = images ? *images : Json::Array();
        for (auto& member : fields.Members())
        {
            if (member.first != "images")
                step[member.first] = member.second;
        }

        {
            ScopeLock<Mutex> lock(mMutex);
            mSteps.Add(step);
        }

        // The debug view polls while the node runs: every step lands right away
        Write();
    }

    void Trace::Write()
    {
        if (mDir.IsEmpty())
            return;

        String text = GetFile().Dump(1);
        ScopeLock<Mutex> lock(mWriteMutex);
        String tmp = mDir + "/trace.json.tmp";
        if (PipelineUtils::WriteFileBytes(tmp, text))
        {
            o2FileSystem.FileDelete(mDir + "/trace.json");
            o2FileSystem.FileMove(tmp, mDir + "/trace.json");
        }
    }

    void Trace::SetSummary(const Json& summary)
    {
        ScopeLock<Mutex> lock(mMutex);
        mSummary = summary;
    }

    Json Trace::GetFile() const
    {
        ScopeLock<Mutex> lock(mMutex);
        Json file = Json::Object();
        for (auto& member : mFile.Members())
        {
            if (member.first != "finishedAt")
                file[member.first] = member.second;
        }
        file["steps"] = mSteps;
        if (!mSummary.IsNull())
            file["summary"] = mSummary;
        if (auto finished = mFile.Get("finishedAt"))
            file["finishedAt"] = *finished;
        return file;
    }

    void Trace::Flush()
    {
        Write();
    }

    void Trace::Reload()
    {
        if (mDir.IsEmpty())
            return;

        Json file;
        if (!Json::Parse(PipelineUtils::ReadFileBytes(mDir + "/trace.json"), file) || !file.IsObject())
            return;

        const Json* steps = file.Get("steps");
        if (!steps || !steps->IsArray())
            return;

        ScopeLock<Mutex> lock(mMutex);
        mSteps = *steps;
        if (const Json* summary = file.Get("summary"))
            mSummary = *summary;
    }

    void Trace::Adopt(const Json& file)
    {
        ScopeLock<Mutex> lock(mMutex);
        for (auto& member : file.Members())
        {
            if (member.first == "steps")
                mSteps = member.second.IsArray() ? member.second : Json::Array();
            else if (member.first == "summary")
                mSummary = member.second;
            else if (member.first != "design")
                mFile[member.first] = member.second;
        }
    }

    String Trace::LastStepId() const
    {
        ScopeLock<Mutex> lock(mMutex);
        if (mSteps.Items().IsEmpty())
            return String();

        const Json* id = mSteps.Items().Last().Get("id");
        return id ? id->ToJsString() : String();
    }

    void Trace::PopLastStep()
    {
        ScopeLock<Mutex> lock(mMutex);
        if (!mSteps.Items().IsEmpty())
            mSteps.Items().RemoveAt(mSteps.Items().Count() - 1);
    }

    void Trace::Save()
    {
        {
            ScopeLock<Mutex> lock(mMutex);
            mFile["finishedAt"] = IsoNow();
        }

        Write();
    }
}
