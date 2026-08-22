#include "o2/stdafx.h"
#include "HttpCache.h"

#include <time.h>

namespace o2
{
    // Returns the value of the header from raw "Name: value" lines by case-insensitive name
    static String FindHeaderLine(const Vector<String>& headerLines, const char* name)
    {
        size_t nameLength = strlen(name);
        for (auto& line : headerLines)
        {
            if (line.size() <= nameLength + 1 || line[nameLength] != ':')
                continue;

            bool equal = true;
            for (size_t i = 0; i < nameLength && equal; i++)
            {
                char a = line[i], b = name[i];
                if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                equal = a == b;
            }

            if (!equal)
                continue;

            size_t valueBegin = line.find_first_not_of(" \t", nameLength + 1);
            return valueBegin == std::string::npos ? String() : (String)line.substr(valueBegin);
        }

        return String();
    }

    // Parses Cache-Control: returns max-age seconds, -1 when absent; sets noStore
    static int ParseCacheControl(const String& cacheControl, bool& noStore)
    {
        noStore = false;
        int maxAge = -1;

        size_t position = 0;
        while (position <= cacheControl.size())
        {
            size_t partEnd = cacheControl.find(',', position);
            if (partEnd == std::string::npos)
                partEnd = cacheControl.size();

            String part = cacheControl.substr(position, partEnd - position);
            position = partEnd + 1;

            size_t begin = part.find_first_not_of(" \t");
            if (begin == std::string::npos)
                continue;

            String trimmed = part.substr(begin);
            String lower = trimmed;
            for (size_t i = 0; i < lower.size(); i++)
            {
                if (lower[i] >= 'A' && lower[i] <= 'Z')
                    lower[i] = (char)(lower[i] - 'A' + 'a');
            }

            if (lower == "no-store" || lower == "no-cache")
                noStore = true;
            else if (lower.compare(0, 8, "max-age=") == 0)
                maxAge = atoi(lower.Data() + 8);
        }

        return maxAge;
    }

    HttpCache::HttpCache(RefCounter* refCounter):
        RefCounterable(refCounter)
    {}

    void HttpCache::SetMaxSize(int bytes)
    {
        mMaxSize = bytes;
        EvictOverflow();
    }

    int HttpCache::GetUsedSize() const
    {
        return mUsedSize;
    }

    int HttpCache::GetCount() const
    {
        return (int)mEntries.size();
    }

    void HttpCache::Clear()
    {
        mEntries.Clear();
        mUsedSize = 0;
    }

    HttpCache::Entry* HttpCache::Find(const String& url)
    {
        auto found = mEntries.find(url);
        if (found == mEntries.end())
            return nullptr;

        found->second.lastUsedAt = (Int64)time(nullptr);
        return &found->second;
    }

    bool HttpCache::IsFresh(const Entry& entry)
    {
        return (Int64)time(nullptr) < entry.storedAt + entry.maxAge;
    }

    void HttpCache::Store(const String& url, int status, const Vector<String>& headerLines, const String& body)
    {
        if (status != 200)
            return;

        bool noStore = false;
        int maxAge = ParseCacheControl(FindHeaderLine(headerLines, "Cache-Control"), noStore);

        String etag = FindHeaderLine(headerLines, "ETag");
        String lastModified = FindHeaderLine(headerLines, "Last-Modified");

        auto previous = mEntries.find(url);
        if (previous != mEntries.end())
        {
            mUsedSize -= (int)previous->second.body.size();
            mEntries.erase(previous);
        }

        // Cache only what can be served fresh or revalidated
        if (noStore || (maxAge <= 0 && etag.IsEmpty() && lastModified.IsEmpty()))
            return;

        Entry entry;
        entry.status = status;
        entry.headerLines = headerLines;
        entry.body = body;
        entry.storedAt = (Int64)time(nullptr);
        entry.maxAge = maxAge > 0 ? maxAge : 0;
        entry.etag = etag;
        entry.lastModified = lastModified;
        entry.lastUsedAt = entry.storedAt;

        mEntries[url] = entry;
        mUsedSize += (int)body.size();

        EvictOverflow();
    }

    void HttpCache::Revalidate(const String& url, const Vector<String>& headerLines)
    {
        auto found = mEntries.find(url);
        if (found == mEntries.end())
            return;

        bool noStore = false;
        int maxAge = ParseCacheControl(FindHeaderLine(headerLines, "Cache-Control"), noStore);
        if (maxAge >= 0)
            found->second.maxAge = maxAge;

        found->second.storedAt = (Int64)time(nullptr);
        found->second.lastUsedAt = found->second.storedAt;
    }

    void HttpCache::EvictOverflow()
    {
        while (mUsedSize > mMaxSize && !mEntries.empty())
        {
            auto oldest = mEntries.begin();
            for (auto it = mEntries.begin(); it != mEntries.end(); ++it)
            {
                if (it->second.lastUsedAt < oldest->second.lastUsedAt)
                    oldest = it;
            }

            mUsedSize -= (int)oldest->second.body.size();
            mEntries.erase(oldest);
        }
    }
}
