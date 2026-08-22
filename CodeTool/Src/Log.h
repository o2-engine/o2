#pragma once

void Log(const char* format, ...);

// Outs string to log if verbose mode is enabled
void VerboseLog(const char* format, ...);

void SetVerboseLog(bool verbose);
