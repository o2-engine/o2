#pragma once

#ifdef PLATFORM_ANDROID

#include <jni.h>
#include <android/asset_manager.h>

#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    // -------------------------------------------------------------------
    // AndroidPlatform — глобальные платформенные параметры, приходящие из
    // Java-слоя (JNI bridge). Заполняются из JavaBridge.cpp ДО вызова
    // Application::Initialize(). Сам Application::Initialize() не
    // принимает аргументов — он читает эти значения внутри
    // InitializePlatform() (по образцу Windows/Mac/Web).
    // -------------------------------------------------------------------
    namespace AndroidPlatform
    {
        void SetJVM(JavaVM* jvm);
        void SetActivity(jobject activity);
        void SetAssetManager(AAssetManager* assetManager);
        void SetDataPath(const String& dataPath);
        void SetResolution(const Vec2I& resolution);

        JavaVM*        GetJVM();
        jobject        GetActivity();
        AAssetManager* GetAssetManager();
        String         GetDataPath();
        Vec2I          GetResolution();
    }
}

#endif // PLATFORM_ANDROID
