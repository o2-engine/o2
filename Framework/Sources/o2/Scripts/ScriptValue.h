#pragma once

#if IS_SCRIPTING_SUPPORTED

#include "o2/Scripts/ScriptValueDef.h"

#if defined(SCRIPTING_BACKEND_JERRYSCRIPT)
#include "o2/Utils/Editor/Attributes/ScriptableAttribute.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Scripts/JerryScript/ScriptValueImpl.h"
#include "o2/Scripts/JerryScript/ScriptValueConverters.h"
#elif defined(SCRIPTING_BACKEND_BROWSERJS)
#include "o2/Utils/Editor/Attributes/ScriptableAttribute.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Scripts/BrowserJS/ScriptValueImpl.h"
#include "o2/Scripts/BrowserJS/ScriptValueConverters.h"
#elif defined(SCRIPTING_BACKEND_QUICKJS)
#include "o2/Utils/Editor/Attributes/ScriptableAttribute.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Scripts/QuickJS/ScriptValueImpl.h"
#include "o2/Scripts/QuickJS/ScriptValueConverters.h"
#endif

#endif // IS_SCRIPTING_SUPPORTED
