#include "o2Editor/stdafx.h"
#include "PipelineMediaViews.h"

#include "miniaudio.h"
#include "o2Editor/Windows/PipelineWindow/PipelineDraw.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"
#include <map>
#include <mutex>
#include <string>
#include <thread>

#include "o2/Assets/Types/SoundAsset.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/HorizontalProgress.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Sound/SoundPlayer.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

#include <cstring>
#include <map>
#include <thread>
#include "o2/Render/VectorSprite.h"

namespace Editor
{
    const float PipelineMediaControls::height = 22.0f;
    const float PipelineAudioView::height = 38.0f;

    static String FormatTime(float seconds)
    {
        int total = (int)Math::Max(0.0f, seconds);
        return String::Format("%d:%02d", total / 60, total % 60);
    }

    // Peaks of the clip for the waveform: 320 buckets of the loudest sample, normalised; decoded once per clip off the
    // main thread (AudioPlayer.tsx BUCKETS, peaksCache)
    namespace
    {
        struct PeaksCache
        {
            std::mutex                         mutex;
            std::map<std::string, Vector<float>> done;
            std::map<std::string, bool>        running;
        };

        PeaksCache& Peaks()
        {
            static PeaksCache* cache = new PeaksCache();
            return *cache;
        }

        std::string PeaksKey(const String& data)
        {
            // Length and a sparse sample of the bytes tell clips apart without hashing megabytes
            std::string key = std::to_string(data.Length()) + ":";
            int step = Math::Max(1, data.Length() / 256);
            for (int i = 0; i < data.Length(); i += step)
                key += (char)('a' + ((unsigned char)data[i] % 26));
            return key;
        }

        Vector<float> DecodePeaks(const String& data)
        {
            const int buckets = 320;
            Vector<float> peaks;
            ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
            ma_decoder decoder;
            if (ma_decoder_init_memory(data.Data(), (size_t)data.Length(), &config, &decoder) != MA_SUCCESS)
                return peaks;

            ma_uint64 frames = 0;
            ma_decoder_get_length_in_pcm_frames(&decoder, &frames);
            int channels = Math::Max(1, (int)decoder.outputChannels);
            if (frames == 0)
            {
                ma_decoder_uninit(&decoder);
                return peaks;
            }

            peaks.Resize(buckets);
            for (auto& p : peaks)
                p = 0.0f;
            ma_uint64 per = Math::Max<ma_uint64>(1, frames / buckets);
            std::vector<float> chunk(4096 * channels);
            ma_uint64 index = 0;
            while (true)
            {
                ma_uint64 read = 0;
                if (ma_decoder_read_pcm_frames(&decoder, chunk.data(), 4096, &read) != MA_SUCCESS || read == 0)
                    break;
                for (ma_uint64 f = 0; f < read; f++, index++)
                {
                    int bucket = (int)Math::Min<ma_uint64>(buckets - 1, index / per);
                    for (int c = 0; c < channels; c++)
                        peaks[bucket] = Math::Max(peaks[bucket], Math::Abs(chunk[f * channels + c]));
                }
            }
            ma_decoder_uninit(&decoder);

            float max = 0.0f;
            for (float p : peaks)
                max = Math::Max(max, p);
            if (max > 1e-4f)
            {
                for (auto& p : peaks)
                    p /= max;
            }
            return peaks;
        }
    }

    PipelineMediaControls::PipelineMediaControls(RefCounter* refCounter):
        Widget(refCounter)
    {
        auto& theme = PipelineTheme::Get();
        layout->minHeight = height;

        // Play: an accent circle with the ink glyph (.ap-play 24 / .tp-play 22)
        mPlayButton = mmake<Button>();
        mPlayButton->name = "play";
        PipelineControls::AddFillLayer(*mPlayButton, "back", theme.accent, 12.0f, Layout::BothStretch(), -2.0f);
        auto hover = PipelineControls::AddFillLayer(*mPlayButton, "hover", theme.accentHover, 12.0f, Layout::BothStretch(), -1.0f);
        hover->transparency = 0.0f;
        mPlayButton->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.12f))->offStateAnimationSpeed = 0.5f;
        auto glyph = mmake<VectorSprite>("ui/pipeline/al_play.svg");
        glyph->color = theme.accentInk;
        mPlayButton->AddLayer("icon", glyph, Layout::Based(BaseCorner::Center, Vec2F(10, 10), Vec2F(0.5f, 0)));
        mPlayButton->onClick = [this]() { if (onPlayPause) onPlayPause(); };
        AddChild(mPlayButton);

        // Seek: the waveform or the 3 px bar, clicked or dragged
        mSeek = mmake<HorizontalProgress>();
        mSeek->name = "seek";
        PipelineControls::AddFillLayer(*mSeek, "back", Color4(0, 0, 0, 1), 2.0f);
        PipelineControls::AddFillLayer(*mSeek, "bar", Color4(0, 0, 0, 1), 2.0f);
        mSeek->SetValueRange(0.0f, 1.0f);
        mSeek->onChangeByUser = [this](float value)
        {
            mSeeking = true;
            if (onSeek)
                onSeek(value);
        };
        AddChild(mSeek);

        auto time = mmake<Text>(PipelineTheme::MonoFont());
        time->height = PipelineTheme::Size(10.5f);
        time->horAlign = HorAlign::Right;
        time->verAlign = VerAlign::Middle;
        time->color = theme.fg3;
        mTimeLayer = AddLayer("time", time, Layout::BothStretch());

        // Loop: a 12 px glyph, the accent while on (.ap-loop)
        mLoopToggle = mmake<Toggle>();
        mLoopToggle->name = "loop";
        auto loopIcon = mmake<VectorSprite>("ui/pipeline/btn_loop.svg");
        loopIcon->color = theme.fg3;
        mLoopToggle->AddLayer("icon", loopIcon, Layout::BothStretch(-1, -1, -1, -1));
        auto loopOn = mmake<VectorSprite>("ui/pipeline/btn_loop.svg");
        loopOn->color = theme.accent;
        mLoopToggle->AddLayer("iconOn", loopOn, Layout::BothStretch(-1, -1, -1, -1))->transparency = 0.0f;
        mLoopToggle->AddState("value", AnimationClip::EaseInOut("layer/iconOn/transparency", 0.0f, 1.0f, 0.12f))->offStateAnimationSpeed = 1.0f;
        mLoopToggle->onToggleByUser = [this](bool value) { if (onLoopChanged) onLoopChanged(value); };
        AddChild(mLoopToggle);

        SetMode(Mode::Transport);
        SetPlaying(false);
    }

    void PipelineMediaControls::SetMode(Mode mode)
    {
        mMode = mode;
        LayoutParts();
    }

    void PipelineMediaControls::SetFormat(const String& format)
    {
        mFormat = format;
        SetTime(mTime, mDuration);
    }

    void PipelineMediaControls::SetPeaks(const Vector<float>& peaks)
    {
        mPeaks = peaks;
    }

    void PipelineMediaControls::LayoutParts()
    {
        bool audio = mMode == Mode::Audio;
        float play = audio ? 24.0f : 22.0f;
        *mPlayButton->layout = WidgetLayout(Vec2F(0, 0.5f), Vec2F(0, 0.5f), Vec2F(0, -play * 0.5f), Vec2F(play, play * 0.5f));
        for (auto name : { "back", "hover" })
        {
            if (auto layer = mPlayButton->GetLayer(name))
                if (auto rect = DynamicCast<PipelineRoundedRect>(layer->GetDrawable()))
                    rect->radius = play * 0.5f;
        }

        // Time "0:00 / 0:02 · WAV" takes its text width; the loop glyph ends the audio row
        float timeWidth = Math::Ceil(PipelineControls::TextWidth("0:00 / 0:00" + (mFormat.IsEmpty() ? String() : " · " + mFormat), 10.5f)) + 2.0f;
        if (auto text = DynamicCast<Text>(mTimeLayer->GetDrawable()))
        {
            if (auto font = text->GetFont())
                timeWidth = Math::Ceil(Text::GetTextSize("0:00 / 0:00" + (mFormat.IsEmpty() ? String() : " · " + mFormat), font, text->height, Vec2F(), HorAlign::Left, VerAlign::Top, false).x) + 2.0f;
        }
        float loop = audio ? 12.0f + 8.0f : 0.0f;
        mTimeLayer->layout = Layout(Vec2F(1, 0), Vec2F(1, 1), Vec2F(-loop - timeWidth, 0), Vec2F(-loop, 0));
        mLoopToggle->enabled = audio;
        *mLoopToggle->layout = WidgetLayout(Vec2F(1, 0.5f), Vec2F(1, 0.5f), Vec2F(-12, -6), Vec2F(0, 6));

        float seekHeight = audio ? 22.0f : 3.0f;
        *mSeek->layout = WidgetLayout(Vec2F(0, 0.5f), Vec2F(1, 0.5f), Vec2F(play + 8.0f, -seekHeight * 0.5f),
                                      Vec2F(-loop - timeWidth - 8.0f, seekHeight * 0.5f));
        auto& theme = PipelineTheme::Get();
        if (auto back = mSeek->GetLayer("back"))
        {
            back->GetDrawable()->color = audio ? Color4(0, 0, 0, 255) : Color4(theme.border.r, theme.border.g, theme.border.b, 255);
            back->transparency = audio ? 0.004f : (theme.dark ? 0.14f : 1.0f);
        }
        if (auto bar = mSeek->GetLayer("bar"))
        {
            bar->GetDrawable()->color = Color4(theme.accent.r, theme.accent.g, theme.accent.b, 255);
            bar->transparency = audio ? 0.0f : 1.0f;
        }
    }

    void PipelineMediaControls::Draw()
    {
        Widget::Draw();
        if (mMode != Mode::Audio || PipelineControls::IsFarView() || !mResEnabledInHierarchy)
            return;

        // 2 px bars 1 px apart, the played ones in the audio colour; a 2 px line without peaks
        auto& theme = PipelineTheme::Get();
        RectF area = mSeek->layout->GetWorldRect();
        float w = area.Width(), h = area.Height(), mid = area.Center().y;
        float progress = mDuration > 0.0f ? Math::Clamp01(mTime / mDuration) : 0.0f;
        Color4 played = theme.portAudio, idle = theme.border;
        if (mPeaks.IsEmpty())
        {
            o2Render.DrawFilledPolygon({ Vec2F(area.left, mid - 1), Vec2F(area.left, mid + 1), Vec2F(area.right, mid + 1), Vec2F(area.right, mid - 1) }, idle);
            float x = area.left + w * progress;
            o2Render.DrawFilledPolygon({ Vec2F(area.left, mid - 1), Vec2F(area.left, mid + 1), Vec2F(x, mid + 1), Vec2F(x, mid - 1) }, played);
            return;
        }

        int bars = Math::Max(1, (int)(w / 3.0f));
        float playedBars = progress * bars;
        Vector<Vertex> vertices;
        Vector<VertexIndex> indices;
        for (int i = 0; i < bars; i++)
        {
            float peak = mPeaks[Math::Min(mPeaks.Count() - 1, (int)((float)i / bars * mPeaks.Count()))];
            float barH = Math::Max(1.0f, peak * (h - 2.0f));
            ULong color = (i < playedBars ? played : idle).ABGR();
            float x = area.left + i * 3.0f;
            VertexIndex base = (VertexIndex)vertices.Count();
            vertices.Add(Vertex(Vec2F(x, mid - barH * 0.5f), color, 0, 0));
            vertices.Add(Vertex(Vec2F(x + 2.0f, mid - barH * 0.5f), color, 0, 0));
            vertices.Add(Vertex(Vec2F(x + 2.0f, mid + barH * 0.5f), color, 0, 0));
            vertices.Add(Vertex(Vec2F(x, mid + barH * 0.5f), color, 0, 0));
            indices.Add(base); indices.Add(base + 1); indices.Add(base + 2);
            indices.Add(base); indices.Add(base + 2); indices.Add(base + 3);
        }
        o2Render.DrawBuffer(PrimitiveType::Polygon, vertices.Data(), vertices.Count(), indices.Data(), indices.Count() / 3,
                            o2Render.GetDefaultMaterial(), TextureRef());
    }

    void PipelineMediaControls::SetPlaying(bool playing)
    {
        mPlaying = playing;
        if (auto icon = mPlayButton->SetLayerImage("icon", playing ? "ui/pipeline/btn_pause.svg" : "ui/pipeline/al_play.svg"))
            icon->color = PipelineTheme::Get().accentInk;
    }

    void PipelineMediaControls::SetTime(float time, float duration)
    {
        mTime = time;
        mDuration = duration;
        if (!mSeeking)
        {
            mSeek->SetValueRange(0.0f, Math::Max(duration, 0.001f));
            mSeek->SetValue(Math::Clamp(time, 0.0f, Math::Max(duration, 0.001f)));
        }

        if (auto text = DynamicCast<Text>(mTimeLayer->GetDrawable()))
            text->text = FormatTime(time) + " / " + FormatTime(duration) + (mFormat.IsEmpty() ? String() : " · " + mFormat);
    }

    void PipelineMediaControls::SetLoop(bool loop)
    {
        mLoopToggle->SetValue(loop);
    }

    bool PipelineMediaControls::IsLoop() const
    {
        return mLoopToggle->GetValue();
    }

    void PipelineMediaControls::SetVolume(float volume)
    {
        mVolumeValue = Math::Clamp01(volume);
    }

    float PipelineMediaControls::GetVolume() const
    {
        return mVolumeValue;
    }

    void PipelineMediaControls::SetActive(bool active)
    {
        mPlayButton->interactable = active;
        mSeek->interactable = active;
        mLoopToggle->interactable = active;
        mPlayButton->transparency = active ? 1.0f : 0.5f;
    }

    void PipelineMediaControls::Update(float dt)
    {
        Widget::Update(dt);
        if (mSeeking && !o2Input.IsCursorDown())
            mSeeking = false;
    }

    // Returns the sound asset of a clip. An asset stays in the assets cache for the session, so the cards share one per
    // clip instead of making one at every build
    static Ref<SoundAsset> SharedSoundAsset(const String& bytes)
    {
        static auto* sounds = new std::multimap<UInt64, Ref<SoundAsset>>();

        // Sampled key: the bytes are compared on a match
        UInt64 key = 1469598103934665603ull ^ (UInt64)bytes.Length();
        int count = bytes.Length();
        for (int i = 0; i < 256 && count > 0; i++)
            key = (key ^ (UInt8)bytes[(int)(((Int64)count*i)/256)]) * 1099511628211ull;

        auto range = sounds->equal_range(key);
        for (auto it = range.first; it != range.second; ++it)
        {
            auto& sound = it->second;
            if (sound->GetDataSize() == (UInt)count && std::memcmp(sound->GetData(), bytes.Data(), count) == 0)
                return sound;
        }

        auto sound = mmake<SoundAsset>();
        sound->SetData(const_cast<char*>(bytes.Data()), (UInt)count);
        sounds->insert({ key, sound });
        return sound;
    }

    PipelineAudioView::PipelineAudioView(RefCounter* refCounter):
        Widget(refCounter)
    {
        auto& theme = PipelineTheme::Get();
        layout->minHeight = height;

        // .audio-player: field fill and line border, r 9, padding 6 / 8
        PipelineControls::AddFillLayer(*this, "back", theme.field, PipelineControls::fieldRadius, Layout::BothStretch(), -2.0f);
        PipelineControls::AddBorderLayer(*this, "border", theme.line, PipelineControls::fieldRadius, 1.0f, Layout::BothStretch(), -1.0f);

        mTitleLabel = PipelineControls::MakeLabel("", true);
        mTitleLabel->name = "title";
        mTitleLabel->horAlign = HorAlign::Middle;
        *mTitleLabel->layout = WidgetLayout::BothStretch(8, 6, 8, 6);
        AddChild(mTitleLabel);

        mInfoLabel = PipelineControls::MakeLabel("", true);
        mInfoLabel->name = "info";
        mInfoLabel->enabled = false;
        AddChild(mInfoLabel);

        mControls = mmake<PipelineMediaControls>();
        mControls->name = "controls";
        mControls->SetMode(PipelineMediaControls::Mode::Audio);
        *mControls->layout = WidgetLayout::BothStretch(8, 6, 8, 6);
        auto weakThis = WeakRef(this);
        mControls->onPlayPause = [weakThis]() { if (auto self = weakThis.Lock()) self->TogglePlay(); };
        mControls->onSeek = [weakThis](float time) { if (auto self = weakThis.Lock()) self->Seek(time); };
        mControls->onLoopChanged = [weakThis](bool) { if (auto self = weakThis.Lock()) self->ApplyPlayerSettings(); };
        mControls->onVolumeChanged = [weakThis](float) { if (auto self = weakThis.Lock()) self->ApplyPlayerSettings(); };
        AddChild(mControls);

        RefreshControls();
    }

    PipelineAudioView::~PipelineAudioView()
    {
        if (mPlayer)
            mPlayer->Stop();
    }

    void PipelineAudioView::SetAudio(const PipelineValue& value, const String& title)
    {
        if (mPlayer)
        {
            mPlayer->Stop();
            mPlayer = nullptr;
        }
        mSoundAsset = nullptr;
        mPlaying = false;
        mDuration = 0.0f;
        mValue = value;
        mHasAudio = value.IsAudio() && !value.data.IsEmpty();
        mTitleLabel->text = mHasAudio ? String() : mHint;
        mTitleLabel->enabled = !mHasAudio;
        mControls->enabled = mHasAudio;
        mInfoLabel->text = mHasAudio ? (String)(value.data.Length() / 1024) + " KB" : String();
        String format = value.mimeType.SubStr(value.mimeType.Find("/") + 1).ToUpperCase();
        if (format.StartsWith("X-"))
            format = format.SubStr(2);
        if (format == "MPEG")
            format = "MP3";
        mControls->SetFormat(mHasAudio ? format : String());
        mControls->SetPeaks({});
        mPeaksShown = false;
        mPeaksKey = mHasAudio ? PeaksKey(value.data) : std::string();
        if (mHasAudio)
        {
            EnsurePlayer();
            RequestPeaks();
        }

        RefreshControls();
    }

    void PipelineAudioView::SetHint(const String& hint)
    {
        mHint = hint;
        if (!mHasAudio)
            mTitleLabel->text = hint;
    }

    void PipelineAudioView::RequestPeaks()
    {
        auto& cache = Peaks();
        std::lock_guard<std::mutex> lock(cache.mutex);
        auto found = cache.done.find(mPeaksKey);
        if (found != cache.done.end())
        {
            mControls->SetPeaks(found->second);
            return;
        }

        if (cache.running[mPeaksKey])
            return;

        cache.running[mPeaksKey] = true;
        String data = mValue.data;
        std::string key = mPeaksKey;
        std::thread([data, key]()
        {
            Vector<float> peaks = DecodePeaks(data);
            auto& cache = Peaks();
            std::lock_guard<std::mutex> lock(cache.mutex);
            cache.done[key] = peaks;
            cache.running.erase(key);
        }).detach();
    }

    bool PipelineAudioView::EnsurePlayer()
    {
        if (mPlayer)
            return true;

        if (!mHasAudio)
            return false;

        mSoundAsset = SharedSoundAsset(mValue.data);

        mPlayer = mmake<SoundPlayer>();
        mPlayer->SetSound(AssetRef<SoundAsset>(mSoundAsset));
        mDuration = Math::Max(0.0f, mPlayer->GetDuration());
        ApplyPlayerSettings();
        return true;
    }

    void PipelineAudioView::ApplyPlayerSettings()
    {
        if (!mPlayer)
            return;

        mPlayer->SetLoop(mControls->IsLoop() ? Loop::Repeat : Loop::None);
        mPlayer->SetVolume(mControls->GetVolume());
    }

    void PipelineAudioView::Play()
    {
        if (!EnsurePlayer())
            return;

        ApplyPlayerSettings();
        if (mDuration > 0.0f && mPlayer->GetTime() >= mDuration - 0.01f)
            mPlayer->RewindAndPlay();
        else
            mPlayer->Play();

        mPlaying = true;
        RefreshControls();
    }

    void PipelineAudioView::Pause()
    {
        if (mPlayer)
            mPlayer->Stop();

        mPlaying = false;
        RefreshControls();
    }

    void PipelineAudioView::TogglePlay()
    {
        if (mPlaying)
            Pause();
        else
            Play();
    }

    void PipelineAudioView::Seek(float time)
    {
        if (!EnsurePlayer())
            return;

        mPlayer->SetTime(Math::Clamp(time, 0.0f, Math::Max(mDuration, 0.0f)));
        RefreshControls();
    }

    float PipelineAudioView::GetTime() const
    {
        return mPlayer ? Math::Clamp(mPlayer->GetTime(), 0.0f, Math::Max(mDuration, 0.0f)) : 0.0f;
    }

    void PipelineAudioView::RefreshControls()
    {
        mControls->SetActive(mHasAudio);
        mControls->SetPlaying(mPlaying);
        mControls->SetTime(GetTime(), mDuration);
    }

    void PipelineAudioView::Update(float dt)
    {
        Widget::Update(dt);
        if (!mPeaksKey.empty() && !mPeaksShown)
        {
            auto& cache = Peaks();
            std::lock_guard<std::mutex> lock(cache.mutex);
            auto found = cache.done.find(mPeaksKey);
            if (found != cache.done.end())
            {
                mControls->SetPeaks(found->second);
                mPeaksShown = true;
            }
        }
        if (mPlayer)
        {
            mPlayer->Update(dt);
            if (mPlaying && !mPlayer->IsPlaying())
                mPlaying = false;
        }

        RefreshControls();
    }

    PipelineVideoView::PipelineVideoView(RefCounter* refCounter):
        Widget(refCounter)
    {
        layout->minHeight = 120;

        // The picture card of the result (176) with the transport row 7 below it (VideoBlock in nodeRenderers.tsx)
        mHintLabel = PipelineControls::MakeLabel("", true);
        mHintLabel->name = "hint";
        mHintLabel->horAlign = HorAlign::Middle;
        mHintLabel->verAlign = VerAlign::Middle;
        *mHintLabel->layout = WidgetLayout(Vec2F(0, 0), Vec2F(1, 1), Vec2F(8, PipelineMediaControls::height + 7 + 8), Vec2F(-8, -8));
        AddChild(mHintLabel);

        mControls = mmake<PipelineMediaControls>();
        mControls->name = "controls";
        mControls->SetMode(PipelineMediaControls::Mode::Transport);
        *mControls->layout = WidgetLayout(Vec2F(0, 0), Vec2F(1, 0), Vec2F(0, 0), Vec2F(0, PipelineMediaControls::height));
        auto weakThis = WeakRef(this);
        mControls->onPlayPause = [weakThis]() { if (auto self = weakThis.Lock()) self->TogglePlay(); };
        mControls->onSeek = [weakThis](float time) { if (auto self = weakThis.Lock()) self->Seek(time); };
        mControls->onVolumeChanged = [weakThis](float volume)
        {
            if (auto self = weakThis.Lock())
            {
                if (self->mPlayer)
                    self->mPlayer->SetVolume(volume);
            }
        };
        AddChild(mControls);

        mFrame = mmake<Sprite>();
        onDraw = [this]() { DrawFrame(); };

        RefreshControls();
    }

    PipelineVideoView::~PipelineVideoView()
    {
        if (mPlayer)
            mPlayer->Stop();
    }

    void PipelineVideoView::SetVideo(const PipelineValue& value, const String& cacheDir)
    {
        bool sameVideo = mHasVideo && value.IsVideo() && cacheDir == mCacheDir && value.data.Length() == mValue.data.Length();
        if (sameVideo)
            return;

        ClearPreview();
        mValue = value;
        mCacheDir = cacheDir;
        mHasVideo = value.IsVideo() && !value.data.IsEmpty();
        if (mHasVideo)
        {
            // A stored preview still has a big atlas to read, off the main thread as well
            auto existing = PipelineVideo::LoadPreview(cacheDir);
            StartExtraction(existing.ok ? &existing : nullptr);
        }

        RefreshControls();
    }

    void PipelineVideoView::SetHint(const String& hint)
    {
        mHint = hint;
        if (!mHasVideo)
            mHintLabel->text = hint;
    }

    void PipelineVideoView::StartExtraction(const PipelineVideo::PreviewInfo* existing /*= nullptr*/)
    {
        auto job = std::make_shared<ExtractJob>();
        job->dir = mCacheDir;
        job->load = existing != nullptr;
        if (existing)
            job->result = *existing;
        else
            job->data = mValue.data;
        mJob = job;
        mHintLabel->text = existing ? "loading.." : "Preparing preview...";
        mHintLabel->enabled = true;

        std::thread([job]()
        {
            if (!job->load)
                job->result = PipelineVideo::BuildPreview(job->data, job->dir);

            // Read from memory: a bitmap loaded from a file names its texture after it, and the render reloads every named
            // texture on each assets rebuild
            if (job->result.ok)
            {
                String bytes = PipelineUtils::ReadFileBytes(job->result.atlasPath);
                auto atlas = mmake<Bitmap>();
                if (!bytes.IsEmpty() && atlas->LoadFromMemory((const UInt8*)bytes.Data(), (UInt)bytes.Length(), Bitmap::ImageType::Png))
                    job->atlas = std::move(atlas);
            }
            job->done = true;
        }).detach();
    }

    void PipelineVideoView::ApplyPreview(const PipelineVideo::PreviewInfo& info, const Ref<Bitmap>& loadedAtlas /*= nullptr*/)
    {
        mInfo = info;
        mReady = false;
        if (!info.ok)
        {
            mHintLabel->text = "Preview failed: " + info.error;
            mHintLabel->enabled = true;
            return;
        }

        auto atlas = loadedAtlas;
        if (!atlas)
        {
            String bytes = PipelineUtils::ReadFileBytes(info.atlasPath);
            atlas = mmake<Bitmap>();
            if (bytes.IsEmpty() || !atlas->LoadFromMemory((const UInt8*)bytes.Data(), (UInt)bytes.Length(), Bitmap::ImageType::Png))
                atlas = nullptr;
        }
        if (!atlas)
        {
            mHintLabel->text = "Preview atlas is unreadable";
            mHintLabel->enabled = true;
            return;
        }

        mAtlas = TextureRef(*atlas);
        mFrame->SetTexture(mAtlas);
        mFrame->SetTextureSrcRect(PipelineVideo::FrameRect(info, 0));
        mFrame->mode = SpriteMode::Default;

        if (info.hasAudio)
        {
            String wav = PipelineUtils::ReadFileBytes(info.audioPath);
            if (!wav.IsEmpty())
            {
                mSoundAsset = SharedSoundAsset(wav);

                mPlayer = mmake<SoundPlayer>();
                mPlayer->SetSound(AssetRef<SoundAsset>(mSoundAsset));
                mPlayer->SetVolume(mControls->GetVolume());
            }
        }

        mTime = 0.0f;
        mPlaying = false;
        mReady = true;
        mHintLabel->enabled = false;
    }

    void PipelineVideoView::ClearPreview()
    {
        if (mPlayer)
            mPlayer->Stop();

        mPlayer = nullptr;
        mSoundAsset = nullptr;
        mAtlas = TextureRef();
        mJob = nullptr;
        mInfo = PipelineVideo::PreviewInfo();
        mReady = false;
        mPlaying = false;
        mHasVideo = false;
        mTime = 0.0f;
        mHintLabel->text = mHint;
        mHintLabel->enabled = true;
    }

    void PipelineVideoView::SyncAudio(bool seek)
    {
        if (!mPlayer)
            return;

        float audioDuration = Math::Max(0.0f, mPlayer->GetDuration());
        if (seek)
            mPlayer->SetTime(Math::Clamp(mTime, 0.0f, audioDuration));

        if (mPlaying && mTime < audioDuration && !mPlayer->IsPlaying())
            mPlayer->Play();
        else if (!mPlaying && mPlayer->IsPlaying())
            mPlayer->Stop();
    }

    void PipelineVideoView::Play()
    {
        if (!mReady)
            return;

        if (mTime >= mInfo.duration - 0.01f)
            mTime = 0.0f;

        mPlaying = true;
        SyncAudio(true);
        RefreshControls();
    }

    void PipelineVideoView::Pause()
    {
        mPlaying = false;
        SyncAudio(false);
        RefreshControls();
    }

    void PipelineVideoView::TogglePlay()
    {
        if (mPlaying)
            Pause();
        else
            Play();
    }

    void PipelineVideoView::Seek(float time)
    {
        if (!mReady)
            return;

        mTime = Math::Clamp(time, 0.0f, mInfo.duration);
        mFrame->SetTextureSrcRect(PipelineVideo::FrameRect(mInfo, GetFrameIndex()));
        SyncAudio(true);
        RefreshControls();
    }

    int PipelineVideoView::GetFrameIndex() const
    {
        if (!mReady || mInfo.frameCount <= 0)
            return 0;

        return Math::Clamp((int)(mTime * mInfo.fps), 0, mInfo.frameCount - 1);
    }

    RectF PipelineVideoView::GetFrameArea() const
    {
        RectF area = GetCardArea();
        area.bottom += 1.0f;
        area.left += 1.0f;
        area.right -= 1.0f;
        area.top -= 1.0f;
        return area;
    }

    RectF PipelineVideoView::GetCardArea() const
    {
        RectF area = layout->GetWorldRect();
        area.bottom += PipelineMediaControls::height + 7.0f;
        return area;
    }

    void PipelineAudioView::Draw()
    {
        if (!PipelineControls::IsFarView())
        {
            Widget::Draw();
            return;
        }

        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        DrawLayers();
        OnDrawn();
        mTitleLabel->Draw();
        mInfoLabel->Draw();
        DrawTopLayers();
    }

    void PipelineVideoView::Draw()
    {
        if (!PipelineControls::IsFarView())
        {
            Widget::Draw();
            return;
        }

        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        DrawLayers();
        OnDrawn();
        DrawTopLayers();
    }

    void PipelineVideoView::DrawFrame()
    {
        if (!mResEnabledInHierarchy)
            return;

        // .result-card plate: shadow, ck1 fill, a line border
        auto& theme = PipelineTheme::Get();
        RectF card = GetCardArea();
        PipelineDraw::Corners corners(PipelineControls::fieldRadius);
        if (!PipelineControls::IsFarView())
            PipelineDraw::Shadow(RectF(card.left, card.top - theme.pictureShadowOffset, card.right, card.bottom - theme.pictureShadowOffset), corners,
                                 theme.pictureShadow, theme.pictureShadowBlur);
        PipelineDraw::Fill(card, corners, theme.checkerA);
        PipelineDraw::Stroke(card, corners, 0.0f, 1.0f, PipelineDraw::Gradient(theme.dark ? Color4(255, 255, 255, 20) : theme.line));

        if (!mReady || mInfo.frameWidth <= 0 || mInfo.frameHeight <= 0)
            return;

        RectF area = GetFrameArea();
        if (area.Width() <= 0.0f || area.Height() <= 0.0f)
            return;

        float k = Math::Min(area.Width() / mInfo.frameWidth, area.Height() / mInfo.frameHeight);
        Vec2F size(mInfo.frameWidth * k, mInfo.frameHeight * k);
        Vec2F center = area.Center();
        mFrame->rect = RectF(center - size * 0.5f, center + size * 0.5f);
        mFrame->Draw();
    }

    void PipelineVideoView::RefreshControls()
    {
        mControls->SetActive(mReady);
        mControls->SetPlaying(mPlaying);
        mControls->SetTime(mTime, mInfo.duration);
    }

    void PipelineVideoView::Update(float dt)
    {
        Widget::Update(dt);

        if (mJob && mJob->done)
        {
            auto job = mJob;
            mJob = nullptr;
            if (job->dir == mCacheDir)
                ApplyPreview(job->result, job->atlas);
            job->atlas = nullptr;
        }

        if (mReady && mPlaying)
        {
            mTime += dt;
            if (mTime >= mInfo.duration)
            {
                if (mControls->IsLoop())
                {
                    mTime = Math::Mod(mTime, Math::Max(mInfo.duration, 0.001f));
                    SyncAudio(true);
                }
                else
                {
                    mTime = mInfo.duration;
                    Pause();
                }
            }

            mFrame->SetTextureSrcRect(PipelineVideo::FrameRect(mInfo, GetFrameIndex()));
        }

        if (mPlayer)
            mPlayer->Update(dt);

        RefreshControls();
    }
}
// --- META ---

ENUM_META(Editor::PipelineMediaControls::Mode, Editor__PipelineMediaControls__Mode)
{
    ENUM_ENTRY(Audio);
    ENUM_ENTRY(Transport);
}
END_ENUM_META;

DECLARE_CLASS(Editor::PipelineMediaControls, Editor__PipelineMediaControls);

DECLARE_CLASS(Editor::PipelineAudioView, Editor__PipelineAudioView);

DECLARE_CLASS(Editor::PipelineVideoView, Editor__PipelineVideoView);
// --- END META ---
