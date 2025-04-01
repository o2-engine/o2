#pragma once

#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"
#include "o2/Utils/Types/WeakRef.h"

namespace o2
{
    // ---------------------------------------------------------------------------------
    // Basic log stream. Contains interfaces of outing data, parent and children streams
    // ---------------------------------------------------------------------------------
    class LogStream: public RefCounterable
    {
    public:
        // Default constructor
        LogStream();
         
		// Default constructor
		LogStream(RefCounter* refCounter);

        // Constructor with id
        LogStream(const WString& id);

        // Destructor
        virtual ~LogStream();

        // Returns name of stream
        const WString& GetId() const;

        // Binds child stream
        void BindStream(const Ref<LogStream>& stream);

        // Unbinds child stream
        void UnbindStream(const Ref<LogStream>& stream);

        // Unbinds and destroy child stream
        void UnbindAndReleaseStream(const Ref<LogStream>& stream);

        // Unbinds and destroy all child streams
        void UnbindAllStreams();

        // Returns parent stream. Null if no parent
        const WeakRef<LogStream>& GetParentStream() const;

        // Outs with low level log
        void Out(WString format, ...);

        // Outs error message
        void Error(WString format, ...);

        // Outs warning message
        void Warning(WString format, ...);

        // Outs string to current stream and parent stream
        void OutStr(const WString& str);

        // Outs error to current stream and parent stream
        void ErrorStr(const WString& str);

        // Outs warning to current stream and parent stream
        void WarningStr(const WString& str);

    protected:
        WString mId; // Name of log stream

        WeakRef<LogStream>     mParentStream; // Parent stream
        Vector<Ref<LogStream>> mChildStreams; // Child streams

    protected:
        // Outs string to stream
        virtual void OutStrEx(const WString& str) {}

        // Outs error to stream
        virtual void OutErrorEx(const WString& str);

        // Outs warning to stream
        virtual void OutWarningEx(const WString& str);
    };
}
