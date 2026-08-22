#pragma once

#include "o2/Network/Http/HttpTypes.h"
#include "o2/Utils/Threading/Atomic.h"
#include "o2/Utils/Threading/SharedRef.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    // -------------------------------------------------------------------------------------------
    // One HTTP transfer passed to a backend. The request fields are immutable after Perform; the
    // backend fills the result fields from any thread and sets done last. The client polls done
    // from the main thread pump. Backends never follow redirects — the client layer does
    // -------------------------------------------------------------------------------------------
    struct HttpTransfer: public ThreadSafeRefCounterable
    {
        String         method;      // Request method name, e.g. "GET"
        String         url;         // Full request URL
        Vector<String> headerLines; // Request headers as "Name: value" lines
        String         body;        // Request body bytes
        float          timeout = 30.0f; // Transfer timeout in seconds

        HttpError      error = HttpError::None; // Transfer error
        int            status = 0;              // Response status code
        Vector<String> responseHeaderLines;     // Response headers as "Name: value" lines
        String         responseBody;            // Response body bytes

        Atomic<int> done{ 0 }; // Set to 1 by the backend after all result fields are written
    };

    // ------------------------------------------------------------------------------
    // HTTP backend interface: performs transfers using platform networking facilities
    // ------------------------------------------------------------------------------
    class IHttpBackend
    {
    public:
        virtual ~IHttpBackend() = default;

        // Begins the transfer. The implementation fills the transfer results and sets done last
        virtual void Perform(const SharedRef<HttpTransfer>& transfer) = 0;

        // Pumps backend work on the main thread every frame
        virtual void Update(float dt) {}
    };
}
