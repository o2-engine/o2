#include "o2/stdafx.h"

#ifdef PLATFORM_ANDROID

#include "o2/Network/Http/HttpBackend.h"

#include "o2/Application/Android/AndroidPlatform.h"
#include "o2/Utils/Jobs/JobSystem.h"

#include <jni.h>

namespace o2
{
    // Returns true and clears the pending Java exception, mapping it to an HttpError
    static bool CheckJavaException(JNIEnv* env, HttpError& outError)
    {
        if (!env->ExceptionCheck())
            return false;

        jthrowable exception = env->ExceptionOccurred();
        env->ExceptionClear();

        outError = HttpError::ConnectionFailed;

        jclass timeoutClass = env->FindClass("java/net/SocketTimeoutException");
        if (timeoutClass && env->IsInstanceOf(exception, timeoutClass))
            outError = HttpError::Timeout;
        env->ExceptionClear();

        jclass unknownHostClass = env->FindClass("java/net/UnknownHostException");
        if (unknownHostClass && env->IsInstanceOf(exception, unknownHostClass))
            outError = HttpError::ResolveFailed;
        env->ExceptionClear();

        env->DeleteLocalRef(exception);
        if (timeoutClass)
            env->DeleteLocalRef(timeoutClass);
        if (unknownHostClass)
            env->DeleteLocalRef(unknownHostClass);

        return true;
    }

    // Performs one blocking HttpURLConnection exchange. Runs on a worker job thread
    static void PerformBlocking(const SharedRef<HttpTransfer>& transfer)
    {
        JavaVM* jvm = AndroidPlatform::GetJVM();
        if (!jvm)
        {
            transfer->error = HttpError::Internal;
            transfer->done.Store(1);
            return;
        }

        JNIEnv* env = nullptr;
        bool attached = false;
        if (jvm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK)
        {
            if (jvm->AttachCurrentThread(&env, nullptr) != JNI_OK)
            {
                transfer->error = HttpError::Internal;
                transfer->done.Store(1);
                return;
            }

            attached = true;
        }

        HttpError error = HttpError::None;
        jobject connection = nullptr;

        do
        {
            // URL url = new URL(urlString); HttpURLConnection connection = (HttpURLConnection)url.openConnection();
            jclass urlClass = env->FindClass("java/net/URL");
            jmethodID urlConstructor = env->GetMethodID(urlClass, "<init>", "(Ljava/lang/String;)V");
            jstring urlString = env->NewStringUTF(transfer->url.Data());
            jobject url = env->NewObject(urlClass, urlConstructor, urlString);
            if (CheckJavaException(env, error) || !url)
            {
                error = error == HttpError::None ? HttpError::InvalidUrl : error;
                break;
            }

            jmethodID openConnection = env->GetMethodID(urlClass, "openConnection", "()Ljava/net/URLConnection;");
            connection = env->CallObjectMethod(url, openConnection);
            if (CheckJavaException(env, error) || !connection)
                break;

            jclass connectionClass = env->FindClass("java/net/HttpURLConnection");

            int timeoutMs = (int)(transfer->timeout * 1000.0f);
            env->CallVoidMethod(connection, env->GetMethodID(connectionClass, "setConnectTimeout", "(I)V"), timeoutMs);
            env->CallVoidMethod(connection, env->GetMethodID(connectionClass, "setReadTimeout", "(I)V"), timeoutMs);
            env->CallVoidMethod(connection, env->GetMethodID(connectionClass, "setInstanceFollowRedirects", "(Z)V"),
                                JNI_FALSE);
            env->CallVoidMethod(connection, env->GetMethodID(connectionClass, "setUseCaches", "(Z)V"), JNI_FALSE);

            jstring methodString = env->NewStringUTF(transfer->method.Data());
            env->CallVoidMethod(connection, env->GetMethodID(connectionClass, "setRequestMethod",
                                                             "(Ljava/lang/String;)V"), methodString);
            if (CheckJavaException(env, error))
                break;

            jmethodID setRequestProperty = env->GetMethodID(connectionClass, "setRequestProperty",
                                                            "(Ljava/lang/String;Ljava/lang/String;)V");
            for (auto& line : transfer->headerLines)
            {
                size_t colon = line.find(':');
                if (colon == std::string::npos)
                    continue;

                String name = line.substr(0, colon);
                size_t valueBegin = line.find_first_not_of(" \t", colon + 1);
                String value = valueBegin == std::string::npos ? String() : (String)line.substr(valueBegin);

                jstring nameString = env->NewStringUTF(name.Data());
                jstring valueString = env->NewStringUTF(value.Data());
                env->CallVoidMethod(connection, setRequestProperty, nameString, valueString);
                env->DeleteLocalRef(nameString);
                env->DeleteLocalRef(valueString);
            }

            if (!transfer->body.IsEmpty())
            {
                env->CallVoidMethod(connection, env->GetMethodID(connectionClass, "setDoOutput", "(Z)V"), JNI_TRUE);

                jobject outputStream = env->CallObjectMethod(connection,
                    env->GetMethodID(connectionClass, "getOutputStream", "()Ljava/io/OutputStream;"));
                if (CheckJavaException(env, error) || !outputStream)
                    break;

                jbyteArray bodyArray = env->NewByteArray((jsize)transfer->body.size());
                env->SetByteArrayRegion(bodyArray, 0, (jsize)transfer->body.size(),
                                        (const jbyte*)transfer->body.data());

                jclass outputStreamClass = env->FindClass("java/io/OutputStream");
                env->CallVoidMethod(outputStream, env->GetMethodID(outputStreamClass, "write", "([B)V"), bodyArray);
                env->CallVoidMethod(outputStream, env->GetMethodID(outputStreamClass, "close", "()V"));
                if (CheckJavaException(env, error))
                    break;

                env->DeleteLocalRef(bodyArray);
                env->DeleteLocalRef(outputStream);
            }

            int status = env->CallIntMethod(connection, env->GetMethodID(connectionClass, "getResponseCode", "()I"));
            if (CheckJavaException(env, error))
                break;

            transfer->status = status;

            // Header lines: index 0 is the status line with a null key
            jmethodID getHeaderFieldKey = env->GetMethodID(connectionClass, "getHeaderFieldKey",
                                                           "(I)Ljava/lang/String;");
            jmethodID getHeaderField = env->GetMethodID(connectionClass, "getHeaderField", "(I)Ljava/lang/String;");

            for (int i = 1;; i++)
            {
                jstring key = (jstring)env->CallObjectMethod(connection, getHeaderFieldKey, i);
                jstring value = (jstring)env->CallObjectMethod(connection, getHeaderField, i);
                if (!value)
                    break;

                if (key)
                {
                    const char* keyChars = env->GetStringUTFChars(key, nullptr);
                    const char* valueChars = env->GetStringUTFChars(value, nullptr);
                    transfer->responseHeaderLines.Add(String(keyChars) + ": " + String(valueChars));
                    env->ReleaseStringUTFChars(key, keyChars);
                    env->ReleaseStringUTFChars(value, valueChars);
                    env->DeleteLocalRef(key);
                }

                env->DeleteLocalRef(value);
            }

            // Body: the error stream serves 4xx/5xx responses
            jobject inputStream = env->CallObjectMethod(connection,
                env->GetMethodID(connectionClass, "getInputStream", "()Ljava/io/InputStream;"));
            if (env->ExceptionCheck())
            {
                env->ExceptionClear();
                inputStream = env->CallObjectMethod(connection,
                    env->GetMethodID(connectionClass, "getErrorStream", "()Ljava/io/InputStream;"));
            }

            if (inputStream)
            {
                jclass inputStreamClass = env->FindClass("java/io/InputStream");
                jmethodID read = env->GetMethodID(inputStreamClass, "read", "([B)I");

                jbyteArray chunk = env->NewByteArray(65536);
                while (true)
                {
                    int readBytes = env->CallIntMethod(inputStream, read, chunk);
                    if (CheckJavaException(env, error) || readBytes <= 0)
                        break;

                    jbyte* chunkData = env->GetByteArrayElements(chunk, nullptr);
                    transfer->responseBody.append((const char*)chunkData, (size_t)readBytes);
                    env->ReleaseByteArrayElements(chunk, chunkData, JNI_ABORT);
                }

                env->DeleteLocalRef(chunk);
                env->CallVoidMethod(inputStream,
                                    env->GetMethodID(inputStreamClass, "close", "()V"));
                env->ExceptionClear();
                env->DeleteLocalRef(inputStream);
            }
        }
        while (false);

        if (connection)
        {
            jclass connectionClass = env->FindClass("java/net/HttpURLConnection");
            env->CallVoidMethod(connection, env->GetMethodID(connectionClass, "disconnect", "()V"));
            env->ExceptionClear();
        }

        if (attached)
            jvm->DetachCurrentThread();

        transfer->error = error;
        transfer->done.Store(1);
    }

    // HttpURLConnection-based backend: each transfer runs blocking on a worker job thread
    class AndroidHttpBackend: public IHttpBackend
    {
    public:
        void Perform(const SharedRef<HttpTransfer>& transfer) override
        {
            o2Jobs.Schedule([transfer] { PerformBlocking(transfer); }, JobPriority::Normal, JobThread::Any);
        }
    };

    IHttpBackend* CreateAndroidHttpBackend()
    {
        return mnew AndroidHttpBackend();
    }
}

#endif // PLATFORM_ANDROID
