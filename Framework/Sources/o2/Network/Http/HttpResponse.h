#pragma once

#include "o2/Network/Http/HttpTypes.h"
#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/Types/Containers/Map.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

#if IS_SCRIPTING_SUPPORTED
#include "o2/Scripts/ScriptValue.h"
#endif

namespace o2
{
    class Bitmap;

    // -------------------------------------------------------------------------------------------
    // HTTP response: transfer error, status code, headers and body with typed accessors for text,
    // JSON and images. Errors are reported here by value, never thrown
    // -------------------------------------------------------------------------------------------
    class HttpResponse: public IObject, public RefCounterable
    {
    public:
        HttpError error = HttpError::None; // Transfer error; None when a response arrived @SCRIPTABLE

        int status = 0; // HTTP status code, 0 when the transfer failed @SCRIPTABLE

        Map<String, String> headers; // Response headers @SCRIPTABLE

        String body; // Response body bytes; binary-safe @SCRIPTABLE

        bool fromCache = false; // True when the response was served from the HTTP cache @SCRIPTABLE

    public:
        // Default constructor
        explicit HttpResponse(RefCounter* refCounter);

        // Returns true when there is no transfer error and the status is 2xx @SCRIPTABLE
        bool IsSuccess() const;

        // Returns the header value by case-insensitive name, empty when absent @SCRIPTABLE
        String GetHeader(const String& name) const;

        // Parses the body as JSON into the document. Returns false on parse failure
        bool GetBodyJson(DataDocument& document) const;

        // Decodes the body as an image (PNG). Returns null on failure
        Ref<Bitmap> GetBodyImage() const;

#if IS_SCRIPTING_SUPPORTED
        // Returns the body parsed as JSON, or undefined on parse failure @SCRIPTABLE
        ScriptValue GetJson() const;
#endif

        IOBJECT(HttpResponse);
    };
}
// --- META ---

CLASS_BASES_META(o2::HttpResponse)
{
    BASE_CLASS(o2::IObject);
    BASE_CLASS(o2::RefCounterable);
}
END_META;
CLASS_FIELDS_META(o2::HttpResponse)
{
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().DEFAULT_VALUE(HttpError::None).NAME(error);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().DEFAULT_VALUE(0).NAME(status);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(headers);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(body);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(fromCache);
}
END_META;
CLASS_METHODS_META(o2::HttpResponse)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, IsSuccess);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(String, GetHeader, const String&);
    FUNCTION().PUBLIC().SIGNATURE(bool, GetBodyJson, DataDocument&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Bitmap>, GetBodyImage);
#if  IS_SCRIPTING_SUPPORTED
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(ScriptValue, GetJson);
#endif
}
END_META;
// --- END META ---
