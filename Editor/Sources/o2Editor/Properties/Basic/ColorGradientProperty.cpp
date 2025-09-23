#include "o2Editor/stdafx.h"
#include "ColorGradientProperty.h"

#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2Editor/Dialogs/CurveEditorDlg.h"
#include "o2Editor/UI/CurvePreview.h"
#include "o2Editor/Dialogs/ColorPickerDlg.h"

namespace Editor
{
	ColorGradientProperty::ColorGradientProperty(RefCounter* refCounter) :
		TPropertyField<Ref<ColorGradient>>(refCounter)
	{
	}

	ColorGradientProperty::ColorGradientProperty(RefCounter* refCounter, const ColorGradientProperty& other) :
		TPropertyField<Ref<ColorGradient>>(refCounter, other)
	{
		InitializeControls();
	}

	ColorGradientProperty& ColorGradientProperty::operator=(const ColorGradientProperty& other)
	{
		TPropertyField<Ref<ColorGradient>>::operator=(other);
		InitializeControls();
		return *this;
	}

	void ColorGradientProperty::InitializeControls()
	{
		mBox = GetChildWidget("container/layout/box");
		if (mBox)
		{
			mBox->layout->minHeight = 10;

			Color4 color1(1.0f, 1.0f, 1.0f, 1.0f), color2(0.7f, 0.7f, 0.7f, 1.0f);
			Bitmap backLayerBitmap(PixelFormat::R8G8B8A8, Vec2I(20, 20));
			backLayerBitmap.Fill(color1);
			backLayerBitmap.FillRect(0, 10, 10, 0, color2);
			backLayerBitmap.FillRect(10, 20, 20, 10, color2);
			auto backSprite = mmake<Sprite>(backLayerBitmap);
			backSprite->mode = SpriteMode::Tiled;
			mBox->AddLayer("back", backSprite, Layout::BothStretch(1, 1, 1, 1));

			mPreview = mmake<GradientPreviewDrawable>();
			mBox->AddLayer("preview", mPreview, Layout::BothStretch(1, 1, 1, 1));

			mBoxArea = mmake<CursorEventsArea>();
			mPreview->onDraw = [=]() { mBoxArea->OnDrawn(); };
			mBoxArea->isUnderPoint = [=](const Vec2F& point) { return mBox->IsUnderPoint(point); };
			mBoxArea->onDblClicked = [=](const Input::Cursor& cursor) { AddNewKey(cursor); };
			mBoxArea->onRightMouseReleased = [=](const Input::Cursor& cursor) { OpenBoxContextMenu(cursor); };
		}

		mBoxContextMenu = o2UI.CreateWidget<ContextMenu>();
		mBoxContextMenu->AddItem("Add key", [=]() { AddNewKey(mBoxPressedCursor); }, nullptr, ShortcutKeys("Double click"));
		mBox->AddChild(mBoxContextMenu);

		mHandleContextMenu = o2UI.CreateWidget<ContextMenu>();
		mHandleContextMenu->AddItem("Change color", [=]() { OpenKeyColorPick(mSelectedHandleUID); }, nullptr, ShortcutKeys("Double click"));
		mHandleContextMenu->AddItem("Delete key", [=]() { DeleteKey(mSelectedHandleUID); }, nullptr, ShortcutKeys({ VK_DELETE }));
		mBox->AddChild(mHandleContextMenu);
	}

	void ColorGradientProperty::OnValueChanged()
	{
		for (auto& ptr : mValuesProxies)
			SetProxy(ptr.first, mCommonValue);
	}

	void ColorGradientProperty::UpdateValueView()
	{
		mPreview->SetGradient(mCommonValue);
		InitializeHandles();
	}

	Ref<WidgetDragHandle> ColorGradientProperty::CreateHandle(HandleType type)
	{
		auto handle = mmake<WidgetDragHandle>(mmake<Sprite>("ui/UI4_map_key.png"),
											  mmake<Sprite>("ui/UI4_map_key_hover.png"),
											  mmake<Sprite>("ui/UI4_map_key_pressed.png"),
											  mmake<Sprite>("ui/UI4_selected_map_key.png"),
											  mmake<Sprite>("ui/UI4_selected_map_key_hover.png"),
											  mmake<Sprite>("ui/UI4_selected_map_key_pressed.png"));

		handle->cursorType = CursorType::SizeWE;
		handle->pixelPerfect = true;
		handle->SetDrawablesSizePivot(Vec2F(4, 1));

		handle->checkPositionFunc = [=](const Vec2F& pos)
			{
				if (type == HandleType::Left)
					return Vec2F(0, mBox->layout->GetHeight() * 0.5f);

				if (type == HandleType::Right)
					return Vec2F(1, mBox->layout->GetHeight() * 0.5f);

				return Vec2F(Math::Clamp01(pos.x), layout->GetHeight() * 0.5f);
			};

		handle->localToWidgetOffsetTransformFunc = [=](const Vec2F& pos)
			{
				return Vec2F(pos.x * mBox->layout->width, -2);
			};

		handle->widgetOffsetToLocalTransformFunc = [=](const Vec2F& pos)
			{
				return Vec2F(pos.x / mBox->layout->width, 0);
			};

		return handle;
	}

	void ColorGradientProperty::InitializeHandles()
	{
		// Clear old handles
		for (auto& pair : mHandles)
			mBox->RemoveChild(pair.second);

		mHandles.Clear();

		if (!mCommonValue)
			return;

		// Create new handles
		float length = mCommonValue->Length();
		int keysCount = mCommonValue->GetKeys().Count();

		for (int i = 0; i < keysCount; i++)
		{
			auto& key = mCommonValue->GetKeys()[i];
			auto handleType = i == 0 ? HandleType::Left : (i == keysCount - 1 ? HandleType::Right : HandleType::Middle);
			auto handle = CreateHandle(handleType);
			handle->SetPosition(Vec2F(key.position / length, 0));

			auto uid = key.uid;
			handle->onChangedPos = [=](const Vec2F& pos) { OnHandleChangedPos(pos, uid); };
			handle->onDblClicked = [=]() { OpenKeyColorPick(uid); };
			handle->onRightButtonReleased = [=](const Input::Cursor& cursor) { OpenKeyContextMenu(uid, cursor); };
			handle->onPressed = [=]() { mHandleContextMenu->SetItemsMaxPriority(); mSelectedHandleUID = uid; };

			mBox->AddChild(handle);
			mHandles.Add({ uid, handle });
		}
	}

	UInt64 ColorGradientProperty::FindSelectedHandle()
	{
		for (auto& pair : mHandles)
		{
			if (pair.second->IsSelected())
				return pair.first;
		}

		return 0;
	}

	void ColorGradientProperty::OnHandleChangedPos(const Vec2F& pos, UInt64 id)
	{
		if (!mCommonValue)
			return;

		int keyIdx = mCommonValue->FindKeyIdx(id);
		if (keyIdx == -1)
			return;

		auto key = mCommonValue->GetKeys()[keyIdx];
		key.position = pos.x * mCommonValue->Length();

		mCommonValue->SetKey(key, keyIdx);
	}

	void ColorGradientProperty::OpenKeyColorPick(UInt64 id)
	{
		if (!mCommonValue)
			return;

		int keyIdx = mCommonValue->FindKeyIdx(id);
		if (keyIdx == -1)
			return;

		auto key = mCommonValue->GetKeys()[keyIdx];

		auto onChanged = [=](const Color4& color, bool byUser)
			{
				auto key = mCommonValue->GetKeys()[keyIdx];
				key.color = color;
				mCommonValue->SetKey(key, keyIdx);
			};

		ColorPickerDlg::Show(key.color, onChanged);
	}

	void ColorGradientProperty::AddNewKey(const Input::Cursor& cursor)
	{
		if (!mCommonValue)
			return;

		float position = (cursor.position.x - mBox->layout->worldLeft) / mBox->layout->width * mCommonValue->Length();
		auto color = mCommonValue->Evaluate(position);

		mCommonValue->InsertKey(position, color);
		InitializeHandles();
	}

	void ColorGradientProperty::OpenBoxContextMenu(const Input::Cursor& cursor)
	{
		mBoxPressedCursor = cursor;
		mBoxContextMenu->Show(cursor.position);
	}

	void ColorGradientProperty::OpenKeyContextMenu(UInt64 id, const Input::Cursor& cursor)
	{
		mHandleContextMenu->Show(cursor.position);
		mSelectedHandleUID = id;
	}

	void ColorGradientProperty::DeleteKey(UInt64 keyUid)
	{
		if (!mCommonValue)
			return;

		auto keyIdx = mCommonValue->FindKeyIdx(keyUid);
		if (keyIdx == -1)
			return;

		// Do not allow to delete first and last keys
		if (keyIdx == 0 || keyIdx == mCommonValue->GetKeys().Count() - 1)
			return;

		mCommonValue->RemoveKeyAt(keyIdx);
		InitializeHandles();
	}

	void ColorGradientProperty::GradientPreviewDrawable::SetGradient(const Ref<ColorGradient>& gradient)
	{
		if (mGradient)
			mGradient->onKeysChanged -= THIS_FUNC(UpdateMesh);

		mGradient = gradient;

		if (mGradient)
			mGradient->onKeysChanged += THIS_FUNC(UpdateMesh);

		UpdateMesh();
	}

	void ColorGradientProperty::GradientPreviewDrawable::Draw()
	{
		if (!mEnabled)
			return;

		mMesh.Draw();
		OnDrawn();
	}

	void ColorGradientProperty::GradientPreviewDrawable::BasisChanged()
	{
		UpdateMesh();
	}

	void ColorGradientProperty::GradientPreviewDrawable::UpdateMesh()
	{
		if (!mGradient)
			return;

		if (mGradient->GetKeys().Count() == 1)
		{
			mMesh.Resize(4, 2);
			mMesh.vertexCount = 4;
			mMesh.polyCount = 2;

			auto key = mGradient->GetKeys()[0];
			auto color32 = key.color.ARGB();

			mMesh.vertices[0].Set(mTransform * Vec2F(0, 0), color32, 0.0f, 0.0f);
			mMesh.vertices[1].Set(mTransform * Vec2F(1, 0), color32, 0.0f, 0.0f);
			mMesh.vertices[2].Set(mTransform * Vec2F(1, 1), color32, 0.0f, 0.0f);
			mMesh.vertices[3].Set(mTransform * Vec2F(0, 1), color32, 0.0f, 0.0f);

			mMesh.indexes[0] = 0; mMesh.indexes[1] = 1; mMesh.indexes[2] = 2;
			mMesh.indexes[3] = 2; mMesh.indexes[4] = 3; mMesh.indexes[5] = 0;

			return;
		}

		int segments = mGradient->GetKeys().Count() - 1;
		mMesh.Resize(segments * 4, segments * 2);

		mMesh.vertexCount = segments * 4;
		mMesh.polyCount = segments * 2;

		float gradientLength = mGradient->Length();
		float prevXPos = 0.0f;
		Color4 prevColor = mGradient->GetKeys()[0].color;

		for (int i = 0; i < segments; i++)
		{
			auto key = mGradient->GetKeys()[i + 1];
			float xPos = key.position / gradientLength;

			int vertexIndex = i * 4;
			auto prevColor32 = prevColor.ABGR();
			auto color32 = key.color.ABGR();

			mMesh.vertices[vertexIndex + 0].Set(mTransform * Vec2F(prevXPos, 0), prevColor32, 0.0f, 0.0f);
			mMesh.vertices[vertexIndex + 1].Set(mTransform * Vec2F(xPos, 0), color32, 0.0f, 0.0f);
			mMesh.vertices[vertexIndex + 2].Set(mTransform * Vec2F(xPos, 1), color32, 0.0f, 0.0f);
			mMesh.vertices[vertexIndex + 3].Set(mTransform * Vec2F(prevXPos, 1), prevColor32, 0.0f, 0.0f);

			int polyIndex = i * 2 * 3;
			mMesh.indexes[polyIndex + 0] = vertexIndex + 0;
			mMesh.indexes[polyIndex + 1] = vertexIndex + 1;
			mMesh.indexes[polyIndex + 2] = vertexIndex + 2;

			mMesh.indexes[polyIndex + 3] = vertexIndex + 2;
			mMesh.indexes[polyIndex + 4] = vertexIndex + 3;
			mMesh.indexes[polyIndex + 5] = vertexIndex + 0;

			prevXPos = xPos;
			prevColor = key.color;
		}
	}

}

DECLARE_TEMPLATE_CLASS(Editor::TPropertyField<o2::Ref<o2::ColorGradient>>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::ColorGradientProperty>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::TPropertyField<o2::Ref<o2::ColorGradient>>>);
// --- META ---

ENUM_META(Editor::ColorGradientProperty::HandleType)
{
    ENUM_ENTRY(Left);
    ENUM_ENTRY(Middle);
    ENUM_ENTRY(Right);
}
END_ENUM_META;

DECLARE_CLASS(Editor::ColorGradientProperty, Editor__ColorGradientProperty);

DECLARE_CLASS(Editor::ColorGradientProperty::GradientPreviewDrawable, Editor__ColorGradientProperty__GradientPreviewDrawable);
// --- END META ---
