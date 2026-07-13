#include "o2/stdafx.h"

#if defined(SCRIPTING_BACKEND_QUICKJS)

#include "o2/Scripts/QuickJS/QuickJSCore.h"

#include <cmath>

namespace o2
{
    namespace QuickJs
    {
        namespace
        {
            // Container pointer boxed into a hidden property object; finalizer drives native cleanup
            struct NativeHolder
            {
                void* ptr = nullptr;
                NativeFreeCallback freeCb = nullptr;
            };

            struct State
            {
                JSRuntime* runtime = nullptr;
                JSContext* context = nullptr;

                JSClassID functionClassId = 0;
                JSClassID nativeHolderClassId = 0;

                JSAtom nativeAtom = 0;
                JSAtom internalAtom = 0;
                JSAtom lengthAtom = 0;

                ErrorCallback errorCallback = nullptr;
                void* errorCallbackUser = nullptr;
            };

            State& S()
            {
                static State state;
                return state;
            }

            void NativeHolderFinalizer(JSRuntime* rt, JSValueConst val)
            {
                auto holder = (NativeHolder*)JS_GetOpaque(val, S().nativeHolderClassId);
                if (holder)
                {
                    if (holder->freeCb && holder->ptr)
                        holder->freeCb(holder->ptr);

                    delete holder;
                }
            }

            JSValue FunctionCallDispatch(JSContext* ctx, JSValueConst func_obj, JSValueConst this_val,
                                         int argc, JSValueConst* argv, int flags)
            {
                auto handler = (ExternalHandler)JS_GetOpaque(func_obj, S().functionClassId);
                if (!handler)
                    return JS_UNDEFINED;

                // Constructor call: create the object here, pass it as this, return it
                // regardless of the handler result
                if (flags & JS_CALL_FLAG_CONSTRUCTOR)
                {
                    JSValue thisObj = JS_NewObject(ctx);
                    JSValue res = handler(func_obj, thisObj, argv, argc);
                    if (JS_IsException(res))
                    {
                        JS_FreeValue(ctx, thisObj);
                        return res;
                    }

                    JS_FreeValue(ctx, res);
                    return thisObj;
                }

                return handler(func_obj, this_val, argv, argc);
            }

            NativeHolder* FindNativeHolder(JSValueConst target)
            {
                auto& s = S();
                if (!JS_IsObject(target))
                    return nullptr;

                JSPropertyDescriptor desc;
                int res = JS_GetOwnProperty(s.context, &desc, target, s.nativeAtom);
                if (res <= 0)
                {
                    if (res < 0)
                        ClearThrown();
                    return nullptr;
                }

                auto holder = (NativeHolder*)JS_GetOpaque(desc.value, s.nativeHolderClassId);
                JS_FreeValue(s.context, desc.value);
                JS_FreeValue(s.context, desc.getter);
                JS_FreeValue(s.context, desc.setter);
                return holder;
            }
        }

        void Initialize()
        {
            auto& s = S();
            if (s.runtime)
                return;

            s.runtime = JS_NewRuntime();
            s.context = JS_NewContext(s.runtime);

            JS_NewClassID(s.runtime, &s.functionClassId);
            static JSClassDef functionClassDef = { "O2NativeFunction", nullptr, nullptr, &FunctionCallDispatch, nullptr };
            JS_NewClass(s.runtime, s.functionClassId, &functionClassDef);

            JS_NewClassID(s.runtime, &s.nativeHolderClassId);
            static JSClassDef holderClassDef = { "O2NativeHolder", &NativeHolderFinalizer, nullptr, nullptr, nullptr };
            JS_NewClass(s.runtime, s.nativeHolderClassId, &holderClassDef);

            s.nativeAtom = JS_NewAtom(s.context, "__o2_native__");
            s.internalAtom = JS_NewAtom(s.context, "__o2_internal__");
            s.lengthAtom = JS_NewAtom(s.context, "length");
        }

        JSRuntime* Runtime()
        {
            Initialize();
            return S().runtime;
        }

        JSContext* Ctx()
        {
            Initialize();
            return S().context;
        }

        JSAtom LengthAtom()
        {
            Initialize();
            return S().lengthAtom;
        }

        JSAtom InternalAtom()
        {
            Initialize();
            return S().internalAtom;
        }

        JSValue NewExternalFunction(ExternalHandler handler)
        {
            auto& s = S();
            JSValue func = JS_NewObjectClass(Ctx(), s.functionClassId);
            JS_SetOpaque(func, (void*)handler);
            JS_SetConstructorBit(s.context, func, true);
            return func;
        }

        void SetNativePointer(JSValueConst target, void* ptr, NativeFreeCallback freeCb)
        {
            auto& s = S();
            Initialize();
            if (!JS_IsObject(target))
                return;

            if (auto existing = FindNativeHolder(target))
                existing->ptr = nullptr;

            auto holder = new NativeHolder{ ptr, freeCb };
            JSValue box = JS_NewObjectClass(s.context, s.nativeHolderClassId);
            JS_SetOpaque(box, holder);
            JS_DefinePropertyValue(s.context, target, s.nativeAtom, box, JS_PROP_CONFIGURABLE | JS_PROP_WRITABLE);
        }

        void* GetNativePointer(JSValueConst target, NativeFreeCallback freeCb)
        {
            Initialize();
            auto holder = FindNativeHolder(target);
            return (holder && holder->freeCb == freeCb) ? holder->ptr : nullptr;
        }

        JSValue TakeThrown()
        {
            auto& s = S();
            JSValue thrown = JS_GetException(s.context);

            if (s.errorCallback)
                s.errorCallback(thrown, s.errorCallbackUser);

            return thrown;
        }

        void ClearThrown()
        {
            JS_FreeValue(S().context, JS_GetException(S().context));
        }

        double ToInteger(JSValueConst value)
        {
            double res = 0;
            if (JS_ToFloat64(Ctx(), &res, value) < 0)
            {
                ClearThrown();
                return 0;
            }

            if (res != res)
                return 0;

            return res < 0 ? ceil(res) : floor(res);
        }

        void SetErrorCallback(ErrorCallback callback, void* userData)
        {
            auto& s = S();
            s.errorCallback = callback;
            s.errorCallbackUser = userData;
        }
    }
}

#endif // SCRIPTING_BACKEND_QUICKJS
