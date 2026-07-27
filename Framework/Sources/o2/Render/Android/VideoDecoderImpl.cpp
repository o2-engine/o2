#include "o2/stdafx.h"

#ifdef PLATFORM_ANDROID

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <dlfcn.h>
#include <android/api-level.h>
#include <android/asset_manager.h>
#include <android/hardware_buffer.h>
#include <media/NdkImage.h>
#include <media/NdkImageReader.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaExtractor.h>
#include <media/NdkMediaFormat.h>
#include <unistd.h>

#include <cstring>

#include "o2/Application/Android/AndroidPlatform.h"
#include "o2/Render/Android/OpenGL.h"
#include "o2/Render/Texture.h"
#include "o2/Render/TextureRef.h"
#include "o2/Render/VideoDecoder.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Debug/Debug.h"

namespace o2
{
    namespace
    {
        // ---------------------------------------------------------------------------------------
        // Draws a decoded frame from an EGLImage (an external OES texture) into a plain RGBA
        // texture of the engine. That keeps the surface decoding path contained in this file: the
        // rest of the render keeps seeing an ordinary GL_TEXTURE_2D. Built lazily on the GL thread
        // ---------------------------------------------------------------------------------------
        class ExternalFrameBlitter
        {
        public:
            ~ExternalFrameBlitter()
            {
                if (mProgram) glDeleteProgram(mProgram);
                if (mVertexBuffer) glDeleteBuffers(1, &mVertexBuffer);
                if (mFrameBuffer) glDeleteFramebuffers(1, &mFrameBuffer);
                if (mExternalTexture) glDeleteTextures(1, &mExternalTexture);
            }

            // Returns false when the device has no EGL image entry points
            bool Initialize()
            {
                if (mInitialized)
                    return mAvailable;

                mInitialized = true;

                mGetNativeClientBuffer = (PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC)
                    eglGetProcAddress("eglGetNativeClientBufferANDROID");
                mCreateImage = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
                mDestroyImage = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
                mImageTargetTexture = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)
                    eglGetProcAddress("glEGLImageTargetTexture2DOES");

                if (!mGetNativeClientBuffer || !mCreateImage || !mDestroyImage || !mImageTargetTexture)
                    return false;

                if (!BuildProgram())
                    return false;

                glGenTextures(1, &mExternalTexture);
                glBindTexture(GL_TEXTURE_EXTERNAL_OES, mExternalTexture);
                glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

                glGenFramebuffers(1, &mFrameBuffer);

                mAvailable = true;
                return true;
            }

            bool Blit(AHardwareBuffer* buffer, GLuint targetTexture, const Vec2I& size)
            {
                if (!Initialize() || !buffer || targetTexture == 0)
                    return false;

                EGLClientBuffer clientBuffer = mGetNativeClientBuffer(buffer);
                if (!clientBuffer)
                    return false;

                const EGLint attribs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE };
                EGLImageKHR image = mCreateImage(eglGetCurrentDisplay(), EGL_NO_CONTEXT,
                                                 EGL_NATIVE_BUFFER_ANDROID, clientBuffer, attribs);
                if (image == EGL_NO_IMAGE_KHR)
                    return false;

                GLint prevFrameBuffer = 0, prevProgram = 0, prevTexture = 0, prevArrayBuffer = 0;
                GLint prevViewport[4] = { 0, 0, 0, 0 };
                glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFrameBuffer);
                glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
                glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTexture);
                glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevArrayBuffer);
                glGetIntegerv(GL_VIEWPORT, prevViewport);
                GLboolean prevBlend = glIsEnabled(GL_BLEND);
                GLboolean prevDepth = glIsEnabled(GL_DEPTH_TEST);
                GLboolean prevScissor = glIsEnabled(GL_SCISSOR_TEST);

                glBindTexture(GL_TEXTURE_EXTERNAL_OES, mExternalTexture);
                mImageTargetTexture(GL_TEXTURE_EXTERNAL_OES, (GLeglImageOES)image);

                glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, targetTexture, 0);

                bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
                if (complete)
                {
                    glViewport(0, 0, size.x, size.y);
                    glDisable(GL_BLEND);
                    glDisable(GL_DEPTH_TEST);
                    glDisable(GL_SCISSOR_TEST);

                    glUseProgram(mProgram);
                    glUniform1i(mTextureUniform, 0);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_EXTERNAL_OES, mExternalTexture);

                    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
                    glEnableVertexAttribArray((GLuint)mPositionAttribute);
                    glVertexAttribPointer((GLuint)mPositionAttribute, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                    glDisableVertexAttribArray((GLuint)mPositionAttribute);
                }

                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
                glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)prevFrameBuffer);
                glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
                glBindTexture(GL_TEXTURE_2D, (GLuint)prevTexture);
                glBindBuffer(GL_ARRAY_BUFFER, (GLuint)prevArrayBuffer);
                glUseProgram((GLuint)prevProgram);
                glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
                if (prevBlend) glEnable(GL_BLEND);
                if (prevDepth) glEnable(GL_DEPTH_TEST);
                if (prevScissor) glEnable(GL_SCISSOR_TEST);

                mDestroyImage(eglGetCurrentDisplay(), image);

                return complete;
            }

        private:
            bool BuildProgram()
            {
                // o2 textures are bottom-up, the decoded frame comes top-down: the quad flips v
                static const char* vertexSource =
                    "attribute vec2 a_position;\n"
                    "varying vec2 v_uv;\n"
                    "void main() {\n"
                    "    v_uv = vec2(a_position.x*0.5 + 0.5, 0.5 - a_position.y*0.5);\n"
                    "    gl_Position = vec4(a_position, 0.0, 1.0);\n"
                    "}\n";

                static const char* fragmentSource =
                    "#extension GL_OES_EGL_image_external : require\n"
                    "precision mediump float;\n"
                    "uniform samplerExternalOES u_texture;\n"
                    "varying vec2 v_uv;\n"
                    "void main() { gl_FragColor = texture2D(u_texture, v_uv); }\n";

                GLuint vertex = Compile(GL_VERTEX_SHADER, vertexSource);
                GLuint fragment = Compile(GL_FRAGMENT_SHADER, fragmentSource);
                if (!vertex || !fragment)
                    return false;

                mProgram = glCreateProgram();
                glAttachShader(mProgram, vertex);
                glAttachShader(mProgram, fragment);
                glLinkProgram(mProgram);
                glDeleteShader(vertex);
                glDeleteShader(fragment);

                GLint linked = GL_FALSE;
                glGetProgramiv(mProgram, GL_LINK_STATUS, &linked);
                if (!linked)
                {
                    char log[512] = {};
                    glGetProgramInfoLog(mProgram, sizeof(log) - 1, nullptr, log);
                    o2Debug.LogError((String)"Video blit program link failed: " + log);
                    glDeleteProgram(mProgram);
                    mProgram = 0;
                    return false;
                }

                mPositionAttribute = glGetAttribLocation(mProgram, "a_position");
                mTextureUniform = glGetUniformLocation(mProgram, "u_texture");

                static const float quad[] = { -1, -1, 1, -1, -1, 1, 1, 1 };
                glGenBuffers(1, &mVertexBuffer);
                glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
                glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                return mPositionAttribute >= 0;
            }

            static GLuint Compile(GLenum type, const char* source)
            {
                GLuint shader = glCreateShader(type);
                glShaderSource(shader, 1, &source, nullptr);
                glCompileShader(shader);

                GLint compiled = GL_FALSE;
                glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
                if (!compiled)
                {
                    char log[512] = {};
                    glGetShaderInfoLog(shader, sizeof(log) - 1, nullptr, log);
                    o2Debug.LogError((String)"Video blit shader compile failed: " + log);
                    glDeleteShader(shader);
                    return 0;
                }

                return shader;
            }

        private:
            bool mInitialized = false;
            bool mAvailable = false;

            GLuint mProgram = 0;
            GLuint mVertexBuffer = 0;
            GLuint mFrameBuffer = 0;
            GLuint mExternalTexture = 0;

            GLint mPositionAttribute = -1;
            GLint mTextureUniform = -1;

            PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC mGetNativeClientBuffer = nullptr;
            PFNEGLCREATEIMAGEKHRPROC               mCreateImage = nullptr;
            PFNEGLDESTROYIMAGEKHRPROC              mDestroyImage = nullptr;
            PFNGLEGLIMAGETARGETTEXTURE2DOESPROC    mImageTargetTexture = nullptr;
        };
    }

    // -----------------------------------------------------------------------------
    // Hardware video decoder over AMediaExtractor + AMediaCodec (NDK). Decodes into
    // an AImageReader surface when the device supports it, so the frame goes from the
    // hardware decoder straight into a GPU texture; falls back to the YUV byte-buffer
    // output with a CPU conversion to RGBA otherwise
    // -----------------------------------------------------------------------------
    class AndroidVideoDecoder: public VideoDecoder
    {
    public:
        ~AndroidVideoDecoder();

        bool Open(const AssetRef<VideoAsset>& asset, bool streaming) override;

        Vec2I GetSize() const override;
        float GetFrameRate() const override;
        float GetDuration() const override;

        bool DecodeNextFrame(float& outTime) override;
        bool SeekFrame(float time, float& outTime) override;
        bool ReadLastFrame(Bitmap& into) override;
        bool UploadLastFrame(const TextureRef& texture) override;

        // Opens the decoder over a file path; used by Open and the format info parse
        bool OpenFile(const String& path);

    private:
        // Pushes input and takes one decoded frame out; timeoutUs 0 makes both codec calls return
        // immediately, so a frame that is not ready yet costs nothing
        bool DecodeFrame(float& outTime, int64_t timeoutUs, int maxAttempts);

    private:
        AMediaExtractor* mExtractor = nullptr;
        AMediaCodec*     mCodec = nullptr;

        // Surface output: the codec renders into the reader, frames reach the GPU without the CPU
        AImageReader*  mImageReader = nullptr;
        ANativeWindow* mReaderWindow = nullptr;
        AImage*        mCurrentImage = nullptr;

        ExternalFrameBlitter mBlitter;

        // Creates the image reader for the surface output path; false keeps the byte-buffer path
        bool CreateImageReader();

        // Takes the freshest frame out of the reader, dropping the previous one
        void AcquireLatestImage();

        // Frees the acquired frame
        void ReleaseCurrentImage();

        Vec2I mSize;
        float mFrameRate = 0.0f;
        float mDuration = 0.0f;

        int mColorFormat = 0; // 19 - planar I420, 21 - semi-planar NV12
        int mStride = 0;      // Y plane row stride in bytes
        int mSliceHeight = 0; // Y plane rows before the chroma planes

        ssize_t               mLastOutputIndex = -1;
        AMediaCodecBufferInfo mLastOutputInfo;

        bool mInputEnded = false;

        void ReleaseLastOutput();
        void ReadOutputFormat();
    };

    AndroidVideoDecoder::~AndroidVideoDecoder()
    {
        ReleaseLastOutput();
        ReleaseCurrentImage();

        if (mCodec)
        {
            AMediaCodec_stop(mCodec);
            AMediaCodec_delete(mCodec);
        }

        if (mExtractor)
            AMediaExtractor_delete(mExtractor);

        if (mImageReader)
            AImageReader_delete(mImageReader); // owns and closes the window

        mReaderWindow = nullptr;
    }

    bool AndroidVideoDecoder::Open(const AssetRef<VideoAsset>& asset, bool streaming)
    {
        return OpenFile(asset->GetBuiltFullPath());
    }

    bool AndroidVideoDecoder::OpenFile(const String& path)
    {
        if (path.IsEmpty())
            return false;

        mExtractor = AMediaExtractor_new();

        // APK assets live inside the zip: open through the asset manager and hand the
        // extractor a file descriptor with the asset's offset/length. Media extensions
        // are stored uncompressed by the APK packer, so the descriptor is seekable
        media_status_t status = AMEDIA_ERROR_BASE;
        if (AAssetManager* assetManager = AndroidPlatform::GetAssetManager())
        {
            if (AAsset* asset = AAssetManager_open(assetManager, path.Data(), AASSET_MODE_RANDOM))
            {
                off_t offset = 0, length = 0;
                int fd = AAsset_openFileDescriptor(asset, &offset, &length);
                if (fd >= 0)
                {
                    status = AMediaExtractor_setDataSourceFd(mExtractor, fd, offset, length);
                    close(fd); // the extractor dups the descriptor
                }

                AAsset_close(asset);
            }
        }

        if (status != AMEDIA_OK)
            status = AMediaExtractor_setDataSource(mExtractor, path.Data()); // plain file fallback

        if (status != AMEDIA_OK)
            return false;

        size_t trackCount = AMediaExtractor_getTrackCount(mExtractor);
        for (size_t i = 0; i < trackCount; i++)
        {
            AMediaFormat* format = AMediaExtractor_getTrackFormat(mExtractor, i);

            const char* mime = nullptr;
            if (AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mime) && mime &&
                strncmp(mime, "video/", 6) == 0)
            {
                int32_t width = 0, height = 0, frameRate = 0;
                int64_t durationUs = 0;
                AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_WIDTH, &width);
                AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_HEIGHT, &height);
                AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_FRAME_RATE, &frameRate);
                AMediaFormat_getInt64(format, AMEDIAFORMAT_KEY_DURATION, &durationUs);

                mSize = Vec2I(width, height);
                mFrameRate = (float)frameRate;
                mDuration = (float)((double)durationUs/1000000.0);
                mStride = width;
                mSliceHeight = height;

                AMediaExtractor_selectTrack(mExtractor, i);

                mCodec = AMediaCodec_createDecoderByType(mime);

                // With a surface the decoded frame stays in graphic memory; without it the
                // framework copies every frame into a linear YUV buffer for the CPU conversion
                CreateImageReader();

                if (!mCodec ||
                    AMediaCodec_configure(mCodec, format, mReaderWindow, nullptr, 0) != AMEDIA_OK ||
                    AMediaCodec_start(mCodec) != AMEDIA_OK)
                {
                    AMediaFormat_delete(format);
                    return false;
                }

                AMediaFormat_delete(format);
                return mSize.x > 0 && mSize.y > 0;
            }

            AMediaFormat_delete(format);
        }

        return false;
    }

    Vec2I AndroidVideoDecoder::GetSize() const
    {
        return mSize;
    }

    float AndroidVideoDecoder::GetFrameRate() const
    {
        return mFrameRate;
    }

    float AndroidVideoDecoder::GetDuration() const
    {
        return mDuration;
    }

    void AndroidVideoDecoder::ReleaseLastOutput()
    {
        if (mLastOutputIndex >= 0)
        {
            AMediaCodec_releaseOutputBuffer(mCodec, (size_t)mLastOutputIndex, false);
            mLastOutputIndex = -1;
        }
    }

    void AndroidVideoDecoder::ReadOutputFormat()
    {
        AMediaFormat* format = AMediaCodec_getOutputFormat(mCodec);
        if (!format)
            return;

        int32_t value = 0;
        if (AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT, &value))
            mColorFormat = value;
        if (AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_STRIDE, &value))
            mStride = value;
        if (AMediaFormat_getInt32(format, "slice-height", &value))
            mSliceHeight = value;

        AMediaFormat_delete(format);
    }

    bool AndroidVideoDecoder::DecodeNextFrame(float& outTime)
    {
        // Playback runs ahead of the video: when the codec has nothing ready the caller simply keeps
        // showing the current frame, so waiting on it would only stall the frame for nothing
        return DecodeFrame(outTime, 0, 2);
    }

    bool AndroidVideoDecoder::DecodeFrame(float& outTime, int64_t timeoutUs, int maxAttempts)
    {
        if (!mCodec)
            return false;

        ReleaseLastOutput();

        int guard = 0;
        while (guard++ < maxAttempts)
        {
            // Queue a few packets before asking for output: the codec starts producing only after
            // several of them, which is what makes the first frame after a flush expensive. Dequeuing
            // input never waits, and the batch is bounded so feeding can't run away with the frame
            for (int fed = 0; fed < 2 && !mInputEnded; fed++)
            {
                ssize_t inputIndex = AMediaCodec_dequeueInputBuffer(mCodec, 0);
                if (inputIndex < 0)
                    break;

                size_t capacity = 0;
                uint8_t* inputBuffer = AMediaCodec_getInputBuffer(mCodec, (size_t)inputIndex, &capacity);
                ssize_t sampleSize = AMediaExtractor_readSampleData(mExtractor, inputBuffer, capacity);

                if (sampleSize < 0)
                {
                    AMediaCodec_queueInputBuffer(mCodec, (size_t)inputIndex, 0, 0, 0,
                                                 AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
                    mInputEnded = true;
                }
                else
                {
                    AMediaCodec_queueInputBuffer(mCodec, (size_t)inputIndex, 0, (size_t)sampleSize,
                                                 AMediaExtractor_getSampleTime(mExtractor), 0);
                    AMediaExtractor_advance(mExtractor);
                }
            }

            AMediaCodecBufferInfo info;
            ssize_t outputIndex = AMediaCodec_dequeueOutputBuffer(mCodec, &info, timeoutUs);

            if (outputIndex >= 0)
            {
                if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM)
                {
                    AMediaCodec_releaseOutputBuffer(mCodec, (size_t)outputIndex, false);
                    return false;
                }

                outTime = (float)((double)info.presentationTimeUs/1000000.0);

                if (mImageReader)
                {
                    // hands the frame to the reader surface, nothing is copied to the cpu
                    AMediaCodec_releaseOutputBuffer(mCodec, (size_t)outputIndex, true);
                    AcquireLatestImage(); // keeps the reader queue drained while seeking
                    return true;
                }

                mLastOutputIndex = outputIndex;
                mLastOutputInfo = info;
                return true;
            }

            if (outputIndex == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED)
                ReadOutputFormat();
        }

        return false;
    }

    bool AndroidVideoDecoder::SeekFrame(float time, float& outTime)
    {
        if (!mCodec || !mExtractor)
            return false;

        ReleaseLastOutput();
        ReleaseCurrentImage();

        AMediaExtractor_seekTo(mExtractor, (int64_t)((double)Math::Max(time, 0.0f)*1000000.0),
                               AMEDIAEXTRACTOR_SEEK_PREVIOUS_SYNC);
        AMediaCodec_flush(mCodec);
        mInputEnded = false;

        // Decode from the key frame up to the requested time
        float halfFrame = mFrameRate > 0.0f ? 0.5f/mFrameRate : 0.0f;
        int guard = 0;
        while (guard++ < 1024)
        {
            // A flushed codec needs a good twenty milliseconds to produce its first frame. Blocking
            // the game for that is not worth it: report the requested position and let the following
            // updates pick the frames up, the picture lags by a couple of frames at most
            if (!DecodeFrame(outTime, 2000, 2))
            {
                outTime = time;
                return true;
            }

            if (outTime + halfFrame >= time)
                return true;
        }

        return false;
    }

    bool AndroidVideoDecoder::ReadLastFrame(Bitmap& into)
    {
        if (mLastOutputIndex < 0)
            return false;

        size_t bufferSize = 0;
        uint8_t* buffer = AMediaCodec_getOutputBuffer(mCodec, (size_t)mLastOutputIndex, &bufferSize);
        if (!buffer)
            return false;

        // Planar I420 (19) and semi-planar NV12 (21) layouts; anything else needs the
        // Surface/ImageReader path and isn't supported by the byte-buffer decoder
        if (mColorFormat != 19 && mColorFormat != 21 && mColorFormat != 0)
        {
            o2Debug.LogError("Unsupported video decoder color format: " + (String)mColorFormat);
            return false;
        }

        int w = mSize.x, h = mSize.y;
        int stride = mStride > 0 ? mStride : w;
        int sliceHeight = mSliceHeight > 0 ? mSliceHeight : h;

        const uint8_t* yPlane = buffer + mLastOutputInfo.offset;
        const uint8_t* uPlane;
        const uint8_t* vPlane;
        int chromaStride, chromaStep;

        if (mColorFormat == 19) // planar I420
        {
            uPlane = yPlane + (size_t)stride*sliceHeight;
            vPlane = uPlane + (size_t)(stride/2)*(sliceHeight/2);
            chromaStride = stride/2;
            chromaStep = 1;
        }
        else // semi-planar NV12 (default assumption)
        {
            uPlane = yPlane + (size_t)stride*sliceHeight;
            vPlane = uPlane + 1;
            chromaStride = stride;
            chromaStep = 2;
        }

        UInt8* dstData = into.GetData();

        // BT.601 integer conversion; o2 bitmaps are bottom-up
        for (int y = 0; y < h; y++)
        {
            const uint8_t* yRow = yPlane + (size_t)y*stride;
            const uint8_t* uRow = uPlane + (size_t)(y/2)*chromaStride;
            const uint8_t* vRow = vPlane + (size_t)(y/2)*chromaStride;
            UInt8* dstRow = dstData + (size_t)(h - 1 - y)*w*4;

            for (int x = 0; x < w; x++)
            {
                int yv = ((int)yRow[x] - 16)*76309;
                int cb = (int)uRow[(x/2)*chromaStep] - 128;
                int cr = (int)vRow[(x/2)*chromaStep] - 128;

                int r = (yv + cr*104597) >> 16;
                int g = (yv - cb*25674 - cr*53278) >> 16;
                int b = (yv + cb*132201) >> 16;

                dstRow[x*4 + 0] = (UInt8)Math::Clamp(r, 0, 255);
                dstRow[x*4 + 1] = (UInt8)Math::Clamp(g, 0, 255);
                dstRow[x*4 + 2] = (UInt8)Math::Clamp(b, 0, 255);
                dstRow[x*4 + 3] = 255;
            }
        }

        return true;
    }

    // The two entry points of the surface path landed in api 26 while the project still builds
    // against 24, where the ndk headers mark them unavailable: take them by name instead
    namespace
    {
        using ImageReaderNewWithUsageFn = media_status_t (*)(int32_t, int32_t, int32_t, uint64_t,
                                                             int32_t, AImageReader**);
        using ImageGetHardwareBufferFn = media_status_t (*)(const AImage*, AHardwareBuffer**);

        ImageReaderNewWithUsageFn GetImageReaderNewWithUsage()
        {
            static auto fn = (ImageReaderNewWithUsageFn)dlsym(RTLD_DEFAULT, "AImageReader_newWithUsage");
            return fn;
        }

        ImageGetHardwareBufferFn GetImageGetHardwareBuffer()
        {
            static auto fn = (ImageGetHardwareBufferFn)dlsym(RTLD_DEFAULT, "AImage_getHardwareBuffer");
            return fn;
        }
    }

    bool AndroidVideoDecoder::CreateImageReader()
    {
        if (mSize.x <= 0 || mSize.y <= 0)
            return false;

        auto newWithUsage = GetImageReaderNewWithUsage();
        if (!newWithUsage || !GetImageGetHardwareBuffer())
            return false;

        // three images let the codec keep decoding while one frame is being sampled
        if (newWithUsage(mSize.x, mSize.y, AIMAGE_FORMAT_PRIVATE,
                         AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE, 3, &mImageReader) != AMEDIA_OK)
        {
            mImageReader = nullptr;
        }

        if (!mImageReader)
            return false;

        if (AImageReader_getWindow(mImageReader, &mReaderWindow) != AMEDIA_OK || !mReaderWindow)
        {
            AImageReader_delete(mImageReader);
            mImageReader = nullptr;
            mReaderWindow = nullptr;
            return false;
        }

        return true;
    }

    void AndroidVideoDecoder::ReleaseCurrentImage()
    {
        if (mCurrentImage)
        {
            AImage_delete(mCurrentImage);
            mCurrentImage = nullptr;
        }
    }

    void AndroidVideoDecoder::AcquireLatestImage()
    {
        if (!mImageReader)
            return;

        AImage* image = nullptr;
        if (AImageReader_acquireLatestImage(mImageReader, &image) == AMEDIA_OK && image)
        {
            ReleaseCurrentImage();
            mCurrentImage = image;
        }
    }

    bool AndroidVideoDecoder::UploadLastFrame(const TextureRef& texture)
    {
        if (!mImageReader || !texture.IsValid())
            return false;

        // the frame is queued asynchronously; when it has not landed yet the caller keeps the
        // current one rather than the frame waiting for it
        if (!mCurrentImage)
            AcquireLatestImage();

        if (!mCurrentImage)
            return false;

        AHardwareBuffer* buffer = nullptr;
        if (auto getHardwareBuffer = GetImageGetHardwareBuffer())
            getHardwareBuffer(mCurrentImage, &buffer);

        if (!buffer)
            return false;

        return mBlitter.Blit(buffer, texture.Get()->mHandle, mSize);
    }

    Ref<VideoDecoder> CreatePlatformVideoDecoder()
    {
        return mmake<AndroidVideoDecoder>();
    }

    bool PlatformParseVideoFormatInfo(const String& path, Vec2I& size, float& frameRate, float& duration)
    {
        AndroidVideoDecoder decoder;
        if (!decoder.OpenFile(path))
            return false;

        size = decoder.GetSize();
        frameRate = decoder.GetFrameRate();
        duration = decoder.GetDuration();
        return true;
    }
}

#endif // PLATFORM_ANDROID
