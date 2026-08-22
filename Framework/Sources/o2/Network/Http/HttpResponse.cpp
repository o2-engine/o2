#include "o2/stdafx.h"
#include "HttpResponse.h"

#include "o2/Utils/Bitmap/Bitmap.h"

namespace o2
{
    HttpResponse::HttpResponse(RefCounter* refCounter):
        RefCounterable(refCounter)
    {}

    bool HttpResponse::IsSuccess() const
    {
        return error == HttpError::None && status >= 200 && status < 300;
    }

    String HttpResponse::GetHeader(const String& name) const
    {
        for (auto& kv : headers)
        {
            if (kv.first.length() != name.length())
                continue;

            bool equal = true;
            for (size_t i = 0; i < name.length() && equal; i++)
            {
                char a = kv.first[i], b = name[i];
                if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                equal = a == b;
            }

            if (equal)
                return kv.second;
        }

        return String();
    }

    bool HttpResponse::GetBodyJson(DataDocument& document) const
    {
        return document.LoadFromData(body, DataDocument::Format::JSON);
    }

    Ref<Bitmap> HttpResponse::GetBodyImage() const
    {
        auto bitmap = mmake<Bitmap>();
        if (!bitmap->LoadFromMemory((const UInt8*)body.data(), (UInt)body.size()))
            return nullptr;

        return bitmap;
    }

#if IS_SCRIPTING_SUPPORTED
    ScriptValue HttpResponse::GetJson() const
    {
        DataDocument document;
        if (!GetBodyJson(document))
            return ScriptValue();

        ScriptValue result;
        document.Get(result);
        return result;
    }
#endif
}
// --- META ---

DECLARE_CLASS(o2::HttpResponse, o2__HttpResponse);
// --- END META ---
