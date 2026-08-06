#include "o2/stdafx.h"

#if defined(SCRIPTING_BACKEND_BROWSERJS)

#if !defined(__EMSCRIPTEN__)
#error "SCRIPTING_BACKEND_BROWSERJS requires an Emscripten build"
#endif

#include "o2/Scripts/BrowserJS/BrowserJSCore.h"

#include <emscripten/emscripten.h>
#include <emscripten/em_js.h>

#include <cstdlib>

// Handle table lives at globalThis.__o2js; every primitive self-initializes through
// the shared preamble so call order does not matter.
EM_JS(void, o2js_initialize, (), {
    if (globalThis.__o2js)
        return;

    globalThis.__o2js = {
        vals: [undefined, null, true, false],
        rc: [1, 1, 1, 1],
        free: [],
        errs: new Set(),
        natives: new WeakMap(),
        internals: new WeakMap(),
        registry: new FinalizationRegistry(function(held) { _o2js_native_free(held.cb, held.ptr); }),
        dec: new TextDecoder('utf-8'),
        enc: new TextEncoder(),
        h: function(v) {
            if (v === undefined) return 0;
            if (v === null) return 1;
            if (v === true) return 2;
            if (v === false) return 3;
            var i;
            if (this.free.length) { i = this.free.pop(); this.vals[i] = v; this.rc[i] = 1; }
            else { i = this.vals.length; this.vals.push(v); this.rc.push(1); }
            return i;
        },
        hErr: function(v) {
            var i;
            if (this.free.length) { i = this.free.pop(); this.vals[i] = v; this.rc[i] = 1; }
            else { i = this.vals.length; this.vals.push(v); this.rc.push(1); }
            this.errs.add(i);
            var t = this.h(v);
            _o2js_notify_error(t);
            this.rel(t);
            return i;
        },
        acq: function(i) {
            if (i > 3 && this.rc[i] > 0) this.rc[i]++;
            return i;
        },
        rel: function(i) {
            if (i <= 3 || !(this.rc[i] > 0)) return;
            if (--this.rc[i] === 0) { this.vals[i] = undefined; this.errs.delete(i); this.free.push(i); }
        },
        cstr: function(p) {
            var heap = HEAPU8, e = p;
            while (heap[e]) e++;
            return this.dec.decode(heap.subarray(p, e));
        },
        isObj: function(v) {
            return v !== null && (typeof v === 'object' || typeof v === 'function');
        },
        mkfn: function(handlerPtr) {
            var s = this;
            var fn = function() {
                var n = arguments.length;
                var buf = _o2js_alloc(n * 4);
                for (var i = 0; i < n; i++) HEAPU32[(buf >> 2) + i] = s.h(arguments[i]);
                var fnH = s.h(fn), thisH = s.h(this);
                var resH = _o2js_call_handler(handlerPtr, fnH, thisH, buf, n);
                for (var j = 0; j < n; j++) s.rel(HEAPU32[(buf >> 2) + j]);
                _o2js_free(buf);
                s.rel(fnH); s.rel(thisH);
                var isErr = s.errs.has(resH);
                var res = s.vals[resH];
                s.rel(resH);
                if (isErr) throw res;
                return res;
            };
            return fn;
        }
    };
});

EM_JS(o2js_value_t, o2js_acquire, (o2js_value_t h), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    return s.acq(h);
});

EM_JS(void, o2js_release, (o2js_value_t h), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    s.rel(h);
});

EM_JS(o2js_value_t, o2js_number, (double v), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    return s.h(v);
});

EM_JS(o2js_value_t, o2js_string, (const char* p), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    return s.h(s.cstr(p));
});

EM_JS(o2js_value_t, o2js_object, (), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    return s.h({});
});

EM_JS(o2js_value_t, o2js_array, (uint32_t size), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    return s.h(new Array(size));
});

EM_JS(o2js_value_t, o2js_external_function, (o2js_external_handler_t handler), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    return s.h(s.mkfn(handler));
});

EM_JS(int, o2js_get_value_type, (o2js_value_t h), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    if (s.errs.has(h)) return 8;
    var v = s.vals[h];
    switch (typeof v)
    {
        case 'undefined': return 1;
        case 'boolean': return 3;
        case 'number': return 4;
        case 'string': return 5;
        case 'function': return 7;
        case 'symbol': return 9;
        case 'bigint': return 10;
        case 'object': return v === null ? 2 : 6;
    }
    return 0;
});

EM_JS(bool, o2js_is_array, (o2js_value_t h), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    return !s.errs.has(h) && Array.isArray(s.vals[h]);
});

EM_JS(bool, o2js_is_constructor, (o2js_value_t h), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    var v = s.vals[h];
    if (typeof v !== 'function') return false;
    try { Reflect.construct(function() {}, [], v); return true; } catch (e) { return false; }
});

EM_JS(bool, o2js_is_error, (o2js_value_t h), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    return s.errs.has(h);
});

EM_JS(bool, o2js_to_boolean, (o2js_value_t h), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    return !!s.vals[h];
});

EM_JS(o2js_value_t, o2js_to_number, (o2js_value_t h), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    try { return s.h(Number(s.vals[h])); } catch (e) { return s.hErr(e); }
});

EM_JS(o2js_value_t, o2js_to_string, (o2js_value_t h), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    try { return s.h(String(s.vals[h])); } catch (e) { return s.hErr(e); }
});

EM_JS(double, o2js_as_integer, (o2js_value_t h), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    var n = Number(s.vals[h]);
    if (n !== n) return 0;
    return Math.trunc(n);
});

EM_JS(double, o2js_get_number, (o2js_value_t h), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    var v = s.vals[h];
    return typeof v === 'number' ? v : 0;
});

EM_JS(uint32_t, o2js_get_string_length, (o2js_value_t h), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    var v = s.vals[h];
    // length in UTF-8 bytes: o2js_string_to_buffer copies UTF-8, not UTF-16 units
    return typeof v === 'string' ? s.enc.encode(v).length : 0;
});

EM_JS(uint32_t, o2js_string_to_buffer, (o2js_value_t h, char* buf, uint32_t size), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    var v = s.vals[h];
    if (typeof v !== 'string') return 0;
    var bytes = s.enc.encode(v);
    var n = bytes.length <= size ? bytes.length : size;
    HEAPU8.set(bytes.subarray(0, n), buf);
    return n;
});

EM_JS(uint32_t, o2js_get_array_length, (o2js_value_t h), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    var v = s.vals[h];
    return Array.isArray(v) ? v.length : 0;
});

EM_JS(o2js_value_t, o2js_get_property, (o2js_value_t obj, o2js_value_t name), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    try { return s.h(s.vals[obj][s.vals[name]]); } catch (e) { return s.hErr(e); }
});

EM_JS(o2js_value_t, o2js_get_property_by_index, (o2js_value_t obj, uint32_t idx), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    try { return s.h(s.vals[obj][idx]); } catch (e) { return s.hErr(e); }
});

EM_JS(o2js_value_t, o2js_set_property, (o2js_value_t obj, o2js_value_t name, o2js_value_t value), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    try { s.vals[obj][s.vals[name]] = s.vals[value]; return 2; } catch (e) { return s.hErr(e); }
});

EM_JS(o2js_value_t, o2js_set_property_by_index, (o2js_value_t obj, uint32_t idx, o2js_value_t value), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    try { s.vals[obj][idx] = s.vals[value]; return 2; } catch (e) { return s.hErr(e); }
});

EM_JS(bool, o2js_delete_property, (o2js_value_t obj, o2js_value_t name), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    try { return delete s.vals[obj][s.vals[name]]; } catch (e) { return false; }
});

EM_JS(bool, o2js_delete_property_by_index, (o2js_value_t obj, uint32_t idx), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    try { return delete s.vals[obj][idx]; } catch (e) { return false; }
});

EM_JS(o2js_value_t, o2js_get_internal_property, (o2js_value_t obj, o2js_value_t name), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    var m = s.internals.get(s.vals[obj]);
    if (!m) return 0;
    var k = String(s.vals[name]);
    return m.has(k) ? s.h(m.get(k)) : 0;
});

EM_JS(bool, o2js_set_internal_property, (o2js_value_t obj, o2js_value_t name, o2js_value_t value), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    var t = s.vals[obj];
    if (!s.isObj(t)) return false;
    var m = s.internals.get(t);
    if (!m) { m = new Map(); s.internals.set(t, m); }
    m.set(String(s.vals[name]), s.vals[value]);
    return true;
});

EM_JS(o2js_value_t, o2js_get_own_property, (o2js_value_t obj, o2js_value_t name), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    var v = s.vals[obj];
    if (!s.isObj(v)) return 0;
    var d = Object.getOwnPropertyDescriptor(v, s.vals[name]);
    if (!d || !('value' in d)) return 0;
    return s.h(d.value);
});

EM_JS(o2js_value_t, o2js_define_accessor, (o2js_value_t obj, o2js_value_t name, o2js_value_t getter, o2js_value_t setter), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    var desc = { get: s.vals[getter], set: s.vals[setter], enumerable: true };
    try { Object.defineProperty(s.vals[obj], s.vals[name], desc); return 2; } catch (e) { return s.hErr(e); }
});

EM_JS(o2js_value_t, o2js_get_property_names, (o2js_value_t obj), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    var v = s.vals[obj];
    var res = [];
    if (s.isObj(v))
    {
        var names = Object.getOwnPropertyNames(v);
        for (var i = 0; i < names.length; i++)
        {
            var k = names[i];
            var d = Object.getOwnPropertyDescriptor(v, k);
            if (!d || !d.enumerable || !d.configurable || d.get || d.set || !d.writable) continue;
            var ki = +k;
            if (Number.isInteger(ki) && ki >= 0 && String(ki) === k) continue;
            res.push(k);
        }
    }
    return s.h(res);
});

EM_JS(o2js_value_t, o2js_get_prototype, (o2js_value_t obj), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    var v = s.vals[obj];
    if (!s.isObj(v)) return s.h(null);
    return s.h(Object.getPrototypeOf(v));
});

EM_JS(o2js_value_t, o2js_set_prototype, (o2js_value_t obj, o2js_value_t proto), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    var p = s.vals[proto];
    if (p === undefined) p = null;
    try { Object.setPrototypeOf(s.vals[obj], p); return 2; } catch (e) { return s.hErr(e); }
});

EM_JS(void, o2js_set_native_pointer, (o2js_value_t obj, void* ptr, o2js_native_free_callback_t freeCb), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    var v = s.vals[obj];
    if (!s.isObj(v)) return;
    if (s.natives.has(v)) s.registry.unregister(v);
    s.natives.set(v, { ptr: ptr, cb: freeCb });
    if (ptr && freeCb) s.registry.register(v, { ptr: ptr, cb: freeCb }, v);
});

EM_JS(void*, o2js_get_native_pointer, (o2js_value_t obj, o2js_native_free_callback_t freeCb), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    var v = s.vals[obj];
    if (!s.isObj(v)) return 0;
    var e = s.natives.get(v);
    return (e && e.cb === freeCb) ? e.ptr : 0;
});

EM_JS(o2js_value_t, o2js_call_function, (o2js_value_t func, o2js_value_t thisVal, const o2js_value_t* args, int count), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    var fn = s.vals[func];
    if (typeof fn !== 'function') return s.hErr(new TypeError('value is not a function'));
    var a = [];
    for (var i = 0; i < count; i++) a.push(s.vals[HEAPU32[(args >> 2) + i]]);
    try { return s.h(fn.apply(s.vals[thisVal], a)); } catch (e) { return s.hErr(e); }
});

EM_JS(o2js_value_t, o2js_construct, (o2js_value_t func, const o2js_value_t* args, int count), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    var fn = s.vals[func];
    if (typeof fn !== 'function') return s.hErr(new TypeError('value is not a constructor'));
    var a = [];
    for (var i = 0; i < count; i++) a.push(s.vals[HEAPU32[(args >> 2) + i]]);
    try { return s.h(Reflect.construct(fn, a)); } catch (e) { return s.hErr(e); }
});

EM_JS(bool, o2js_equals, (o2js_value_t lhs, o2js_value_t rhs), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    try { return s.vals[lhs] == s.vals[rhs]; } catch (e) { return false; }
});

EM_JS(o2js_value_t, o2js_get_error_value, (o2js_value_t h), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    return s.h(s.vals[h]);
});

EM_JS(o2js_value_t, o2js_get_global, (), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    return s.h(globalThis);
});

// Parse only syntax-checks and stores the source; run must reproduce script semantics:
// top-level class/let/const become global bindings, and the completion value is returned.
// A synchronously injected <script> element gives true script scoping (but no completion value),
// so sources that read as a single expression run through indirect eval instead.
EM_JS(o2js_value_t, o2js_parse, (const char* src, uint32_t srcLen, const char* name, uint32_t nameLen), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    var text = s.dec.decode(HEAPU8.subarray(src, src + srcLen));
    try { new Function(text); } catch (e) { return s.hErr(e); }

    var B = String.fromCharCode(92);
    var expr = false;
    if (!new RegExp('^' + B + 's*(class|function|var|let|const)([^A-Za-z0-9_$]|$)').test(text))
    {
        try { new Function('return (' + text + '\n);'); expr = true; } catch (e) {}
    }

    var named = text;
    if (!expr)
    {
        var classRe = new RegExp('(^|' + B + 'n)' + B + 's*class' + B + 's+([A-Za-z_$][A-Za-z0-9_$]*)', 'g');
        var m;
        while ((m = classRe.exec(text)) !== null)
            named += '\ntry { globalThis.' + m[2] + ' = ' + m[2] + '; } catch (__o2e) {}';
    }
    if (nameLen > 0)
        named += '\n//# sourceURL=' + s.dec.decode(HEAPU8.subarray(name, name + nameLen));

    return s.h({ __o2src: named, __o2raw: text, __o2expr: expr });
});

EM_JS(o2js_value_t, o2js_run, (o2js_value_t h), {
    var s = globalThis.__o2js || (o2js_initialize(), globalThis.__o2js);
    var p = s.vals[h];
    if (!p || typeof p.__o2src !== 'string') return s.hErr(new TypeError('invalid parse result'));

    if (p.__o2expr)
    {
        try { return s.h((0, eval)('(' + p.__o2raw + '\n)')); } catch (e) { return s.hErr(e); }
    }

    if (typeof document !== 'undefined')
    {
        var err = null;
        var onerr = function(ev) { err = ev.error || new Error(String(ev.message)); ev.preventDefault(); };
        window.addEventListener('error', onerr);
        var el = document.createElement('script');
        el.textContent = p.__o2src;
        document.head.appendChild(el);
        el.remove();
        window.removeEventListener('error', onerr);
        if (err) return s.hErr(err);
        return 0;
    }

    try { return s.h((0, eval)(p.__o2src)); } catch (e) { return s.hErr(e); }
});

EM_JS(int, o2js_get_used_memory, (), {
    if (typeof performance !== 'undefined' && performance.memory && performance.memory.usedJSHeapSize)
        return performance.memory.usedJSHeapSize | 0;
    return 0;
});

namespace
{
    o2js_error_created_callback_t gErrorCallback = nullptr;
    void* gErrorCallbackUser = nullptr;
}

extern "C"
{
    EMSCRIPTEN_KEEPALIVE void* o2js_alloc(int size)
    {
        return malloc(size > 0 ? (size_t)size : 1);
    }

    EMSCRIPTEN_KEEPALIVE void o2js_free(void* ptr)
    {
        free(ptr);
    }

    EMSCRIPTEN_KEEPALIVE o2js_value_t o2js_call_handler(void* handler, o2js_value_t funcObj,
                                                        o2js_value_t thisVal, o2js_value_t* args, int count)
    {
        return ((o2js_external_handler_t)handler)(funcObj, thisVal, args, count);
    }

    EMSCRIPTEN_KEEPALIVE void o2js_native_free(void* freeCb, void* ptr)
    {
        if (freeCb && ptr)
            ((o2js_native_free_callback_t)freeCb)(ptr);
    }

    EMSCRIPTEN_KEEPALIVE void o2js_notify_error(o2js_value_t errorValue)
    {
        if (gErrorCallback)
            gErrorCallback(errorValue, gErrorCallbackUser);
    }
}

void o2js_set_error_created_callback(o2js_error_created_callback_t callback, void* userData)
{
    gErrorCallback = callback;
    gErrorCallbackUser = userData;
}

#endif // SCRIPTING_BACKEND_BROWSERJS
