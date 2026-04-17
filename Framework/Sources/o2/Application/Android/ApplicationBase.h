#pragma once

#ifdef PLATFORM_ANDROID

#include <jni.h>
#include <android/asset_manager.h>

#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    // -------------------------------
    // Android application base fields
    // -------------------------------
    class ApplicationBase
    {
    protected:
        Vec2I mResolution;

        JavaVM*        mJVM = nullptr;
        jobject        mActivity = nullptr;
        AAssetManager* mAssetManager = nullptr;
        String         mDataPath;

    public:
        // Returns android Java virtual machine
        JavaVM* GetJVM() const;

        // Returns android activity
        jobject GetActivity() const;

        // Returns android asset manager
        AAssetManager* GetAssetManager() const;

        // Returns android data path
        String GetDataPath() const;
    };
}

#endif // PLATFORM_ANDROID
