#pragma once

#include "EngineSettings.h"
#include "o2/Utils/Debug/Profiling/NanoProfiler.h"
#include "o2/Utils/Reflection/Reflection.h"

#define INITIALIZE_O2 \
o2::WaitForProfilerConnection(); \
o2::Reflection::InitializeTypes()
