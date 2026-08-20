#include "Log.h"

#include <cstdarg>
#include <cstdio>

static bool gVerbose = false;

void Log(const char* format, ...)
{
    va_list vlist;
    va_start(vlist, format);
    vprintf(format, vlist);
    va_end(vlist);
}

void VerboseLog(const char* format, ...)
{
    if (!gVerbose)
        return;

    va_list vlist;
    va_start(vlist, format);
    vprintf(format, vlist);
    va_end(vlist);
}

void SetVerboseLog(bool verbose)
{
    gVerbose = verbose;
}
