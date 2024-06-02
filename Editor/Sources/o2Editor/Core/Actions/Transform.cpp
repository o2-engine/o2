#include "o2Editor/stdafx.h"
#include "Transform.h"

#include "o2/Scene/Actor.h"
#include "o2Editor/SceneWindow/SceneEditScreen.h"

namespace Editor
{
	TransformAction::TransformAction()
	{}

	TransformAction::TransformAction(const Vector<Ref<SceneEditableObject>>& actors)
	{
		objectsIds = actors.Convert<UInt64>([](auto& x) { return x->GetID(); });
		GetTransforms(objectsIds, beforeTransforms);
	}

	void TransformAction::Completed()
	{
		GetTransforms(objectsIds, doneTransforms);
	}

	String TransformAction::GetName() const
	{
		return "Actors transformation";
	}

	void TransformAction::Redo()
	{
		SetTransforms(objectsIds, doneTransforms);
	}

	void TransformAction::Undo()
	{
		SetTransforms(objectsIds, beforeTransforms);
	}

	void TransformAction::GetTransforms(const Vector<SceneUID>& objectIds, Vector<Transform>& transforms)
	{
		transforms = objectIds.Convert<Transform>([=](SceneUID id)
		{
			auto object = o2Scene.GetEditableObjectByID(id);
			if (object)
			{
				Transform res;
				res.transform = object->GetTransform();
				res.layout = object->GetLayout();
				return res;
			}

			return Transform();
		});
	}

	void TransformAction::SetTransforms(const Vector<SceneUID>& objectIds, Vector<Transform>& transforms)
	{
		for (int i = 0; i < objectsIds.Count(); i++)
		{
			auto object = o2Scene.GetEditableObjectByID(objectsIds[i]);
			if (object)
			{
				object->SetTransform(transforms[i].transform);
				object->SetLayout(transforms[i].layout);
			}
		}
	}

	bool TransformAction::Transform::operator==(const Transform& other) const
	{
		return transform == other.transform && layout == other.layout;
	}

}
// --- META ---

DECLARE_CLASS(Editor::TransformAction, Editor__TransformAction);
// --- END META ---
