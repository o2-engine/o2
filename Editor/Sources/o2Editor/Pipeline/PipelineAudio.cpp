#include "o2Editor/stdafx.h"
#include "PipelineAudio.h"

#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Types/UID.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

#include <cstdio>
#include <cstdlib>

namespace Editor::PipelineAudio
{
    String PcmToWav(const String& pcm, int sampleRate, int channels)
    {
        int bitsPerSample = 16;
        int byteRate = sampleRate * channels * bitsPerSample / 8;
        int blockAlign = channels * bitsPerSample / 8;
        int dataSize = pcm.Length();

        String out;
        out.Reserve(44 + dataSize);
        auto u32 = [&](UInt v) { for (int i = 0; i < 4; i++) out += (char)((v >> (i * 8)) & 0xff); };
        auto u16 = [&](UInt v) { for (int i = 0; i < 2; i++) out += (char)((v >> (i * 8)) & 0xff); };

        out += "RIFF"; u32(36 + dataSize); out += "WAVE";
        out += "fmt "; u32(16); u16(1); u16(channels); u32(sampleRate); u32(byteRate); u16(blockAlign); u16(bitsPerSample);
        out += "data"; u32(dataSize);
        out += pcm;
        return out;
    }

    int PcmRateFromMime(const String& mime)
    {
        int idx = mime.ToLowerCase().Find("rate=");
        if (idx < 0)
            return 24000;

        int rate = atoi(mime.SubStr(idx + 5).Data());
        return rate > 0 ? rate : 24000;
    }

    static bool ffmpegChecked = false;
    static bool ffmpegAvailable = false;

    String ToolPath(const char* name)
    {
#if defined(PLATFORM_WINDOWS)
        return name;
#else
        const char* candidates[] = { "/opt/homebrew/bin/", "/usr/local/bin/", "/usr/bin/" };
        for (auto prefix : candidates)
        {
            String path = String(prefix) + name;
            if (o2FileSystem.IsFileExist(path))
                return path;
        }
        return name;
#endif
    }

    bool IsFfmpegAvailable()
    {
#if defined(PLATFORM_WASM) || defined(PLATFORM_IOS) || defined(PLATFORM_ANDROID)
        return false;
#else
        if (ffmpegChecked)
            return ffmpegAvailable;

        ffmpegChecked = true;
        String cmd = ToolPath("ffmpeg") + " -version > /dev/null 2>&1";
#if defined(PLATFORM_WINDOWS)
        cmd = "ffmpeg -version > NUL 2>&1";
#endif
        ffmpegAvailable = system(cmd.Data()) == 0;
        return ffmpegAvailable;
#endif
    }

    String TempFilePath(const String& ext)
    {
        UID uid;
        uid.Randomize();
        String folder = PipelineUtils::GetWorkPath() + "tmp/";
        o2FileSystem.FolderCreate(folder, true);
        return folder + (String)uid + "." + ext;
    }

    ProcessResult Process(const String& data, const String& mimeType, const ProcessOptions& opt)
    {
        ProcessResult result;
        if (!IsFfmpegAvailable())
        {
            result.ffmpegMissing = true;
            result.error = "ffmpeg is not installed";
            return result;
        }

        String inExt = PipelineUtils::ExtensionForMime(mimeType);
        if (inExt == "bin") inExt = "mp3";
        String outExt = opt.format == "keep" ? inExt : opt.format;
        String inPath = TempFilePath(inExt);
        String outPath = TempFilePath(outExt);
        PipelineUtils::WriteFileBytes(inPath, data);

        Vector<String> filters;
        if (opt.trimSilence)
            filters.Add("silenceremove=start_periods=1:start_threshold=-50dB:start_silence=0.05,areverse,silenceremove=start_periods=1:start_threshold=-50dB:start_silence=0.05,areverse");
        if (opt.normalize)
            filters.Add("loudnorm=I=" + (String)opt.loudnessTarget + ":TP=-1.5:LRA=11");
        if (opt.seamlessLoop && opt.crossfadeMs > 0)
        {
            float duration = DurationSeconds(data, mimeType);
            float cf = Math::Min(opt.crossfadeMs / 1000.0f, Math::Max(0.01f, duration / 2.0f - 0.01f));
            if (duration > cf * 2.0f)
            {
                // Overlap the tail onto the head so the loop point crossfades
                filters.Add("asplit=2[a][b];[a]atrim=0:" + (String)(duration - cf) + "[head];[b]atrim=" + (String)(duration - cf) +
                            ",asetpts=PTS-STARTPTS[tail];[tail][head]acrossfade=d=" + (String)cf + ":c1=tri:c2=tri");
            }
        }
        if (opt.fadeInMs > 0)
            filters.Add("afade=t=in:st=0:d=" + (String)(opt.fadeInMs / 1000.0f));
        if (opt.fadeOutMs > 0)
        {
            float duration = DurationSeconds(data, mimeType);
            if (duration > 0)
                filters.Add("afade=t=out:st=" + (String)Math::Max(0.0f, duration - opt.fadeOutMs / 1000.0f) + ":d=" + (String)(opt.fadeOutMs / 1000.0f));
        }

        String cmd = "\"" + ToolPath("ffmpeg") + "\" -y -loglevel error -i \"" + inPath + "\"";
        if (!filters.IsEmpty())
        {
            String chain;
            for (int i = 0; i < filters.Count(); i++)
                chain += (i ? String(",") : String()) + filters[i];
            bool complex = chain.Contains("[");
            cmd += complex ? " -filter_complex \"" + chain + "\"" : " -af \"" + chain + "\"";
        }
        if (opt.sampleRate > 0)
            cmd += " -ar " + (String)opt.sampleRate;
        if (opt.channels > 0)
            cmd += " -ac " + (String)opt.channels;
        if (outExt == "mp3")
            cmd += " -codec:a libmp3lame -b:a 192k";
        else if (outExt == "ogg")
            cmd += " -codec:a libvorbis -q:a 5";
        cmd += " \"" + outPath + "\"";

        int code = system(cmd.Data());
        o2FileSystem.FileDelete(inPath);
        if (code != 0 || !o2FileSystem.IsFileExist(outPath))
        {
            result.error = "ffmpeg failed with exit code " + (String)code;
            o2FileSystem.FileDelete(outPath);
            return result;
        }

        result.data = PipelineUtils::ReadFileBytes(outPath);
        result.mimeType = PipelineUtils::MimeForExtension(outExt);
        result.ok = !result.data.IsEmpty();
        o2FileSystem.FileDelete(outPath);
        return result;
    }

    float DurationSeconds(const String& data, const String& mimeType)
    {
        if (!IsFfmpegAvailable())
            return 0.0f;

        String inExt = PipelineUtils::ExtensionForMime(mimeType);
        if (inExt == "bin") inExt = "mp3";
        String inPath = TempFilePath(inExt);
        String outPath = TempFilePath("txt");
        PipelineUtils::WriteFileBytes(inPath, data);

        String cmd = "\"" + ToolPath("ffprobe") + "\" -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 \"" +
            inPath + "\" > \"" + outPath + "\" 2>/dev/null";
        system(cmd.Data());
        String text = PipelineUtils::ReadFileBytes(outPath).Trimed(" \n\r\t");
        o2FileSystem.FileDelete(inPath);
        o2FileSystem.FileDelete(outPath);
        return (float)atof(text.Data());
    }
}
