#include "o2/stdafx.h"

#ifdef PLATFORM_WASM

#include "WebServerFS.h"

#include <emscripten.h>
#include <filesystem>

EM_JS(int, o2webfs_enabled, (), {
    return (typeof window !== 'undefined' && window.o2fsEndpoint) ? 1 : 0;
});

// Synchronous mirror: structural ops POST {endpoint}/fs/op, writes POST the
// file bytes read straight from MEMFS. Keeps o2's synchronous FileSystem
// contract — when the engine call returns, the server session matches MEMFS.
EM_JS(int, o2webfs_op, (const char* op, const char* path, const char* path2, double mtime), {
    try {
        if (typeof window === 'undefined' || !window.o2fsEndpoint)
            return 0;

        var opStr = UTF8ToString(op);
        var pathStr = UTF8ToString(path);
        var path2Str = path2 ? UTF8ToString(path2) : null;
        var xhr = new XMLHttpRequest();

        if (opStr === 'write') {
            var data;
            try { data = FS.readFile(pathStr); }
            catch (e) { console.error('[o2webfs] readFile failed:', pathStr, e); return -3; }
            xhr.open('POST', window.o2fsEndpoint + '/fs/write?path=' + encodeURIComponent(pathStr), false);
            xhr.setRequestHeader('Content-Type', 'application/octet-stream');
            xhr.send(data);
        } else {
            xhr.open('POST', window.o2fsEndpoint + '/fs/op', false);
            xhr.setRequestHeader('Content-Type', 'application/json');
            xhr.send(JSON.stringify({ op: opStr, path: pathStr, path2: path2Str, mtime: mtime }));
        }

        if (xhr.status >= 200 && xhr.status < 300)
            return 0;

        console.error('[o2webfs]', opStr, pathStr, '->', xhr.status, xhr.responseText);
        return xhr.status || -1;
    } catch (e) {
        console.error('[o2webfs]', e);
        return -2;
    }
});

// Asynchronous mirror for bulk output (BuiltAssets): the in-browser asset
// builder writes hundreds of files, and a sync XHR per file would stall the
// editor for minutes over a real network. A promise chain keeps op order;
// file bytes are captured from MEMFS before queueing.
EM_JS(void, o2webfs_op_async, (const char* op, const char* path, const char* path2, double mtime), {
    try {
        if (typeof window === 'undefined' || !window.o2fsEndpoint)
            return;

        var opStr = UTF8ToString(op);
        var pathStr = UTF8ToString(path);
        var path2Str = path2 ? UTF8ToString(path2) : null;

        var data = null;
        if (opStr === 'write') {
            try { data = FS.readFile(pathStr); }  // copy taken now, sent later
            catch (e) { console.error('[o2webfs] readFile failed:', pathStr, e); return; }
        }

        window.__o2MirrorQueue = (window.__o2MirrorQueue || Promise.resolve()).then(function () {
            if (opStr === 'write')
                return fetch(window.o2fsEndpoint + '/fs/write?path=' + encodeURIComponent(pathStr), {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/octet-stream' },
                    body: data,
                });
            return fetch(window.o2fsEndpoint + '/fs/op', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ op: opStr, path: pathStr, path2: path2Str, mtime: mtime }),
            });
        }).then(function (r) {
            if (r && !r.ok) console.error('[o2webfs.async]', opStr, pathStr, '->', r.status);
        }).catch(function (e) {
            console.error('[o2webfs.async]', opStr, pathStr, e);
        });
    } catch (e) {
        console.error('[o2webfs.async]', e);
    }
});

namespace o2
{
    namespace WebFS
    {
        namespace
        {
            // Resolves an engine path (usually relative to /project/Bin/WebAssembly)
            // to an absolute normalized VFS path; empty when the path must not be
            // mirrored (outside the project or inside Bin — logs, temporaries)
            String NormalizeForMirror(const String& path)
            {
                if (path.IsEmpty())
                    return String();

                std::filesystem::path p(path.Data());
                std::error_code ec;
                auto abs = std::filesystem::absolute(p, ec);
                if (ec)
                    return String();

                String norm(abs.lexically_normal().generic_string().c_str());

                if (!norm.StartsWith("/project/") || norm.StartsWith("/project/Bin"))
                    return String();

                return norm;
            }

            // Built assets are derived bulk output — mirrored asynchronously
            bool IsBulkPath(const String& normalized)
            {
                return normalized.StartsWith("/project/BuiltAssets");
            }

            void SendOp(const char* op, const String& path, const String& path2 = String(),
                        double mtime = 0.0)
            {
                if (!o2webfs_enabled())
                    return;

                String p1 = NormalizeForMirror(path);
                if (p1.IsEmpty())
                    return;

                String p2;
                if (!path2.IsEmpty())
                {
                    p2 = NormalizeForMirror(path2);
                    if (p2.IsEmpty())
                        return;
                }

                const char* p2data = p2.IsEmpty() ? nullptr : p2.Data();
                if (IsBulkPath(p1) && (p2.IsEmpty() || IsBulkPath(p2)))
                    o2webfs_op_async(op, p1.Data(), p2data, mtime);
                else
                    o2webfs_op(op, p1.Data(), p2data, mtime);
            }
        }

        bool IsEnabled()
        {
            return o2webfs_enabled() != 0;
        }

        void NotifyFileWritten(const String& path) { SendOp("write", path); }
        void NotifyFileDeleted(const String& path) { SendOp("delete", path); }
        void NotifyFolderCreated(const String& path) { SendOp("mkdir", path); }
        void NotifyFolderRemoved(const String& path) { SendOp("rmdir", path); }
        void NotifyMoved(const String& from, const String& to) { SendOp("move", from, to); }
        void NotifyFileCopied(const String& from, const String& to) { SendOp("copyfile", from, to); }
        void NotifyFolderCopied(const String& from, const String& to) { SendOp("copytree", from, to); }

        void NotifyEditDateSet(const String& path, long long unixTimeSeconds)
        {
            SendOp("utime", path, String(), (double)unixTimeSeconds);
        }
    }
}

#endif // PLATFORM_WASM
