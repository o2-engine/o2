#pragma once
#include "DataValue.h"
#include "o2/Utils/Memory/Allocators/StackAllocator.h"

namespace o2
{
    // Parses json document into DataDocument. It uses "Insitu" parse method: all strings will be referenced to buffer
    bool ParseJsonInplace(char* str, DataDocument& document);

    // Parses json document into DataDocumen
    bool ParseJson(const char* str, DataDocument& document);

    // Writes data into json string
    void WriteJson(String& str, const DataDocument& document);

    // -------------------------------------------------------------------
    // Json data document parser handler. Build DataDocument DOM structure
    // -------------------------------------------------------------------
    class JsonDataDocumentParseHandler
    {
    public:
        DataDocument& document;

        StackAllocator stack;

    public:
        JsonDataDocumentParseHandler(DataDocument& document);

        bool Null();
        bool Bool(bool value);
        bool Int(int value);
        bool Uint(unsigned value);
        bool Int64(int64_t value);
        bool Uint64(uint64_t value);
        bool Double(double value);
        bool String(const char* str, unsigned length, bool copy);
        bool RawNumber(const char* str, unsigned length, bool copy);
        bool StartObject();
        bool Key(const char* str, unsigned length, bool copy);
        bool EndObject(unsigned memberCount);
        bool StartArray();
        bool EndArray(unsigned elementCount);
    };
}
