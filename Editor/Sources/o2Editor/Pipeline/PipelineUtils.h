#pragma once

#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"

using namespace o2;

namespace Editor
{
    // Helpers shared by the pipeline graph, executor, nodes and providers
    namespace PipelineUtils
    {
        // Encodes bytes as base64 (standard alphabet, padded)
        String Base64Encode(const String& bytes);

        // Decodes base64 (standard or url-safe alphabet), skipping unknown characters
        String Base64Decode(const String& text);

        // Returns FNV-1a 64 bit hash of a string
        UInt64 Fnv1a64(const String& data);

        // Returns 32 hex characters: FNV-1a of the string followed by FNV-1a of a salted copy
        String Fnv1a64Hex(const String& data);

        // Returns SHA-256 digest as 32 raw bytes
        String Sha256(const String& data);

        // Returns HMAC-SHA256 of data with the key as 32 raw bytes
        String HmacSha256(const String& key, const String& data);

        // Returns JSON with sorted object keys, excluded top-level keys skipped
        String CanonicalJson(const DataValue& value, const Vector<String>& excludeTopLevelKeys = {});

        // Returns value as string tolerant to the stored JSON type, def for objects, arrays and null
        String ValueToString(const DataValue& value, const String& def = "");

        // Returns value as number tolerant to the stored JSON type, def when it is not numeric
        float ValueToNumber(const DataValue& value, float def = 0.0f);

        // Returns whole file as binary-safe string; empty when missing
        String ReadFileBytes(const String& path);

        // Writes bytes into the file creating its folder, returns false when the file can't be opened
        bool WriteFileBytes(const String& path, const String& data);

        // Returns "size:mtime" string of a file, or "missing"
        String FileSignature(const String& path);

        // Returns raw bytes of a data:image/...;base64,... url (empty when not a base64 data url)
        String DataUrlToBytes(const String& dataUrl);

        // Returns data url with the mime and base64 encoded bytes
        String BytesToDataUrl(const String& bytes, const String& mime);

        // Returns mime type for a file extension, application/octet-stream when unknown
        String MimeForExtension(const String& ext);

        // Returns file extension for a mime type, "bin" when unknown
        String ExtensionForMime(const String& mime);

        // Returns the name a person reads for a model id: the known product name, else the id split into capitalised words
        String PrettyModelName(const String& id);

        // Returns root of the pipeline working directory (caches, uploads, settings)
        String GetWorkPath();

        // Overrides the working directory, empty restores the default under the project root
        void SetWorkPathOverride(const String& path);

        // Returns the uploads folder inside the working directory
        String GetUploadsPath();

        // Returns full path of an upload by id, empty for an empty id
        String GetUploadPath(const String& uploadId);

        // Copies a file into uploads and returns its upload id, empty on failure
        String StoreUpload(const String& sourceFile);

        // Shortens text to max characters preferring a sentence, clause or word boundary
        String ClampPromptChars(const String& text, int maxChars);

        // Parses #rgb / #rrggbb, returns false when it is not a color
        bool ParseHexColor(const String& text, Color4& color);

        // Returns #rrggbb of the color
        String ColorToHex(const Color4& color);

        // Returns human readable name of a colour, for prompts
        String ColorName(const Color4& color);

        // Returns text without leading and trailing whitespace
        String Trim(const String& text);

        // Percent-encodes everything except unreserved url characters
        String UrlEncode(const String& text);
    }
}
