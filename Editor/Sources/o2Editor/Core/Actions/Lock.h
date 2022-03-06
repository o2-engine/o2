#pragma once

#include "o2/Utils/Types/Containers/Vector.h"
#include "o2Editor/Core/Actions/IAction.h"

using namespace o2;

namespace o2
{
	class SceneEditableObject;
}

namespace Editor
{
	// ----------------------------------
	// Locking or unlocking object action
	// ----------------------------------
	class LockAction: public IAction
	{
	public:
		Vector<SceneUID> objectsIds;
		bool             lock;

	public:
		// Default constructor
		LockAction();
		// Constructor with list of objects
		LockAction(const Vector<SceneEditableObject*>& object, bool lock);

		// Return name of action
		String GetName() const;

		// Sets stored lock
		void Redo();

		// Sets previous lock 
		void Undo();

		SERIALIZABLE(LockAction);
	};
}

CLASS_BASES_META(Editor::LockAction)
{
	BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::LockAction)
{
	FIELD().PUBLIC().NAME(objectsIds);
	FIELD().PUBLIC().NAME(lock);
}
END_META;
CLASS_METHODS_META(Editor::LockAction)
{

	FUNCTION().PUBLIC().SIGNATURE(String, GetName);
	FUNCTION().PUBLIC().SIGNATURE(void, Redo);
	FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;
