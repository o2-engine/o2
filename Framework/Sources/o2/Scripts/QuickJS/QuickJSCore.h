#pragma once

#if defined(SCRIPTING_BACKEND_QUICKJS)

#include "quickjs.h"

namespace o2
{
    // Shared QuickJS engine state and native binding glue
    namespace QuickJs
    {
        // Native C callback behind script-callable function objects
        typedef JSValue (*ExternalHandler)(JSValueConst func_obj, JSValueConst this_val,
                                           JSValueConst* args, int args_count);

        typedef void (*NativeFreeCallback)(void* native_p);

        typedef void (*ErrorCallback)(JSValueConst error_value, void* user_p);

        // Initializes runtime, context and binding classes; safe to call multiple times
        void Initialize();

        JSRuntime* Runtime();
        JSContext* Ctx();

        JSAtom LengthAtom();
        JSAtom InternalAtom();

        // Callable and constructable object backed by a native handler; on `new` a fresh
        // object is created and passed as this, handler result is ignored
        JSValue NewExternalFunction(ExternalHandler handler);

        // Native pointer attached to any object via a hidden finalized property;
        // replacing forgets the previous pointer without invoking its callback
        void SetNativePointer(JSValueConst target, void* ptr, NativeFreeCallback freeCb);
        void* GetNativePointer(JSValueConst target, NativeFreeCallback freeCb);

        // Takes the pending context exception (owned), reporting it to the error callback
        JSValue TakeThrown();

        // Drops the pending context exception
        void ClearThrown();

        // ToInteger conversion: truncated number, 0 for NaN or on conversion error
        double ToInteger(JSValueConst value);

        void SetErrorCallback(ErrorCallback callback, void* userData);
    }
}

#endif // SCRIPTING_BACKEND_QUICKJS
