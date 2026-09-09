#include "o2Editor/stdafx.h"
#include "PipelineVideo.h"

#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Pipeline/PipelineAudio.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

#include <mutex>

namespace Editor
{
    namespace PipelineVideo
    {
        static const String metaFile = "meta.json";
        static const String atlasFile = "atlas.png";
        static const String audioFile = "audio.wav";
        static const String clipFile = "clip.mp4";

        PreviewInfo LoadPreview(const String& dir)
        {
            PreviewInfo info;
            if (!o2FileSystem.IsFileExist(dir + metaFile) || !o2FileSystem.IsFileExist(dir + atlasFile))
                return info;

            DataDocument doc;
            if (!doc.LoadFromFile(dir + metaFile))
                return info;

            auto number = [&](const char* name)
            {
                auto member = doc.FindMember(name);
                return member ? PipelineUtils::ValueToNumber(*member, 0.0f) : 0.0f;
            };

            info.frameWidth = (int)number("frameWidth");
            info.frameHeight = (int)number("frameHeight");
            info.frameCount = (int)number("frameCount");
            info.columns = (int)number("columns");
            info.fps = number("fps");
            info.duration = number("duration");
            info.hasAudio = number("hasAudio") > 0.5f && o2FileSystem.IsFileExist(dir + audioFile);
            info.atlasPath = dir + atlasFile;
            info.audioPath = info.hasAudio ? dir + audioFile : String();
            info.ok = info.frameWidth > 0 && info.frameHeight > 0 && info.frameCount > 0 && info.columns > 0;
            return info;
        }

        static void SavePreviewMeta(const String& dir, const PreviewInfo& info)
        {
            DataDocument doc;
            doc["frameWidth"] = info.frameWidth;
            doc["frameHeight"] = info.frameHeight;
            doc["frameCount"] = info.frameCount;
            doc["columns"] = info.columns;
            doc["fps"] = info.fps;
            doc["duration"] = info.duration;
            doc["hasAudio"] = info.hasAudio ? 1 : 0;
            doc.SaveToFile(dir + metaFile);
        }

        PreviewInfo BuildPreview(const String& videoData, const String& dir, int frameWidth /*= 256*/, int maxFrames /*= 200*/)
        {
            // Two views of one clip share a folder: the second waits and picks up the first result
            static std::mutex buildMutex;
            std::lock_guard<std::mutex> lock(buildMutex);

            PreviewInfo existing = LoadPreview(dir);
            if (existing.ok)
                return existing;

            PreviewInfo info;
            if (videoData.IsEmpty())
            {
                info.error = "empty clip";
                return info;
            }

            if (!PipelineAudio::IsFfmpegAvailable())
            {
                info.error = "ffmpeg is not installed";
                return info;
            }

            o2FileSystem.FolderCreate(dir, true);
            String clipPath = dir + clipFile;
            PipelineUtils::WriteFileBytes(clipPath, videoData);

            info.duration = PipelineAudio::DurationSeconds(videoData, "video/mp4");
            float fps = 12.0f;
            if (info.duration > 0.0f && info.duration * fps > (float)maxFrames)
                fps = (float)maxFrames / info.duration;

            info.fps = fps;

            String framesDir = dir + "frames/";
            o2FileSystem.FolderRemove(framesDir, true);
            o2FileSystem.FolderCreate(framesDir, true);

            String ffmpeg = "\"" + PipelineAudio::ToolPath("ffmpeg") + "\"";
            String framesCmd = ffmpeg + " -y -loglevel error -i \"" + clipPath + "\" -vf \"fps=" + (String)fps + ",scale=" +
                (String)frameWidth + ":-2\" -q:v 4 \"" + framesDir + "%04d.jpg\"";

            if (system(framesCmd.Data()) != 0)
            {
                info.error = "ffmpeg failed to extract frames";
                o2FileSystem.FileDelete(clipPath);
                return info;
            }

            String audioPath = dir + audioFile;
            String audioCmd = ffmpeg + " -y -loglevel quiet -i \"" + clipPath + "\" -vn -ac 2 -ar 44100 -acodec pcm_s16le \"" + audioPath + "\"";
            system(audioCmd.Data());
            info.hasAudio = o2FileSystem.IsFileExist(audioPath) && o2FileSystem.GetFileInfo(audioPath).size > 1000;
            if (!info.hasAudio)
                o2FileSystem.FileDelete(audioPath);

            Vector<String> framePaths;
            for (auto& file : o2FileSystem.GetFolderInfo(framesDir).files)
            {
                if (file.path.EndsWith(".jpg"))
                    framePaths.Add(file.path);
            }
            framePaths.Sort([](const String& a, const String& b) { return a < b; });

            Vector<Ref<Bitmap>> frames;
            for (auto& path : framePaths)
            {
                auto bitmap = mmake<Bitmap>();
                if (bitmap->Load(path, Bitmap::ImageType::Auto) && bitmap->GetFormat() == PixelFormat::R8G8B8A8)
                {
                    if (frames.IsEmpty() || frames[0]->GetSize() == bitmap->GetSize())
                        frames.Add(bitmap);
                }
            }

            o2FileSystem.FolderRemove(framesDir, true);
            o2FileSystem.FileDelete(clipPath);

            if (frames.IsEmpty())
            {
                info.error = "no frames extracted";
                return info;
            }

            int count = frames.Count();
            Vec2I frameSize = frames[0]->GetSize();
            int columns = Math::Max(1, (int)Math::Ceil(Math::Sqrt((float)count)));
            int rows = (count + columns - 1) / columns;
            Vec2I atlasSize(columns * frameSize.x, rows * frameSize.y);

            auto atlas = mmake<Bitmap>(PixelFormat::R8G8B8A8, atlasSize);
            atlas->Fill(Color4(0, 0, 0, 255));
            for (int i = 0; i < count; i++)
            {
                int col = i % columns;
                int row = i / columns;
                atlas->CopyImage(*frames[i], Vec2I(col * frameSize.x, atlasSize.y - (row + 1) * frameSize.y));
            }

            if (!atlas->Save(dir + atlasFile, Bitmap::ImageType::Png))
            {
                info.error = "failed to write the frame atlas";
                return info;
            }

            info.frameWidth = frameSize.x;
            info.frameHeight = frameSize.y;
            info.frameCount = count;
            info.columns = columns;
            if (info.duration <= 0.0f)
                info.duration = (float)count / fps;

            info.atlasPath = dir + atlasFile;
            info.audioPath = info.hasAudio ? audioPath : String();
            info.ok = true;
            SavePreviewMeta(dir, info);
            return info;
        }

        RectI FrameRect(const PreviewInfo& info, int frame)
        {
            if (info.frameCount <= 0 || info.columns <= 0)
                return RectI();

            int index = Math::Clamp(frame, 0, info.frameCount - 1);
            int col = index % info.columns;
            int row = index / info.columns;
            int rows = (info.frameCount + info.columns - 1) / info.columns;
            int bottom = rows * info.frameHeight - (row + 1) * info.frameHeight;
            return RectI(col * info.frameWidth, bottom + info.frameHeight, (col + 1) * info.frameWidth, bottom);
        }
    }
}
