// Stub implementations for editor-only class registrations that the auto-generated
// o2Framework.cpp references unconditionally. These classes are only defined when
// IS_EDITOR is true, but CodeTool generates registration calls for them regardless.

#include "o2/stdafx.h"

#if !IS_EDITOR

void __RegisterClass__o2__SceneEditableObject() {}
void __RegisterClass__o2__Widget__LayersEditable() {}
void __RegisterClass__o2__Widget__InternalChildrenEditableEditable() {}

#endif
