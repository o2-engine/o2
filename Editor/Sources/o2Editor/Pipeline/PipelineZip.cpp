#include "o2Editor/stdafx.h"
#include "PipelineZip.h"

#include <cstring>
#include <string>

// zlib's global Byte clashes with o2::Byte once the o2 namespace is open, so zlib gets its own name for it
#define Byte ZlibByte
#include "3rdPartyLibs/zlib/zlib.h"
#undef Byte

namespace Editor
{
    namespace PipelineZip
    {
        static const unsigned int localHeaderSignature = 0x04034b50;
        static const unsigned int centralHeaderSignature = 0x02014b50;
        static const unsigned int endRecordSignature = 0x06054b50;
        static const size_t endRecordSize = 22;
        static const size_t maxCommentSize = 65535;

        static unsigned int ReadU16(const unsigned char* p)
        {
            return p[0] | (p[1] << 8);
        }

        static unsigned int ReadU32(const unsigned char* p)
        {
            return p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned int)p[3] << 24);
        }

        static void PutU16(std::string& out, unsigned int value)
        {
            out.push_back((char)(value & 0xff));
            out.push_back((char)((value >> 8) & 0xff));
        }

        static void PutU32(std::string& out, unsigned int value)
        {
            PutU16(out, value & 0xffff);
            PutU16(out, (value >> 16) & 0xffff);
        }

        static bool Inflate(const unsigned char* src, size_t srcSize, size_t dstSize, std::string& out)
        {
            out.assign(dstSize, '\0');
            if (dstSize == 0)
                return true;

            z_stream stream;
            std::memset(&stream, 0, sizeof(stream));
            if (inflateInit2(&stream, -MAX_WBITS) != Z_OK)
                return false;

            stream.next_in = (Bytef*)src;
            stream.avail_in = (uInt)srcSize;
            stream.next_out = (Bytef*)&out[0];
            stream.avail_out = (uInt)dstSize;
            int result = inflate(&stream, Z_FINISH);
            bool ok = (result == Z_STREAM_END || result == Z_BUF_ERROR) && stream.total_out == dstSize;
            inflateEnd(&stream);
            return ok;
        }

        static bool Deflate(const std::string& src, std::string& out)
        {
            z_stream stream;
            std::memset(&stream, 0, sizeof(stream));
            if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK)
                return false;

            out.assign(deflateBound(&stream, (uLong)src.size()), '\0');
            stream.next_in = (Bytef*)src.data();
            stream.avail_in = (uInt)src.size();
            stream.next_out = (Bytef*)&out[0];
            stream.avail_out = (uInt)out.size();
            bool ok = deflate(&stream, Z_FINISH) == Z_STREAM_END;
            out.resize(stream.total_out);
            deflateEnd(&stream);
            return ok;
        }

        bool IsZip(const String& bytes)
        {
            return bytes.Length() >= 4 && ReadU32((const unsigned char*)bytes.Data()) == localHeaderSignature;
        }

        bool Read(const String& bytes, Vector<Entry>& entries, String& error)
        {
            entries.Clear();
            const unsigned char* data = (const unsigned char*)bytes.Data();
            size_t size = (size_t)bytes.Length();
            if (size < endRecordSize)
            {
                error = "too short for a zip archive";
                return false;
            }

            // The end record sits before an optional comment, so it is searched backwards
            size_t endPos = 0;
            bool found = false;
            size_t minPos = size > endRecordSize + maxCommentSize ? size - endRecordSize - maxCommentSize : 0;
            for (size_t pos = size - endRecordSize; ; pos--)
            {
                if (ReadU32(data + pos) == endRecordSignature)
                {
                    endPos = pos;
                    found = true;
                    break;
                }

                if (pos == minPos)
                    break;
            }

            if (!found)
            {
                error = "end of central directory not found";
                return false;
            }

            unsigned int count = ReadU16(data + endPos + 10);
            unsigned int dirSize = ReadU32(data + endPos + 12);
            unsigned int dirOffset = ReadU32(data + endPos + 16);
            if (count == 0xffff || dirOffset == 0xffffffff)
            {
                error = "zip64 archives are not supported";
                return false;
            }

            if ((size_t)dirOffset + dirSize > size)
            {
                error = "central directory out of bounds";
                return false;
            }

            size_t pos = dirOffset;
            for (unsigned int i = 0; i < count; i++)
            {
                if (pos + 46 > size || ReadU32(data + pos) != centralHeaderSignature)
                {
                    error = "broken central directory";
                    return false;
                }

                unsigned int method = ReadU16(data + pos + 10);
                unsigned int compressedSize = ReadU32(data + pos + 20);
                unsigned int uncompressedSize = ReadU32(data + pos + 24);
                unsigned int nameLength = ReadU16(data + pos + 28);
                unsigned int extraLength = ReadU16(data + pos + 30);
                unsigned int commentLength = ReadU16(data + pos + 32);
                unsigned int localOffset = ReadU32(data + pos + 42);
                if (pos + 46 + nameLength > size)
                {
                    error = "broken central directory";
                    return false;
                }

                String name(std::string((const char*)data + pos + 46, nameLength));
                pos += 46 + nameLength + extraLength + commentLength;

                if (name.EndsWith("/"))
                    continue;

                if (compressedSize == 0xffffffff || uncompressedSize == 0xffffffff)
                {
                    error = "zip64 entries are not supported: " + name;
                    return false;
                }

                if ((size_t)localOffset + 30 > size || ReadU32(data + localOffset) != localHeaderSignature)
                {
                    error = "broken local header: " + name;
                    return false;
                }

                size_t dataStart = localOffset + 30 + ReadU16(data + localOffset + 26) + ReadU16(data + localOffset + 28);
                if (dataStart + compressedSize > size)
                {
                    error = "entry out of bounds: " + name;
                    return false;
                }

                std::string content;
                if (method == 0)
                    content.assign((const char*)data + dataStart, compressedSize);
                else if (method == 8)
                {
                    if (!Inflate(data + dataStart, compressedSize, uncompressedSize, content))
                    {
                        error = "cannot inflate " + name;
                        return false;
                    }
                }
                else
                {
                    error = "unsupported compression method in " + name;
                    return false;
                }

                entries.Add({ name, String(content) });
            }

            return true;
        }

        String Write(const Vector<Entry>& entries, bool compress)
        {
            std::string out;
            std::string central;
            for (auto& entry : entries)
            {
                std::string name(entry.name.Data(), entry.name.Length());
                std::string raw(entry.data.Data(), entry.data.Length());
                std::string stored = raw;
                unsigned int method = 0;
                if (compress && !raw.empty())
                {
                    std::string deflated;
                    if (Deflate(raw, deflated) && deflated.size() < raw.size())
                    {
                        stored = deflated;
                        method = 8;
                    }
                }

                unsigned int crc = (unsigned int)crc32(0L, (const Bytef*)raw.data(), (uInt)raw.size());
                unsigned int offset = (unsigned int)out.size();

                PutU32(out, localHeaderSignature);
                PutU16(out, 20);
                PutU16(out, 0);
                PutU16(out, method);
                PutU16(out, 0);
                PutU16(out, 0x21);
                PutU32(out, crc);
                PutU32(out, (unsigned int)stored.size());
                PutU32(out, (unsigned int)raw.size());
                PutU16(out, (unsigned int)name.size());
                PutU16(out, 0);
                out += name;
                out += stored;

                PutU32(central, centralHeaderSignature);
                PutU16(central, 20);
                PutU16(central, 20);
                PutU16(central, 0);
                PutU16(central, method);
                PutU16(central, 0);
                PutU16(central, 0x21);
                PutU32(central, crc);
                PutU32(central, (unsigned int)stored.size());
                PutU32(central, (unsigned int)raw.size());
                PutU16(central, (unsigned int)name.size());
                PutU16(central, 0);
                PutU16(central, 0);
                PutU16(central, 0);
                PutU16(central, 0);
                PutU32(central, 0);
                PutU32(central, offset);
                central += name;
            }

            unsigned int dirOffset = (unsigned int)out.size();
            out += central;
            PutU32(out, endRecordSignature);
            PutU16(out, 0);
            PutU16(out, 0);
            PutU16(out, (unsigned int)entries.Count());
            PutU16(out, (unsigned int)entries.Count());
            PutU32(out, (unsigned int)central.size());
            PutU32(out, dirOffset);
            PutU16(out, 0);
            return String(out);
        }
    }
}
