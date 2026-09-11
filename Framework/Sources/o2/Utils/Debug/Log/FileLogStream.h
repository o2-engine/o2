#pragma once

#include "o2/Utils/Debug/Log/LogStream.h"

namespace o2
{
    // ----------------------------------------
    // File log stream, puts messages into file
    // ----------------------------------------
    class FileLogStream:public LogStream
    {
    public:
        // Constructor with file name
        FileLogStream(const String& fileName);

        // Constructor with id and file name
        FileLogStream(const WString& id, const String& fileName);

        // Destructor
        ~FileLogStream();

        // Sets the file the log goes to; a file already written stays as it is, the new one opens on the next message
        void SetFileName(const String& fileName);

        // Returns the log file name
        const String& GetFileName() const { return mFileName; }

    protected:
        String        mFileName; // Log file, opened on the first message so a renamed log never touches the default file
        std::ofstream mStream;   // Output stream

    protected:
        // Opens the file stream when it is not open yet
        void EnsureOpen();

        // Outs string into file
        void OutStrEx(const WString& str);
    };
}
