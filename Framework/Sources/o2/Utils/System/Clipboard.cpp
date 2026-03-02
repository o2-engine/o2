#include "o2/stdafx.h"
#include "Clipboard.h"

#if defined PLATFORM_WINDOWS
#include <Windows.h>
#include <shlobj.h>
#elif defined PLATFORM_MAC
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace o2
{

    void Clipboard::SetText(const WString& text)
    {
#if defined PLATFORM_WINDOWS
        if (OpenClipboard(NULL))
        {
            HGLOBAL hgBuffer;
            wchar_t* chBuffer;
            EmptyClipboard();
            hgBuffer = GlobalAlloc(GMEM_DDESHARE, (text.Length() + 1)*sizeof(wchar_t));
            chBuffer = (wchar_t*)GlobalLock(hgBuffer);
            memcpy(chBuffer, text.Data(), (text.Length() + 1)*sizeof(wchar_t));
            GlobalUnlock(hgBuffer);
            SetClipboardData(CF_UNICODETEXT, hgBuffer);
            CloseClipboard();
        }
#elif defined PLATFORM_MAC
        String utf8Text;
        ConvertString(utf8Text, text);

        PasteboardRef pasteboard = nullptr;
        if (PasteboardCreate(kPasteboardClipboard, &pasteboard) == noErr)
        {
            PasteboardClear(pasteboard);
            CFDataRef cfData = CFDataCreate(kCFAllocatorDefault, (const UInt8*)utf8Text.Data(), (CFIndex)utf8Text.Length());
            if (cfData)
            {
                PasteboardPutItemFlavor(pasteboard, (PasteboardItemID)1, CFSTR("public.utf8-plain-text"), cfData, 0);
                CFRelease(cfData);
            }
            CFRelease(pasteboard);
        }
#endif
    }

    WString Clipboard::GetText()
    {
#if defined PLATFORM_WINDOWS
        WString res;

        if (OpenClipboard(NULL))
        {
            HANDLE hData = GetClipboardData(CF_UNICODETEXT);
            wchar_t* chBuffer = (wchar_t*)GlobalLock(hData);
            res = chBuffer;
            GlobalUnlock(hData);
            CloseClipboard();
        }

        return res;
#elif PLATFORM_ANDROID
        return WString();
#elif PLATFORM_MAC
        {
            WString res;
            PasteboardRef pasteboard = nullptr;
            if (PasteboardCreate(kPasteboardClipboard, &pasteboard) == noErr)
            {
                PasteboardSynchronize(pasteboard);

                ItemCount itemCount = 0;
                PasteboardGetItemCount(pasteboard, &itemCount);
                for (ItemCount i = 0; i < itemCount; i++)
                {
                    PasteboardItemID itemId = 0;
                    PasteboardGetItemIdentifier(pasteboard, i + 1, &itemId);
                    CFArrayRef flavorTypeArray = nullptr;
                    PasteboardCopyItemFlavors(pasteboard, itemId, &flavorTypeArray);
                    if (flavorTypeArray)
                    {
                        CFDataRef cfData = nullptr;
                        if (PasteboardCopyItemFlavorData(pasteboard, itemId, CFSTR("public.utf8-plain-text"), &cfData) == noErr && cfData)
                        {
                            CFIndex length = CFDataGetLength(cfData);
                            String utf8Str;
                            utf8Str.resize((size_t)length + 1);
                            CFDataGetBytes(cfData, CFRangeMake(0, length), (UInt8*)&utf8Str[0]);
                            utf8Str[(size_t)length] = '\0';
                            ConvertString(res, utf8Str);
                            CFRelease(cfData);
                            CFRelease(flavorTypeArray);
                            CFRelease(pasteboard);
                            return res;
                        }
                        CFRelease(flavorTypeArray);
                    }
                }
                CFRelease(pasteboard);
            }
            return res;
        }
#elif PLATFORM_IOS
        return WString();
#elif PLATFORM_LINUX
        return WString();
#endif
    }

#undef CopyFile

    void Clipboard::CopyFile(const WString& path)
    {
#if defined PLATFORM_WINDOWS
        if (OpenClipboard(NULL))
        {
            EmptyClipboard();

            int size = sizeof(DROPFILES) + (path.Length() + 2)*sizeof(WCHAR);
            HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
            DROPFILES *df = (DROPFILES*)GlobalLock(hGlobal);
            ZeroMemory(df, size);
            df->pFiles = sizeof(DROPFILES);
            df->fWide = TRUE;
            LPWSTR ptr = (LPWSTR)(df + 1);
            lstrcpyW(ptr, path.Data());
            GlobalUnlock(hGlobal);
            SetClipboardData(CF_HDROP, hGlobal);
            CloseClipboard();
        }
#elif defined PLATFORM_MAC
        String utf8Path;
        ConvertString(utf8Path, path);

        CFStringRef pathRef = CFStringCreateWithCString(kCFAllocatorDefault, utf8Path.Data(), kCFStringEncodingUTF8);
        if (pathRef)
        {
            CFURLRef urlRef = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, pathRef, kCFURLPOSIXPathStyle, false);
            if (urlRef)
            {
                CFStringRef urlStringRef = CFURLGetString(urlRef);
                if (urlStringRef)
                {
                    CFIndex maxSize = CFStringGetMaximumSizeForEncoding(CFStringGetLength(urlStringRef), kCFStringEncodingUTF8) + 1;
                    Vector<char> urlBuffer;
                    urlBuffer.Resize((int)maxSize);
                    if (CFStringGetCString(urlStringRef, urlBuffer.Data(), maxSize, kCFStringEncodingUTF8))
                    {
                        PasteboardRef pasteboard = nullptr;
                        if (PasteboardCreate(kPasteboardClipboard, &pasteboard) == noErr)
                        {
                            PasteboardClear(pasteboard);
                            CFDataRef cfData = CFDataCreate(kCFAllocatorDefault, (const UInt8*)urlBuffer.Data(), (CFIndex)strlen(urlBuffer.Data()));
                            if (cfData)
                            {
                                PasteboardPutItemFlavor(pasteboard, (PasteboardItemID)1, CFSTR("public.file-url"), cfData, 0);
                                CFRelease(cfData);
                            }
                            CFRelease(pasteboard);
                        }
                    }
                }
                CFRelease(urlRef);
            }
            CFRelease(pathRef);
        }
#endif
    }

    void Clipboard::CopyFiles(const Vector<WString>& paths)
    {
#if defined PLATFORM_WINDOWS
        if (OpenClipboard(NULL))
        {
            EmptyClipboard();

            int size = sizeof(DROPFILES) + (paths.Sum<int>([](const WString& x) { return x.Length() + 1; }) + 1)*sizeof(WCHAR);
            HGLOBAL hGlobal = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_DDESHARE, size);
            DROPFILES *df = (DROPFILES*)GlobalLock(hGlobal);
            ZeroMemory(df, size);
            df->pFiles = sizeof(DROPFILES);
            df->fWide = TRUE;
            LPWSTR ptr = (LPWSTR)(df + 1);

            for (auto& path : paths)
            {
                lstrcpyW(ptr, path.Data());
                ptr += path.Length();
                *ptr = '\0';
                ptr++;
            }

            *ptr = '\0';

            GlobalUnlock(hGlobal);
            SetClipboardData(CF_HDROP, hGlobal);
            CloseClipboard();
        }
#elif defined PLATFORM_MAC
        PasteboardRef pasteboard = nullptr;
        if (PasteboardCreate(kPasteboardClipboard, &pasteboard) == noErr)
        {
            PasteboardClear(pasteboard);

            for (int i = 0; i < paths.Count(); i++)
            {
                String utf8Path;
                ConvertString(utf8Path, paths[i]);

                CFStringRef pathRef = CFStringCreateWithCString(kCFAllocatorDefault, utf8Path.Data(), kCFStringEncodingUTF8);
                if (pathRef)
                {
                    CFURLRef urlRef = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, pathRef, kCFURLPOSIXPathStyle, false);
                    if (urlRef)
                    {
                        CFStringRef urlStringRef = CFURLGetString(urlRef);
                        if (urlStringRef)
                        {
                            CFIndex maxSize = CFStringGetMaximumSizeForEncoding(CFStringGetLength(urlStringRef), kCFStringEncodingUTF8) + 1;
                            Vector<char> urlBuffer;
                            urlBuffer.Resize((int)maxSize);
                            if (CFStringGetCString(urlStringRef, urlBuffer.Data(), maxSize, kCFStringEncodingUTF8))
                            {
                                CFDataRef cfData = CFDataCreate(kCFAllocatorDefault, (const UInt8*)urlBuffer.Data(), (CFIndex)strlen(urlBuffer.Data()));
                                if (cfData)
                                {
                                    PasteboardPutItemFlavor(pasteboard, (PasteboardItemID)(intptr_t)(i + 1), CFSTR("public.file-url"), cfData, 0);
                                    CFRelease(cfData);
                                }
                            }
                        }
                        CFRelease(urlRef);
                    }
                    CFRelease(pathRef);
                }
            }

            CFRelease(pasteboard);
        }
#endif
    }

    Vector<WString> Clipboard::GetCopyFiles()
    {
        Vector<WString> res;

#if defined PLATFORM_WINDOWS
        if (OpenClipboard(NULL))
        {
            HANDLE hData = GetClipboardData(CF_HDROP);
            if (hData)
            {
                DROPFILES* df = (DROPFILES*)GlobalLock(hData);

                wchar_t* files = (wchar_t*)(df + 1);
                wchar_t buf[MAX_PATH];
                int bufLen = 0;
                int i = 0;

                while (true)
                {
                    buf[bufLen++] = files[i];

                    if (files[i] == '\0')
                    {
                        if (bufLen == 1)
                            break;

                        res.Add(buf);
                        bufLen = 0;
                    }

                    i++;
                }

                GlobalUnlock(hData);
            }

            CloseClipboard();
        }
#elif defined PLATFORM_MAC
        PasteboardRef pasteboard = nullptr;
        if (PasteboardCreate(kPasteboardClipboard, &pasteboard) == noErr)
        {
            PasteboardSynchronize(pasteboard);

            ItemCount itemCount = 0;
            PasteboardGetItemCount(pasteboard, &itemCount);
            for (ItemCount i = 0; i < itemCount; i++)
            {
                PasteboardItemID itemId = 0;
                PasteboardGetItemIdentifier(pasteboard, i + 1, &itemId);
                CFDataRef cfData = nullptr;
                if (PasteboardCopyItemFlavorData(pasteboard, itemId, CFSTR("public.file-url"), &cfData) == noErr && cfData)
                {
                    CFIndex length = CFDataGetLength(cfData);
                    Vector<char> urlBuffer;
                    urlBuffer.Resize((int)length + 1);
                    CFDataGetBytes(cfData, CFRangeMake(0, length), (UInt8*)urlBuffer.Data());
                    urlBuffer[(int)length] = '\0';

                    CFURLRef urlRef = CFURLCreateWithBytes(kCFAllocatorDefault, (const UInt8*)urlBuffer.Data(), length, kCFStringEncodingUTF8, nullptr);
                    if (urlRef)
                    {
                        CFStringRef pathRef = CFURLCopyFileSystemPath(urlRef, kCFURLPOSIXPathStyle);
                        if (pathRef)
                        {
                            CFIndex maxSize = CFStringGetMaximumSizeForEncoding(CFStringGetLength(pathRef), kCFStringEncodingUTF8) + 1;
                            Vector<char> pathBuffer;
                            pathBuffer.Resize((int)maxSize);
                            if (CFStringGetCString(pathRef, pathBuffer.Data(), maxSize, kCFStringEncodingUTF8))
                            {
                                String utf8Path(pathBuffer.Data());
                                WString wPath;
                                ConvertString(wPath, utf8Path);
                                res.Add(wPath);
                            }
                            CFRelease(pathRef);
                        }
                        CFRelease(urlRef);
                    }
                    CFRelease(cfData);
                }
            }
            CFRelease(pasteboard);
        }
#endif

        return res;
    }

}
