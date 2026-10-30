#include "o2Editor/stdafx.h"
#include "MediaPreview.h"

#include "o2/Assets/Types/SoundAsset.h"
#include "o2/Assets/Types/VideoAsset.h"
#include "o2/Render/Render.h"
#include "o2/Render/Texture.h"
#include "o2/Render/Video.h"
#include "o2Editor/Pipeline/PipelineAudio.h"

#include <chrono>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>

namespace Editor
{
    namespace MediaPreview
    {
        namespace
        {
            struct PeaksCache
            {
                std::mutex                           mutex;
                std::map<std::string, Vector<float>> done;
                std::set<std::string>                running;
            };

            PeaksCache& Peaks()
            {
                static PeaksCache* cache = new PeaksCache();
                return *cache;
            }

            // An asset of the project goes by its path, one made in memory by its address
            template<typename _asset_type>
            std::string AssetKey(const AssetRef<_asset_type>& asset, UInt size)
            {
                String path = asset->GetPath();
                if (path.IsEmpty())
                    path = String::Format("%p", (const void*)asset.Get());

                return std::string(path.Data()) + ":" + std::to_string(size);
            }
        }

        Vector<float> GetPeaks(const AssetRef<SoundAsset>& sound)
        {
            if (!sound || !sound->GetData() || sound->GetDataSize() == 0)
                return Vector<float>();

            std::string key = AssetKey(sound, sound->GetDataSize());

            auto& cache = Peaks();
            std::lock_guard<std::mutex> lock(cache.mutex);
            auto found = cache.done.find(key);
            if (found != cache.done.end())
                return found->second;

            if (cache.running.count(key))
                return Vector<float>();

            cache.running.insert(key);
            String data(std::string(sound->GetData(), (size_t)sound->GetDataSize()));
            std::thread([data, key]()
            {
                Vector<float> peaks = PipelineAudio::DecodePeaks(data);
                auto& cache = Peaks();
                std::lock_guard<std::mutex> lock(cache.mutex);
                cache.done[key] = peaks;
                cache.running.erase(key);
            }).detach();

            return Vector<float>();
        }

        Vector<float> WaitPeaks(const AssetRef<SoundAsset>& sound)
        {
            for (int i = 0; i < 2500; i++)
            {
                auto peaks = GetPeaks(sound);
                if (!peaks.IsEmpty() || !sound || !sound->GetData())
                    return peaks;

                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }

            return Vector<float>();
        }

        void DrawWaveform(const RectF& rect, const Vector<float>& peaks, float progress, const Color4& played,
                          const Color4& idle, float barWidth /*= 2.0f*/, float gap /*= 1.0f*/)
        {
            float w = rect.Width(), h = rect.Height(), mid = rect.Center().y;
            progress = Math::Clamp01(progress);

            if (peaks.IsEmpty())
            {
                o2Render.DrawFilledPolygon({ Vec2F(rect.left, mid - 1), Vec2F(rect.left, mid + 1), Vec2F(rect.right, mid + 1),
                                             Vec2F(rect.right, mid - 1) }, idle);

                float x = rect.left + w*progress;
                o2Render.DrawFilledPolygon({ Vec2F(rect.left, mid - 1), Vec2F(rect.left, mid + 1), Vec2F(x, mid + 1),
                                             Vec2F(x, mid - 1) }, played);
                return;
            }

            float step = barWidth + gap;
            int bars = Math::Max(1, (int)(w/step));
            float playedBars = progress*bars;
            Vector<Vertex> vertices;
            Vector<VertexIndex> indices;
            for (int i = 0; i < bars; i++)
            {
                float peak = peaks[Math::Min(peaks.Count() - 1, (int)((float)i/bars*peaks.Count()))];
                float barH = Math::Max(1.0f, peak*(h - 2.0f));
                ULong color = (i < playedBars ? played : idle).ABGR();
                float x = rect.left + i*step;
                VertexIndex base = (VertexIndex)vertices.Count();
                vertices.Add(Vertex(Vec2F(x, mid - barH*0.5f), color, 0, 0));
                vertices.Add(Vertex(Vec2F(x + barWidth, mid - barH*0.5f), color, 0, 0));
                vertices.Add(Vertex(Vec2F(x + barWidth, mid + barH*0.5f), color, 0, 0));
                vertices.Add(Vertex(Vec2F(x, mid + barH*0.5f), color, 0, 0));
                indices.Add(base); indices.Add(base + 1); indices.Add(base + 2);
                indices.Add(base); indices.Add(base + 2); indices.Add(base + 3);
            }

            o2Render.DrawBuffer(PrimitiveType::Polygon, vertices.Data(), vertices.Count(), indices.Data(), indices.Count()/3,
                                o2Render.GetDefaultMaterial(), TextureRef());
        }

        TextureRef GetVideoThumbnail(const AssetRef<VideoAsset>& video)
        {
            static std::map<std::string, TextureRef> cache;
            if (!video)
                return TextureRef();

            std::string key = AssetKey(video, (UInt)video->GetDataSize());
            auto found = cache.find(key);
            if (found != cache.end())
                return found->second;

            TextureRef thumbnail;
            auto player = mmake<Video>();
            player->SetVideoAsset(video);

            Vec2I size = player->GetVideoSize();
            const TextureRef& frame = player->GetTexture();
            if (frame && size.x > 0 && size.y > 0)
            {
                thumbnail = TextureRef(size, TextureFormat::R8G8B8A8, Texture::Usage::Default);
                thumbnail->Copy(*frame.Get(), RectI(Vec2I(), size));
                thumbnail->SetName("Video thumbnail " + video->GetPath());
            }

            cache[key] = thumbnail;
            return thumbnail;
        }

        namespace
        {
            String SizeText(UInt bytes)
            {
                if (bytes < 1024)
                    return (String)(int)bytes + " B";

                if (bytes < 1024*1024)
                    return String::Format("%.1f KB", bytes/1024.0);

                return String::Format("%.1f MB", bytes/(1024.0*1024.0));
            }
        }

        String GetSoundInfo(const SoundAsset& sound)
        {
            int channels = sound.GetChannelsCount();
            String channelsText = channels == 1 ? String("Mono") : channels == 2 ? String("Stereo") : (String)channels + " channels";
            return String::Format("%.2f s · %.1f kHz · ", sound.GetDuration(), sound.GetSampleRate()/1000.0f) + channelsText +
                " · " + SizeText(sound.GetDataSize());
        }

        String GetVideoInfo(const VideoAsset& video)
        {
            Vec2I size = video.GetImageSize();
            return String::Format("%i×%i · %.0f fps · %.2f s · ", size.x, size.y, video.GetFrameRate(), video.GetDuration()) +
                SizeText(video.GetDataSize());
        }

        String FormatTime(float seconds, float duration)
        {
            seconds = Math::Max(0.0f, seconds);
            if (duration < 60.0f)
                return String::Format("%.1f", seconds);

            int total = (int)seconds;
            return String::Format("%d:%02d", total/60, total%60);
        }
    }
}

// --- META ---
// --- END META ---
