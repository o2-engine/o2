#include "o2Editor/stdafx.h"
#include "ColorGradientProperty.h"

#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2Editor/Core/Dialogs/CurveEditorDlg.h"
#include "o2Editor/Core/UI/CurvePreview.h"

namespace Editor
{
	ColorGradientProperty::ColorGradientProperty(RefCounter* refCounter):
		TPropertyField<Ref<ColorGradient>>(refCounter)
	{}

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
			mBox->AddLayer("preview", mPreview, Layout::BothStretch(1, 3, 1, 3));
 		}
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

	Ref<WidgetDragHandle> ColorGradientProperty::CreateHandle()
	{
		auto handle = mmake<WidgetDragHandle>(mmake<Sprite>("ui/UI4_key.png"),
			mmake<Sprite>("ui/UI4_key_hover.png"),
			mmake<Sprite>("ui/UI4_key_pressed.png"),
			mmake<Sprite>("ui/UI4_selected_key.png"),
			mmake<Sprite>("ui/UI4_selected_key_hover.png"),
			mmake<Sprite>("ui/UI4_selected_key_pressed.png"));

		handle->cursorType = CursorType::SizeWE;
		handle->pixelPerfect = true;
		handle->SetDrawablesSizePivot(Vec2F(7, 1));

		handle->checkPositionFunc = [&](const Vec2F& pos)
			{
				float position = pos.x;
				if (position < 0.0f)
					position = 0.0f;

				return Vec2F(position, layout->GetHeight() * 0.5f);
			};

		handle->localToWidgetOffsetTransformFunc = [&](const Vec2F& pos)
			{
				return Vec2F(pos.x * mBox->layout->width, 0);
			};

		handle->widgetOffsetToLocalTransformFunc = [&](const Vec2F& pos)
			{
				return Vec2F(pos.x / mBox->layout->width, 0);
			};

		return handle;
	}

	void ColorGradientProperty::InitializeHandles()
	{
		// Clear old handles
		for (auto& handle : mHandles)
			mBox->RemoveChild(handle);

		mHandles.Clear();

		if (!mCommonValue)
			return;

		// Create new handles
		for (auto& key : mCommonValue->GetKeys())
		{
			auto handle = CreateHandle();
			handle->SetPosition(Vec2F(key.position, 0));
			mBox->AddChild(handle);
			mHandles.Add(handle);
		}
	}

	void ColorGradientProperty::GradientPreviewDrawable::SetGradient(const Ref<ColorGradient>& gradient)
	{
		mGradient = gradient;
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
			auto prevColor32 = prevColor.ARGB();
			auto color32 = key.color.ARGB();

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

DECLARE_CLASS(Editor::ColorGradientProperty, Editor__ColorGradientProperty);
// --- END META ---
