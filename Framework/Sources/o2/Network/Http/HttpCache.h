#pragma once

#include "o2/Utils/Types/Containers/Map.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    // -------------------------------------------------------------------------------------------
    // In-memory HTTP response cache for GET requests. Honors Cache-Control max-age/no-store and
    // keeps ETag/Last-Modified validators for revalidation; entries are evicted oldest-used-first
    // when the size limit is exceeded. Used automatically by the HTTP client; per-request control
    // with HttpRequest::cachePolicy
    // -------------------------------------------------------------------------------------------
    class HttpCache: public RefCounterable
    {
    public:
        // Cached response entry
        struct Entry
        {
            int    status = 0;      // Response status
            Vector<String> headerLines; // Raw response header lines
            String body;            // Response body
            Int64  storedAt = 0;    // Unix time the entry was stored or last revalidated
            int    maxAge = 0;      // Freshness lifetime in seconds from Cache-Control
            String etag;            // ETag validator, empty when absent
            String lastModified;    // Last-Modified validator, empty when absent
            Int64  lastUsedAt = 0;  // Unix time of the last hit, drives eviction
        };

    public:
        // Default constructor
        explicit HttpCache(RefCounter* refCounter);

        // Sets the total body bytes limit; storing over it evicts oldest-used entries
        void SetMaxSize(int bytes);

        // Returns the total body bytes stored
        int GetUsedSize() const;

        // Returns the entries count
        int GetCount() const;

        // Removes all entries
        void Clear();

        // Returns the entry for the URL, null when absent. Marks the entry used
        Entry* Find(const String& url);

        // Returns true when the entry is still fresh by its max-age
        static bool IsFresh(const Entry& entry);

        // Stores a response for the URL when its headers allow caching; removes a previously
        // stored entry when they forbid it
        void Store(const String& url, int status, const Vector<String>& headerLines, const String& body);

        // Marks the entry revalidated: refreshes its stored time from the 304 response headers
        void Revalidate(const String& url, const Vector<String>& headerLines);

    protected:
        Map<String, Entry> mEntries;             // Entries by URL
        int                mMaxSize = 32*1024*1024; // Total body bytes limit
        int                mUsedSize = 0;        // Total body bytes stored

    protected:
        // Evicts oldest-used entries until the size fits the limit
        void EvictOverflow();
    };
}
