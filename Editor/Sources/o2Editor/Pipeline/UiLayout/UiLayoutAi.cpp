#include "o2Editor/stdafx.h"
#include "UiLayoutAi.h"

#include "o2/Utils/Threading/Mutex.h"
#include "o2/Utils/Threading/ScopeLock.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/PipelineValue.h"
#include "o2Editor/Pipeline/Providers/AiRouter.h"

namespace Editor::UiLayout
{
    namespace
    {
        Mutex    replayMutex;
        AiReplay replayHook;

        bool Replay(const String& tag, const String& ext, const String& prompt, const Vector<String>& images, String& data, bool& hooked)
        {
            ScopeLock<Mutex> lock(replayMutex);
            hooked = !replayHook.IsEmpty();
            if (!hooked)
                return false;

            AiReplayCall call;
            call.tag = tag;
            call.ext = ext;
            call.prompt = prompt;
            call.images = images;
            return replayHook(call, data);
        }

        String MemoKey(const String& prefix, const Vector<String>& parts)
        {
            String payload;
            for (auto& part : parts)
            {
                payload += part;
                payload += '\0';
            }

            static const char* hex = "0123456789abcdef";
            String digest = PipelineUtils::Sha256(payload), out = prefix;
            for (int i = 0; i < 20; i++)
            {
                unsigned char c = (unsigned char)digest[i];
                out += hex[c >> 4];
                out += hex[c & 15];
            }
            return out;
        }

        const char* dot = " \xC2\xB7 ";
    }

    void AiCtx::Log(const String& message) const
    {
        ScopeLock<Mutex> lock(logMutex);
        if (exec)
            exec->Log(message);
    }

    void AiCtx::Record(const String& tag, const String& ext, const String& data) const
    {
        ScopeLock<Mutex> lock(logMutex);
        if (record)
            record(tag, ext, data);
    }

    Ref<AiCtx> ParallelAiCtx(const Ref<AiCtx>& ai, Mutex* mutex)
    {
        auto worker = mmake<AiCtx>();
        worker->memo = ai->memo;
        worker->fresh = ai->fresh;
        worker->textModel = ai->textModel;
        worker->imageModel = ai->imageModel;
        worker->seed = ai->seed;
        worker->record = ai->record;
        if (!ai->exec)
            return worker;

        auto& from = *ai->exec;
        auto exec = mmake<PipelineExecContext>();
        exec->settings = from.settings;
        exec->seed = from.seed;
        exec->cachedOnly = from.cachedOnly;
        exec->pipelineId = from.pipelineId;
        exec->assetsPath = from.assetsPath;
        exec->nodeId = from.nodeId;
        exec->workDir = from.workDir;
        exec->fresh = from.fresh;
        exec->memoGet = from.memoGet;
        exec->memoPut = from.memoPut;
        auto log = from.log;
        auto cancelled = from.isCancelled;
        auto retry = from.signalRetry;
        exec->log = [log, mutex](const String& message) { ScopeLock<Mutex> lock(*mutex); if (log) log(message); };
        exec->isCancelled = [cancelled, mutex]() { ScopeLock<Mutex> lock(*mutex); return cancelled && cancelled(); };
        exec->signalRetry = [retry, mutex](int attempt, int max, int status, const String& reason)
        {
            ScopeLock<Mutex> lock(*mutex);
            if (retry)
                retry(attempt, max, status, reason);
        };
        worker->exec = exec;
        return worker;
    }

    void SetAiReplay(const AiReplay& replay)
    {
        ScopeLock<Mutex> lock(replayMutex);
        replayHook = replay;
    }

    String TextMemoKey(const String& model, const String& prompt, const Vector<String>& images)
    {
        Vector<String> parts = { "text", model, prompt };
        parts.Add(images);
        return MemoKey("uil-t-", parts);
    }

    String ImageMemoKey(const String& model, const String& prompt, const String& aspectRatio, int seed, const Vector<String>& refs)
    {
        Vector<String> parts = { "image", model, prompt, aspectRatio, seed >= 0 ? (String)seed : String() };
        parts.Add(refs);
        return MemoKey("uil-i-", parts);
    }

    String ImageMimeOf(const String& bytes)
    {
        return bytes.Length() >= 2 && (unsigned char)bytes[0] == 0xFF && (unsigned char)bytes[1] == 0xD8 ? "image/jpeg" : "image/png";
    }

    Coroutine<AiAnswer> AiText(const Ref<AiCtx>& ai, String prompt, Vector<String> images, String tag)
    {
        AiAnswer res;
        bool hooked = false;
        String replayed;
        if (Replay(tag, ".txt", prompt, images, replayed, hooked))
        {
            ai->replayed.FetchAdd(1);
            ai->Log("imageToUi" + String(dot) + tag + ": replayed answer");
            res.ok = true;
            res.data = replayed;
            co_return res;
        }

        if (hooked)
        {
            res.error = "imageToUi: no recorded answer for \"" + tag + "\"";
            co_return res;
        }

        String key = TextMemoKey(ai->textModel, prompt, images);
        if (ai->memo.get && !ai->fresh)
        {
            String hit;
            if (ai->memo.get(key, ".txt", hit))
            {
                ai->replayed.FetchAdd(1);
                ai->Log("imageToUi" + String(dot) + tag + ": replayed answer");
                ai->Record(tag, ".txt", hit);
                res.ok = true;
                res.data = hit;
                co_return res;
            }
        }

        ai->textCalls.FetchAdd(1);
        ai->Log("imageToUi" + String(dot) + tag + ": asking " + ai->textModel);

        Vector<AiImageRef> refs;
        for (auto& image : images)
            refs.Add({ ImageMimeOf(image), image });

        AiTextResult answer = co_await AiRouter::GenerateText(ai->exec, ai->textModel, prompt, refs);
        if (!answer.ok)
        {
            res.error = answer.error.IsEmpty() ? String("the vision model failed") : answer.error;
            co_return res;
        }

        if (ai->memo.put)
            ai->memo.put(key, ".txt", answer.text);
        ai->Record(tag, ".txt", answer.text);

        res.ok = true;
        res.data = answer.text;
        co_return res;
    }

    Coroutine<AiAnswer> AiImage(const Ref<AiCtx>& ai, String prompt, Vector<String> refs, String tag, String aspectRatio, int seed /*= -2*/)
    {
        if (seed == -2)
            seed = ai->seed;

        AiAnswer res;
        bool hooked = false;
        String replayed;
        if (Replay(tag, ".png", prompt, refs, replayed, hooked))
        {
            ai->replayed.FetchAdd(1);
            res.ok = true;
            res.data = replayed;
            co_return res;
        }

        if (hooked)
        {
            res.error = "imageToUi: no recorded answer for \"" + tag + "\"";
            co_return res;
        }

        String key = ImageMemoKey(ai->imageModel, prompt, aspectRatio, seed, refs);
        if (ai->memo.get && !ai->fresh)
        {
            String hit;
            if (ai->memo.get(key, ".png", hit))
            {
                ai->replayed.FetchAdd(1);
                ai->Record(tag, ".png", hit);
                res.ok = true;
                res.data = hit;
                co_return res;
            }
        }

        Vector<AiImageRef> references;
        for (auto& ref : refs)
            references.Add({ "image/png", ref });

        AiImageOptions options;
        options.aspectRatio = aspectRatio;

        // An image model now and then answers with text only; that is worth another try
        AiBytesResult answer;
        for (int attempt = 1;; attempt++)
        {
            ai->imageCalls.FetchAdd(1);
            ai->Log("imageToUi" + String(dot) + tag + ": rendering with " + ai->imageModel + (attempt > 1 ? " (try " + (String)attempt + ")" : String()));
            answer = co_await AiRouter::GenerateImage(ai->exec, ai->imageModel, prompt, references, seed, false, options);
            if (answer.ok)
                break;

            if (attempt >= 3 || !answer.error.ToLowerCase().Contains("no image"))
            {
                res.error = answer.error.IsEmpty() ? String("the image model failed") : answer.error;
                co_return res;
            }
        }

        String png = answer.data;
        if (answer.mimeType != "image/png")
        {
            auto bitmap = DecodeImageBytes(answer.data);
            if (!bitmap)
            {
                res.error = "the image model answered with an undecodable image (" + answer.mimeType + ")";
                co_return res;
            }
            png = EncodeBitmapPng(*bitmap);
        }

        if (ai->memo.put)
            ai->memo.put(key, ".png", png);
        ai->Record(tag, ".png", png);

        res.ok = true;
        res.data = png;
        co_return res;
    }
}
