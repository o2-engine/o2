#include "o2/stdafx.h"

#ifdef PLATFORM_WASM

#include <emscripten.h>

#include <cstdio>
#include <vector>

#include "o2/Render/TextureRef.h"
#include "o2/Render/VideoDecoder.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Debug/Debug.h"

// Browser video element registry: the browser decodes hardware-accelerated on its own
// clock; frames are uploaded straight into the WebGL texture via texImage2D

EM_JS(int, o2video_create, (const char* data, int size), {
    if (!Module.o2Videos)
    {
        Module.o2Videos = {};
        Module.o2VideoNextId = 1;
    }

    var bytes = HEAPU8.slice(data, data + size);
    var blob = new Blob([bytes], { type: 'video/mp4' });

    var el = document.createElement('video');
    el.src = URL.createObjectURL(blob);
    el.muted = true;
    el.playsInline = true;
    el.preload = 'auto';
    el.load();

    var id = Module.o2VideoNextId++;
    Module.o2Videos[id] = el;
    return id;
});

EM_JS(void, o2video_destroy, (int id), {
    var el = Module.o2Videos && Module.o2Videos[id];
    if (!el)
        return;

    el.pause();
    URL.revokeObjectURL(el.src);
    delete Module.o2Videos[id];
});

EM_JS(int, o2video_ready_state, (int id), {
    var el = Module.o2Videos && Module.o2Videos[id];
    return el ? el.readyState : 0;
});

EM_JS(int, o2video_width, (int id), {
    var el = Module.o2Videos && Module.o2Videos[id];
    return el ? el.videoWidth : 0;
});

EM_JS(int, o2video_height, (int id), {
    var el = Module.o2Videos && Module.o2Videos[id];
    return el ? el.videoHeight : 0;
});

EM_JS(double, o2video_duration, (int id), {
    var el = Module.o2Videos && Module.o2Videos[id];
    var d = el ? el.duration : 0;
    return isFinite(d) ? d : 0;
});

EM_JS(double, o2video_time, (int id), {
    var el = Module.o2Videos && Module.o2Videos[id];
    return el ? el.currentTime : 0;
});

EM_JS(void, o2video_seek, (int id, double time), {
    var el = Module.o2Videos && Module.o2Videos[id];
    if (el)
        el.currentTime = time;
});

EM_JS(void, o2video_play, (int id), {
    var el = Module.o2Videos && Module.o2Videos[id];
    if (el && el.paused)
    {
        var p = el.play();
        if (p && p.catch)
            p.catch(function() {});
    }
});

EM_JS(int, o2video_upload, (int id, int glHandle), {
    var el = Module.o2Videos && Module.o2Videos[id];
    if (!el || el.readyState < 2)
        return 0;

    var tex = GL.textures[glHandle];
    if (!tex)
        return 0;

    var prev = GLctx.getParameter(0x8069 /*TEXTURE_BINDING_2D*/);
    GLctx.bindTexture(0x0DE1 /*TEXTURE_2D*/, tex);
    GLctx.pixelStorei(0x9240 /*UNPACK_FLIP_Y_WEBGL*/, true);
    GLctx.texImage2D(0x0DE1, 0, 0x1908 /*RGBA*/, 0x1908, 0x1401 /*UNSIGNED_BYTE*/, el);
    GLctx.pixelStorei(0x9240, false);
    GLctx.bindTexture(0x0DE1, prev);
    return 1;
});

namespace o2
{
    // -----------------------------------------------------------------------------
    // Browser video decoder over an HTMLVideoElement. The browser plays and decodes
    // the clip on its own clock; DecodeNextFrame reports when the element shows a
    // new frame, and UploadLastFrame copies it into the WebGL texture on the GPU.
    // Setup is asynchronous: the size becomes known once metadata is loaded.
    // -----------------------------------------------------------------------------
    class WasmVideoDecoder: public VideoDecoder
    {
    public:
        ~WasmVideoDecoder();

        bool Open(const AssetRef<VideoAsset>& asset, bool streaming) override;

        Vec2I GetSize() const override;
        float GetFrameRate() const override;
        float GetDuration() const override;

        bool DecodeNextFrame(float& outTime) override;
        bool SeekFrame(float time, float& outTime) override;
        bool ReadLastFrame(Bitmap& into) override;
        bool UploadLastFrame(const TextureRef& texture) override;

    private:
        int   mId = 0;
        float mLastTime = -1.0f;
    };

    WasmVideoDecoder::~WasmVideoDecoder()
    {
        if (mId != 0)
            o2video_destroy(mId);
    }

    bool WasmVideoDecoder::Open(const AssetRef<VideoAsset>& asset, bool streaming)
    {
        if (asset->GetData() && asset->GetDataSize() > 0)
        {
            mId = o2video_create(asset->GetData(), (int)asset->GetDataSize());
            return mId != 0;
        }

        // File-only asset: read the built file (MEMFS) and hand the bytes to the blob
        String path = asset->GetBuiltFullPath();
        FILE* file = fopen(path.Data(), "rb");
        if (!file)
            return false;

        fseek(file, 0, SEEK_END);
        long length = ftell(file);
        fseek(file, 0, SEEK_SET);

        std::vector<char> bytes((size_t)length);
        fread(bytes.data(), 1, (size_t)length, file);
        fclose(file);

        mId = o2video_create(bytes.data(), (int)bytes.size());
        return mId != 0;
    }

    Vec2I WasmVideoDecoder::GetSize() const
    {
        if (o2video_ready_state(mId) < 1)
            return Vec2I();

        return Vec2I(o2video_width(mId), o2video_height(mId));
    }

    float WasmVideoDecoder::GetFrameRate() const
    {
        return 0.0f; // Not exposed by the video element
    }

    float WasmVideoDecoder::GetDuration() const
    {
        return (float)o2video_duration(mId);
    }

    bool WasmVideoDecoder::DecodeNextFrame(float& outTime)
    {
        if (o2video_ready_state(mId) < 2)
            return false;

        o2video_play(mId);

        float time = (float)o2video_time(mId);
        if (time <= mLastTime + 0.001f)
            return false;

        mLastTime = time;
        outTime = time;
        return true;
    }

    bool WasmVideoDecoder::SeekFrame(float time, float& outTime)
    {
        if (!(time == time) || time < 0.0f) // HTMLMediaElement.currentTime throws on non-finite
            time = 0.0f;

        o2video_seek(mId, (double)time);
        mLastTime = time;
        outTime = time;
        return o2video_ready_state(mId) >= 1;
    }

    bool WasmVideoDecoder::ReadLastFrame(Bitmap& into)
    {
        return false; // No CPU readback path; frames go straight to the texture
    }

    bool WasmVideoDecoder::UploadLastFrame(const TextureRef& texture)
    {
        if (!texture.IsValid())
            return false;

        return o2video_upload(mId, (int)texture.Get()->mHandle) != 0;
    }

    Ref<VideoDecoder> CreatePlatformVideoDecoder()
    {
        return mmake<WasmVideoDecoder>();
    }

    bool PlatformParseVideoFormatInfo(const String& path, Vec2I& size, float& frameRate, float& duration)
    {
        // The video element parses asynchronously; container info isn't available synchronously
        return false;
    }
}

#endif // PLATFORM_WASM
