#include "o2Editor/stdafx.h"
#include "CurvesEditor.h"

#include "o2/Application/Application.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/HorizontalScrollBar.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Scene/UI/Widgets/VerticalScrollBar.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/Math/Interpolation.h"
#include "o2/Utils/System/Clipboard.h"
#include "o2Editor/Dialogs/KeyEditDlg.h"
#include "o2Editor/UI/CurveEditor/CurveActions.h"
#include "o2Editor/UIRoot.h"

namespace Editor
{
    CurvesEditor::CurvesEditor(RefCounter* refCounter):
        FrameScrollView(refCounter), SelectableDragHandlesGroup(refCounter)
    {
        mReady = false;

        mSelectionSprite = mmake<Sprite>();
        mTextFont = AssetRef<VectorFontAsset>("stdFont.ttf")->GetFont();
        mTextFont->CheckCharacters("0123456789.,+-", 10);

        mCurvesMesh = mmake<Mesh>();
        mCurvesMesh->Resize(4, 2);

        InitializeTextDrawables();
        InitializeContextMenu();

        mTransformFrame = mmake<FrameHandles>();
        mTransformFrame->SetPivotEnabled(false);
        mTransformFrame->SetRotationEnabled(false);
        mTransformFrame->onTransformed = THIS_FUNC(OnTransformFrameTransformed);
        mTransformFrame->onPressed = THIS_FUNC(OnTransformBegin);
        mTransformFrame->onChangeCompleted = THIS_FUNC(OnTransformCompleted);
        mTransformFrame->messageFallDownListener = this;

        mBackColor = Color4(225, 232, 232, 255);

        mHandleSamplesStubInfo = mmake<CurveInfo>();
        mHandleSamplesStubInfo->editor = Ref(this);

        mReady = true;
    }

    CurvesEditor::CurvesEditor(RefCounter* refCounter, const CurvesEditor& other):
        FrameScrollView(refCounter, other), SelectableDragHandlesGroup(refCounter), mSelectionSprite(other.mSelectionSprite->CloneAs<Sprite>()), mTextFont(other.mTextFont)
    {
        mReady = false;

        mHandleSamplesStubInfo = mmake<CurveInfo>();
        mHandleSamplesStubInfo->editor = Ref(this);

        mContextMenu = FindChildByType<ContextMenu>();

        InitializeTextDrawables();
        InitializeContextMenu();
        RetargetStatesAnimations();

        mReady = true;
    }

    CurvesEditor::~CurvesEditor()
    {}

    Editor::CurvesEditor& CurvesEditor::operator=(const CurvesEditor& other)
    {
        FrameScrollView::operator=(other);

        mReady = false;

        mSelectionSprite = other.mSelectionSprite->CloneAsRef<Sprite>();
        mTextFont = other.mTextFont;

        mMainHandleSample = other.mMainHandleSample;
        mSupportHandleSample = other.mSupportHandleSample;
        mRangeHandleSample = other.mRangeHandleSample;

        InitializeTextDrawables();
        RetargetStatesAnimations();

        mReady = true;

        return *this;
    }

    void CurvesEditor::Draw()
    {
        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        FrameScrollView::Draw();

        o2Render.EnableScissorTest(layout->GetWorldRect());

        DrawTransformFrame();
        DrawHandles();
        DrawSelection();

        o2Render.DisableScissorTest();
    }

    void CurvesEditor::Update(float dt)
    {
        FrameScrollView::Update(dt);

        if (mReady && mResEnabledInHierarchy && !mIsClipped && mNeedAdjustView)
        {
            mNeedAdjustView = false;
            mViewCameraTargetScale = mAvailableArea.Size()/mViewCamera.GetSize();
            mViewCamera.center = mAvailableArea.Center();
            mViewCameraTargetPos = mViewCamera.position;
        }
    }

    Map<String, Ref<Curve>> CurvesEditor::GetCurves() const
    {
        Map<String, Ref<Curve>> res;
        for (auto& curveInfo : mCurves)
            res.Add(curveInfo->curveId, curveInfo->curve);

        return res;
    }

    void CurvesEditor::AddCurve(const String& id, const Ref<Curve>& curve, const Color4& color /*= Color4(44, 62, 80)*/)
    {
        UpdateSelfTransform();

        auto info = mmake<CurveInfo>();
        info->editor = Ref(this);
        info->curveId = id;
        info->curve = curve;

        info->AdjustScale();

        info->curve->onKeysChanged += MakeSubscription(info.Get(), &CurveInfo::OnCurveChanged, [=]() { info->curve = nullptr; });

        if (color == Color4(44, 62, 80) && !mCurves.IsEmpty())
            info->color = Color4::SomeColor(mCurves.Count());
        else
            info->color = color;

        for (int i = 0; i < curve->GetKeys().Count(); i++)
            AddCurveKeyHandles(info, i);

        mCurves.Add(info);
        RecalculateViewArea();

        if (mCurves.Count() == 1)
            mNeedAdjustView = true;
    }

    void CurvesEditor::RemoveCurve(const Ref<Curve>& curve)
    {
        for (auto& info : mCurves)
        {
            if (info->curve == curve)
            {
                for (auto& handle : info->handles)
                {
                    handle->mainHandle->SetSelectionGroup(nullptr);

                    handle->leftSupportHandle->SetSelectionGroup(nullptr);
                    mSupportHandles.Remove(handle->leftSupportHandle);

                    handle->rightSupportHandle->SetSelectionGroup(nullptr);
                    mSupportHandles.Remove(handle->rightSupportHandle);
                }

                mCurves.Remove(info);
                break;
            }
        }
    }

    void CurvesEditor::RemoveCurve(const String& id)
    {
        auto curve = FindCurve(id);
        if (curve)
            RemoveCurve(curve);
    }

    void CurvesEditor::RemoveAllCurves()
    {
        auto curves = mCurves;
        for (auto& info : curves)
            RemoveCurve(info->curve);
    }

    void CurvesEditor::SetCurveColor(const Ref<Curve>& curve, const Color4& color)
    {
        for (auto& info : mCurves)
        {
            if (info->curve == curve)
            {
                info->color = color;

                for (auto& handle : info->handles)
                    handle->mainHandle->SetDrawablesColor(color);

                break;
            }
        }
    }

    void CurvesEditor::UpdateSelfTransform()
    {
        FrameScrollView::UpdateSelfTransform();

        UpdateLocalScreenTransforms();
        OnCameraTransformChanged();
    }

    const Ref<ContextMenu>& CurvesEditor::GetContextMenu() const
    {
        return mContextMenu;
    }

    Ref<RefCounterable> CurvesEditor::CastToRefCounterable(const Ref<CurvesEditor>& ref)
    {
        return DynamicCast<FrameScrollView>(ref);
    }

    void CurvesEditor::SetSelectionSpriteImage(const AssetRef<ImageAsset>& image)
    {
        mSelectionSprite->LoadFromImage(image);
    }

    void CurvesEditor::SetTextFont(const Ref<Font>& font)
    {
        mTextFont = font;
        mTextLeft->SetFont(mTextFont);
        mTextRight->SetFont(mTextFont);
        mTextTop->SetFont(mTextFont);
        mTextBottom->SetFont(mTextFont);
    }

    void CurvesEditor::SetTextBorder(const BorderF& border)
    {
        mTextBorder = border;
    }

    void CurvesEditor::SetMainHandleImages(const AssetRef<ImageAsset>& regular, const AssetRef<ImageAsset>& hover,
                                           const AssetRef<ImageAsset>& pressed, const AssetRef<ImageAsset>& selected)
    {
        mMainHandleSample.curveInfo = mHandleSamplesStubInfo;
        mMainHandleSample = CurveHandle(mmake<Sprite>(regular), mmake<Sprite>(hover),
                                        mmake<Sprite>(pressed), mmake<Sprite>(selected));
    }

    void CurvesEditor::SetSupportHandleImages(const AssetRef<ImageAsset>& regular, const AssetRef<ImageAsset>& hover, 
                                              const AssetRef<ImageAsset>& pressed, const AssetRef<ImageAsset>& selected)
    {
        mSupportHandleSample.curveInfo = mHandleSamplesStubInfo;
        mSupportHandleSample = CurveHandle(mmake<Sprite>(regular), mmake<Sprite>(hover),
                                           mmake<Sprite>(pressed), mmake<Sprite>(selected));
    }

    void CurvesEditor::SetRangeHandleImages(const AssetRef<ImageAsset>& regular, const AssetRef<ImageAsset>& hover, 
                                            const AssetRef<ImageAsset>& pressed, const AssetRef<ImageAsset>& selected)
    {
        mRangeHandleSample.curveInfo = mHandleSamplesStubInfo;
        mRangeHandleSample = CurveHandle(mmake<Sprite>(regular), mmake<Sprite>(hover),
                                         mmake<Sprite>(pressed), mmake<Sprite>(selected));
    }

    void CurvesEditor::SetAdjustCurvesScale(bool enable)
    {
        mAdjustCurvesScale = enable;

        for (auto& curve : mCurves) {
            curve->AdjustScale();
        }

        RecalculateViewArea();

        mNeedRedraw = true;
        mNeedAdjustView = true;
    }

	void CurvesEditor::OnFocusedByRoot()
	{
		mContextMenu->SetItemsMaxPriority();
	}

	void CurvesEditor::OnUnfocusedByRoot()
	{
        mContextMenu->SetItemsMinPriority();
	}

	void CurvesEditor::OnScrolled(float scroll)
    {
        Vec2F newScale = mViewCameraTargetScale;

        if (o2Input.IsKeyDown(VK_CONTROL))
            newScale.x *= 1.0f - (scroll*mViewCameraScaleSence);
        else if (o2Input.IsKeyDown(VK_SHIFT))
            newScale.y *= 1.0f - (scroll*mViewCameraScaleSence);
        else
            newScale *= 1.0f - (scroll*mViewCameraScaleSence);

        ChangeCameraScaleRelativeToCursor(newScale);
    }

    Ref<Curve> CurvesEditor::FindCurve(const String& id)
    {
        for (auto& curve : mCurves)
        {
            if (curve->curveId == id)
                return curve->curve;
        }

        return nullptr;
    }

    void CurvesEditor::InitializeContextMenu()
    {
        mContextMenu = o2UI.CreateWidget<ContextMenu>();

        mContextMenu->AddItems( {
            mmake<ContextMenu::Item>("Auto smooth", false, [&](bool) { SetHandlesTypesChecked(Curve::Key::Type::Smooth); }),
            mmake<ContextMenu::Item>("Flat", false, [&](bool) { SetHandlesTypesChecked(Curve::Key::Type::Flat); }),
            mmake<ContextMenu::Item>("Free", false, [&](bool) { SetHandlesTypesChecked(Curve::Key::Type::Free); }),
            mmake<ContextMenu::Item>("Linear", false, [&](bool) { SetHandlesTypesChecked(Curve::Key::Type::Linear); }),
            mmake<ContextMenu::Item>("Broken", false, [&](bool) { SetHandlesTypesChecked(Curve::Key::Type::Broken); }),
            mmake<ContextMenu::Item>("Discrete", false, [&](bool) { SetHandlesTypesChecked(Curve::Key::Type::Discrete); }),

            ContextMenu::Item::Separator(),

            mmake<ContextMenu::Item>("Edit", THIS_FUNC(OnEditPressed)),

            ContextMenu::Item::Separator(),

            mmake<ContextMenu::Item>("Copy keys", THIS_FUNC(OnCopyPressed), "", AssetRef<ImageAsset>(), ShortcutKeys('C', true)),
            mmake<ContextMenu::Item>("Cut keys", THIS_FUNC(OnCutPressed), "", AssetRef<ImageAsset>(), ShortcutKeys('X', true)),
            mmake<ContextMenu::Item>("Paste keys", THIS_FUNC(OnPastePressed), "", AssetRef<ImageAsset>(), ShortcutKeys('V', true)),
            mmake<ContextMenu::Item>("Delete keys", THIS_FUNC(OnDeletePressed), "", AssetRef<ImageAsset>(), ShortcutKeys(VK_DELETE)),
            mmake<ContextMenu::Item>("Insert key", THIS_FUNC(OnInsertPressed)),

            ContextMenu::Item::Separator(),

            mmake<ContextMenu::Item>("Undo", THIS_FUNC(OnUndoPressed), "", AssetRef<ImageAsset>(), ShortcutKeys('Z', true)),
            mmake<ContextMenu::Item>("Redo", THIS_FUNC(OnRedoPressed), "", AssetRef<ImageAsset>(), ShortcutKeys('Z', true, true))
            }
        );

        onShow = [&]() { mContextMenu->SetItemsMaxPriority(); };
        onHide = [&]() { mContextMenu->SetItemsMinPriority(); };

        AddChild(mContextMenu);
    }

    void CurvesEditor::InitializeTextDrawables()
    {
        mTextLeft = mmake<Text>(mTextFont);
        mTextLeft->SetHeight(10);
        mTextLeft->SetHorAlign(HorAlign::Left);
        mTextLeft->SetVerAlign(VerAlign::Bottom);
        mTextLeft->color = Color4(96, 125, 139);

        mTextRight = mmake<Text>(mTextFont);
        mTextRight->SetHeight(10);
        mTextRight->SetHorAlign(HorAlign::Right);
        mTextRight->SetVerAlign(VerAlign::Bottom);
        mTextRight->color = Color4(96, 125, 139);


        mTextTop = mmake<Text>(mTextFont);
        mTextTop->SetHeight(10);
        mTextTop->SetHorAlign(HorAlign::Left);
        mTextTop->SetVerAlign(VerAlign::Bottom);
        mTextTop->SetAngleDegrees(-90.0f);
        mTextTop->color = Color4(96, 125, 139);


        mTextBottom = mmake<Text>(mTextFont);
        mTextBottom->SetHeight(10);
        mTextBottom->SetHorAlign(HorAlign::Right);
        mTextBottom->SetVerAlign(VerAlign::Bottom);
        mTextBottom->SetAngleDegrees(-90.0f);
        mTextBottom->color = Color4(96, 125, 139);
    }

    void CurvesEditor::RecalculateViewArea()
    {
        if (mCurves.IsEmpty())
        {
            mAvailableArea = RectF(0, 0, 1, 1);
            return;
        }

        Vec2F firstPoint;
        if (!mCurves.Last()->approximatedPoints.IsEmpty())
            firstPoint = mCurves.Last()->approximatedPoints.Last().first;

        mAvailableArea = RectF(firstPoint, firstPoint);

        for (auto& curve : mCurves)
        {
            for (auto& p : curve->approximatedPoints)
            {
                mAvailableArea.left = Math::Min(mAvailableArea.left, p.first.x);
                mAvailableArea.right = Math::Max(mAvailableArea.right, p.first.x);
                mAvailableArea.top = Math::Max(mAvailableArea.top, p.first.y);
                mAvailableArea.bottom = Math::Min(mAvailableArea.bottom, p.first.y);

                mAvailableArea.left = Math::Min(mAvailableArea.left, p.second.x);
                mAvailableArea.right = Math::Max(mAvailableArea.right, p.second.x);
                mAvailableArea.top = Math::Max(mAvailableArea.top, p.second.y);
                mAvailableArea.bottom = Math::Min(mAvailableArea.bottom, p.second.y);
            }
        }

        float bordersCoef = 1.5f;
        Vec2F size = mAvailableArea.Size();
        mAvailableArea.left -= size.x*bordersCoef;
        mAvailableArea.right += size.x*bordersCoef;
        mAvailableArea.top += size.y*bordersCoef;
        mAvailableArea.bottom -= size.y*bordersCoef;

        mHorScrollbar->SetValueRange(mAvailableArea.left, mAvailableArea.right);
        mVerScrollbar->SetValueRange(mAvailableArea.bottom, mAvailableArea.top);
    }

    void CurvesEditor::RedrawContent()
    {
        DrawGrid();
        DrawCurves();
    }

    void CurvesEditor::DrawGrid()
    {
        // Screen -> Local -> Curve View

        Vec2F gridScale(1, 1);
        Vec2F gridOffset(0, 0);

        bool allSameCurve = !mSelectedHandles.IsEmpty() && mSelectedHandles.All([&](const Ref<DragHandle>& x) {
            return DynamicCast<CurveHandle>(x)->curveInfo == DynamicCast<CurveHandle>(mSelectedHandles[0])->curveInfo;
        });

        if (allSameCurve)
        {
            auto curvehandle = DynamicCast<CurveHandle>(mSelectedHandles[0]);
            gridScale = curvehandle->curveInfo->viewScale;
            gridOffset = curvehandle->curveInfo->viewOffset;
        }

        bool unknownScale = mAdjustCurvesScale && !allSameCurve;

        if (unknownScale && mCurves.Count() == 1) {
            gridScale = mCurves[0]->viewScale;
            gridOffset = mCurves[0]->viewOffset;
            unknownScale = false;
        }

        RectF localCameraRect = mViewCamera.GetRect();
        RectF curveViewCameraRect(LocalToCurveView(localCameraRect.LeftTop(), gridScale, gridOffset),
                                  LocalToCurveView(localCameraRect.RightBottom(), gridScale, gridOffset));

        Camera screenCamera;
        RectF screenCameraRect = screenCamera.GetRect();

        auto camTransformBasis = mViewCamera.GetBasis().Inverted()*screenCamera.GetBasis();
        auto localToScreen = [&](const Vec2F& point) { return point*camTransformBasis; };

        o2Render.SetCamera(screenCamera);

        float minCellSize = 0.000001f;
        float maxCellSize = 1000000.0f;
        Vec2F curveViewCellSize(minCellSize, minCellSize);
        Vec2F optimalCurveViewCellSize = curveViewCameraRect.Size()/2.0f;

        while (curveViewCellSize.x < maxCellSize)
        {
            float next = curveViewCellSize.x*10.0f;
            if (optimalCurveViewCellSize.x > curveViewCellSize.x && optimalCurveViewCellSize.x <= next)
                break;

            curveViewCellSize.x = next;
        }

        while (curveViewCellSize.y < maxCellSize)
        {
            float next = curveViewCellSize.y*10.0f;
            if (optimalCurveViewCellSize.y > curveViewCellSize.y && optimalCurveViewCellSize.y <= next)
                break;

            curveViewCellSize.y = next;
        }

        Vec2F localCellSize = CurveViewToLocal(curveViewCellSize, gridScale, gridOffset) -
            CurveViewToLocal(Vec2F(), gridScale, gridOffset);

        Vec2F screenCellSize = localToScreen(localCellSize) - localToScreen(Vec2F());

        Vec2F gridCurveViewOrigin(Math::Round(curveViewCameraRect.Center().x/curveViewCellSize.x)*curveViewCellSize.x,
                                  Math::Round(curveViewCameraRect.Center().y/curveViewCellSize.y)*curveViewCellSize.y);

        Vec2F gridLocalOrigin = CurveViewToLocal(gridCurveViewOrigin, gridScale, gridOffset);
        Vec2F gridScreenOrigin = localToScreen(gridLocalOrigin);

        Vec2I cellsCount(Math::CeilToInt(curveViewCameraRect.Width()/curveViewCellSize.x),
                         Math::CeilToInt(curveViewCameraRect.Height()/curveViewCellSize.y));

        Color4 cellColorSmoothed = Math::Lerp(mGridColor, mBackColor, 0.7f);

        Vec2F tenCurveViewCellSize = curveViewCellSize*10.0f;

        if (o2Input.IsKeyDown(VK_F1))
        {
            o2Render.DrawCross(gridScreenOrigin, 5, Color4::Red());
            mTextLeft->SetScale(Vec2F(1, 1));
            mTextLeft->SetText(String(gridCurveViewOrigin) + "\n" + String(curveViewCellSize));
            mTextLeft->SetPosition(gridScreenOrigin);
            mTextLeft->Draw();

            mTextTop->SetScale(Vec2F(1, 1));
            mTextTop->SetText(String(curveViewCameraRect.top));
            mTextTop->SetPosition(Vec2F(0, screenCameraRect.top));
            mTextTop->Draw();

            mTextBottom->SetScale(Vec2F(1, 1));
            mTextBottom->SetText(String(curveViewCameraRect.bottom));
            mTextBottom->SetPosition(Vec2F(0, screenCameraRect.bottom));
            mTextBottom->Draw();

            mTextLeft->SetScale(Vec2F(1, 1));
            mTextLeft->SetText(String(curveViewCameraRect.left));
            mTextLeft->SetPosition(Vec2F(screenCameraRect.left, 0));
            mTextLeft->Draw();

            mTextRight->SetScale(Vec2F(1, 1));
            mTextRight->SetText(String(curveViewCameraRect.right));
            mTextRight->SetPosition(Vec2F(screenCameraRect.right, 0));
            mTextRight->Draw();
        }

        // Y
        if (horGridEnabled)
        {
            for (int i = -cellsCount.y/2; i < cellsCount.y/2; i++)
            {
                float d = (float)i*screenCellSize.y;
                float f = (gridCurveViewOrigin.y + (float)i*curveViewCellSize.y)/tenCurveViewCellSize.y;
                float rdy = Math::Abs(f - Math::Floor(f));
                bool yTen = rdy < 0.05f || rdy > 0.95f;

                Vec2F xBegin = Vec2F(screenCameraRect.left, Math::Round(d + gridScreenOrigin.y));
                Vec2F xEnd = Vec2F(screenCameraRect.right, Math::Round(d + gridScreenOrigin.y));

                o2Render.DrawAALine(xBegin, xEnd, yTen ? mGridColor : cellColorSmoothed);
            }
        }

        // X
        if (verGridEnabled)
        {
            for (int i = -cellsCount.x/2; i < cellsCount.x/2; i++)
            {
                float d = (float)i*screenCellSize.x;

                float f = (gridCurveViewOrigin.x + (float)i*curveViewCellSize.x)/tenCurveViewCellSize.x;
                float rdx = Math::Abs(f - Math::Floor(f));
                bool xTen = rdx < 0.05f || rdx > 0.95f;

                Vec2F yBegin = Vec2F(Math::Round(d + gridScreenOrigin.x), screenCameraRect.bottom);
                Vec2F yEnd = Vec2F(Math::Round(d + gridScreenOrigin.x), screenCameraRect.top);

                o2Render.DrawAALine(yBegin, yEnd, xTen ? mGridColor : cellColorSmoothed);
            }
        }

        if (unknownScale)
            return;

        char buf[255];

        // Y
        if (horGridEnabled)
        {
            for (int i = -cellsCount.y/2; i < cellsCount.y/2; i++)
            {
                float y = (float)i*screenCellSize.y + gridScreenOrigin.y;
                float yc = (float)i*curveViewCellSize.y + gridCurveViewOrigin.y;

                sprintf(buf, "%.1f", (Math::Round(yc*10.0f)/10.0f));
                String yCaption = buf;

                if (horGridEnabled)
                {
                    mTextLeft->SetText(yCaption);
                    mTextLeft->SetPosition(Vec2F(screenCameraRect.left + mTextBorder.left, y));
                    mTextLeft->Draw();

                    mTextRight->SetText(yCaption);
                    mTextRight->SetPosition(Vec2F(screenCameraRect.right - mTextBorder.right, y));
                    mTextRight->Draw();
                }
            }
        }

        // X
        if (verGridEnabled)
        {
            for (int i = -cellsCount.x/2; i < cellsCount.x/2; i++)
            {
                float x = (float)i*screenCellSize.x + gridScreenOrigin.x;
                float xc = (float)i*curveViewCellSize.x + gridCurveViewOrigin.x;

                sprintf(buf, "%.1f", (Math::Round(xc*10.0f)/10.0f));
                String xCaption = buf;

                if (verGridEnabled)
                {
                    mTextTop->SetText(xCaption);
                    mTextTop->SetPosition(Vec2F(x, screenCameraRect.top - mTextBorder.top));
                    mTextTop->Draw();

                    mTextBottom->SetText(xCaption);
                    mTextBottom->SetPosition(Vec2F(x, screenCameraRect.bottom + mTextBorder.bottom));
                    mTextBottom->Draw();
                }
            }
        }

        o2Render.SetCamera(mViewCamera);
    }

    void CurvesEditor::DrawCurves()
    {
        o2Render.camera = Camera();

        Basis transform = mViewCamera.GetBasis().Inverted()*Camera().GetBasis();

        for (auto& curve : mCurves)
        {
            if (curve->approximatedPoints.IsEmpty())
                continue;

            //o2Render.DrawLine(curve->approximatedPoints, curve->color);

            float cameraLeftPos = mViewCamera.GetRect().left;
            float cameraRightPos = mViewCamera.GetRect().right;
            for (int i = 1; i < curve->approximatedPoints.Count(); i++)
            {
                auto& left = curve->approximatedPoints[i - 1];
                auto& right = curve->approximatedPoints[i];

                if (right.first.x < cameraLeftPos)
                    continue;

                if (left.first.x > cameraRightPos)
                    break;

                Vec2F leftTop = left.first*transform;
                Vec2F leftBottom = left.second*transform;
                Vec2F rightTop = right.first*transform;
                Vec2F rightBottom = right.second*transform;

                Color4 meshColor = curve->color;
                meshColor.a /= 2;

                mCurvesMesh->vertices[0] = Vertex(leftTop, meshColor.ABGR(), 0, 0);
                mCurvesMesh->vertices[1] = Vertex(leftBottom, meshColor.ABGR(), 0, 0);
                mCurvesMesh->vertices[2] = Vertex(rightTop, meshColor.ABGR(), 0, 0);
                mCurvesMesh->vertices[3] = Vertex(rightBottom, meshColor.ABGR(), 0, 0);

                mCurvesMesh->indexes[0] = 0; mCurvesMesh->indexes[1] = 1; mCurvesMesh->indexes[2] = 2;
                mCurvesMesh->indexes[3] = 2; mCurvesMesh->indexes[4] = 1; mCurvesMesh->indexes[5] = 3;

                mCurvesMesh->vertexCount = 4;
                mCurvesMesh->polyCount = 2;

                mCurvesMesh->Draw();

                o2Render.DrawAALine(leftTop, rightTop, curve->color);
                o2Render.DrawAALine(leftBottom, rightBottom, curve->color);
            }
        }

        o2Render.camera = mViewCamera;
    }

    void CurvesEditor::DrawHandles()
    {
        for (auto& curve : mCurves)
        {
            for (auto& keyHandles : curve->handles)
                keyHandles->Draw(layout->GetWorldRect());
        }
    }

    void CurvesEditor::DrawSelection()
    {
        if (mIsPressed)
        {
            mSelectionSprite->rect = RectF(LocalToScreenPoint(mSelectingPressedPoint), o2Input.cursorPos);
            mSelectionSprite->Draw();
        }
    }

    void CurvesEditor::DrawTransformFrame()
    {
        if (!mTransformFrameVisible)
            return;

        Vec2F borders(10, 10);

        mTransformFrame->SetBasis(Basis(LocalToScreenPoint(mTransformFrameBasis.origin) - borders,
                                 mTransformFrameBasis.xv/mViewCamera.GetScale() + Vec2F(borders.x*2.0f, 0),
                                 mTransformFrameBasis.yv/mViewCamera.GetScale() + Vec2F(0, borders.y*2.0f)));

        mTransformFrame->Draw();

        if (o2Input.IsKeyDown(VK_CONTROL))
        {
            Vec2F left = mTransformFrame->GetCurrentBasis().origin;
            Vec2F right = mTransformFrame->GetCurrentBasis().origin + mTransformFrame->GetCurrentBasis().xv;
            RectF rect = layout->worldRect;

            o2Render.DrawAALine(Vec2F(right.x, rect.bottom), Vec2F(right.x, rect.top), mTransformFrame->GetFrameColor());
            o2Render.DrawAALine(Vec2F(left.x, rect.bottom), Vec2F(left.x, rect.top), mTransformFrame->GetFrameColor());
        }
    }

#undef DrawText

    void CurvesEditor::AddCurveKeyHandles(const Ref<CurveInfo>& info, int keyId)
    {
        PushEditorScopeOnStack scope;

        auto keyHandles = mmake<KeyHandles>(mMainHandleSample, mSupportHandleSample, mRangeHandleSample, Ref(this), info->color);
        keyHandles->curveKeyIdx = keyId;
        keyHandles->curveKeyUid = info->curve->GetKeyAt(keyId).uid;

        Curve::Key curveKey = info->curve->GetKeyAt(keyId);
        Curve::Key lastCurveKey = info->curve->GetKeyAt(Math::Max(0, keyId - 1));
        Curve::Key nextCurveKey = info->curve->GetKeyAt(Math::Min(keyId + 1, info->curve->GetKeys().Count()));

        // main 
        keyHandles->mainHandle->curveInfo = info;
        keyHandles->mainHandle->SetPosition(Vec2F(curveKey.position, curveKey.value));
        keyHandles->mainHandle->onChangedPos = [=](const Vec2F& pos) { OnCurveKeyMainHandleDragged(info, keyHandles, pos); };
        keyHandles->mainHandle->onRightButtonReleased = THIS_FUNC(OnCursorRightMouseReleased);
        keyHandles->mainHandle->onPressed = [=]() { OnCurveKeyMainHandlePressed(info, keyHandles); };
        keyHandles->mainHandle->onChangeCompleted = THIS_FUNC(OnTransformCompleted);
        keyHandles->mainHandle->messageFallDownListener = this;

        keyHandles->mainHandle->onDraw = [=]() {
            if (o2Input.IsKeyDown(VK_F1))
                o2Debug.DrawText(keyHandles->mainHandle->GetScreenPosition(), String(keyHandles->curveKeyIdx));
        };


        // left support handle
        keyHandles->leftSupportHandle->curveInfo = info;
        keyHandles->leftSupportHandle->SetPosition(Vec2F(curveKey.position + curveKey.leftSupportPosition,
                                                  curveKey.value + curveKey.leftSupportValue));

        keyHandles->leftSupportHandle->onChangedPos =
            [=](const Vec2F& pos) { OnCurveKeyLeftSupportHandleDragged(info, keyHandles, pos); };

        keyHandles->leftSupportHandle->checkPositionFunc =
            [=](const Vec2F& pos) { return CheckLeftSupportHandlePosition(info, keyHandles, pos); };

        keyHandles->leftSupportHandle->enabled = false;
        keyHandles->leftSupportHandle->onRightButtonReleased = THIS_FUNC(OnCursorRightMouseReleased);
        keyHandles->leftSupportHandle->onPressed = THIS_FUNC(OnTransformBegin);
        keyHandles->leftSupportHandle->onChangeCompleted = THIS_FUNC(OnTransformCompleted);
        keyHandles->leftSupportHandle->messageFallDownListener = this;


        // right support handle
        keyHandles->rightSupportHandle->curveInfo = info;
        keyHandles->rightSupportHandle->SetPosition(Vec2F(curveKey.position + curveKey.rightSupportPosition,
                                                   curveKey.value + curveKey.rightSupportValue));

        keyHandles->rightSupportHandle->onChangedPos =
            [=](const Vec2F& pos) { OnCurveKeyRightSupportHandleDragged(info, keyHandles, pos); };

        keyHandles->rightSupportHandle->checkPositionFunc =
            [=](const Vec2F& pos) { return CheckRightSupportHandlePosition(info, keyHandles, pos); };

        keyHandles->rightSupportHandle->enabled = false;
        keyHandles->rightSupportHandle->onRightButtonReleased = THIS_FUNC(OnCursorRightMouseReleased);
        keyHandles->rightSupportHandle->onPressed = THIS_FUNC(OnTransformBegin);
        keyHandles->rightSupportHandle->onChangeCompleted = THIS_FUNC(OnTransformCompleted);
        keyHandles->rightSupportHandle->messageFallDownListener = this;


        // Top range handle
        keyHandles->topRangeHandle->curveInfo = info;
        keyHandles->topRangeHandle->SetPosition(Vec2F(curveKey.position, curveKey.value + curveKey.valueRange*0.5f));

        keyHandles->topRangeHandle->onChangedPos =
            [=](const Vec2F& pos) { OnCurveKeyTopRangeHandleDragged(info, keyHandles, pos); };

        keyHandles->topRangeHandle->checkPositionFunc =
            [=](const Vec2F& pos) { return CheckTopRangeHandlePosition(info, keyHandles, pos); };

        keyHandles->topRangeHandle->enabled = false;
        keyHandles->topRangeHandle->onRightButtonReleased = THIS_FUNC(OnCursorRightMouseReleased);
        keyHandles->topRangeHandle->onPressed = THIS_FUNC(OnTransformBegin);
        keyHandles->topRangeHandle->onChangeCompleted = THIS_FUNC(OnTransformCompleted);
        keyHandles->topRangeHandle->messageFallDownListener = this;
        keyHandles->topRangeHandle->cursorType = CursorType::SizeNS;
        keyHandles->topRangeHandle->angle = Math::PI()*0.5f;


        // Bottom range handle
        keyHandles->bottomRangeHandle->curveInfo = info;
        keyHandles->bottomRangeHandle->SetPosition(Vec2F(curveKey.position, curveKey.value - curveKey.valueRange*0.5f));

        keyHandles->bottomRangeHandle->onChangedPos =
            [=](const Vec2F& pos) { OnCurveKeyBottomRangeHandleDragged(info, keyHandles, pos); };

        keyHandles->bottomRangeHandle->checkPositionFunc =
            [=](const Vec2F& pos) { return CheckBottomRangeHandlePosition(info, keyHandles, pos); };

        keyHandles->bottomRangeHandle->enabled = false;
        keyHandles->bottomRangeHandle->onRightButtonReleased = THIS_FUNC(OnCursorRightMouseReleased);
        keyHandles->bottomRangeHandle->onPressed = THIS_FUNC(OnTransformBegin);
        keyHandles->bottomRangeHandle->onChangeCompleted = THIS_FUNC(OnTransformCompleted);
        keyHandles->bottomRangeHandle->messageFallDownListener = this;
        keyHandles->bottomRangeHandle->cursorType = CursorType::SizeNS;
        keyHandles->bottomRangeHandle->angle = Math::PI()*1.5f;

        for (int i = keyId; i < info->handles.Count(); i++)
            info->handles[i]->curveKeyIdx++;

        // Register handles
        info->handles.Insert(keyHandles, keyId);

        mSupportHandles.Add(keyHandles->leftSupportHandle);
        mSupportHandles.Add(keyHandles->rightSupportHandle);

        keyHandles->mainHandle->SetSelectionGroup(Ref(this));
        keyHandles->leftSupportHandle->SetSelectionGroup(mSupportHandlesGroup);
        keyHandles->rightSupportHandle->SetSelectionGroup(mSupportHandlesGroup);

        mNeedRedraw = true;
    }

    void CurvesEditor::RemoveCurveKeyHandles(const Ref<CurveInfo>& info, int keyId)
    {
        for (int i = keyId + 1; i < info->handles.Count(); i++)
            info->handles[i]->curveKeyIdx--;

        info->handles[keyId]->mainHandle->SetSelectionGroup(nullptr);
        info->handles[keyId]->leftSupportHandle->SetSelectionGroup(nullptr);
        info->handles[keyId]->rightSupportHandle->SetSelectionGroup(nullptr);

        mSupportHandles.Remove(info->handles[keyId]->leftSupportHandle);
        mSupportHandles.Remove(info->handles[keyId]->rightSupportHandle);

        info->handles.RemoveAt(keyId);

        mNeedRedraw = true;
    }

    void CurvesEditor::OnCurveKeyMainHandleDragged(const Ref<CurveInfo>& info, const Ref<KeyHandles>& handles, const Vec2F& position)
    {
        info->BeginCurveManualChange();

        Curve::Key key = info->curve->GetKeyAt(handles->curveKeyIdx);

        Vec2F initialDragPoint = handles->mainHandle->GetDraggingBeginPosition();

        if (handles->mainHandle->IsPressed())
        {
            if (o2Input.IsKeyDown(VK_CONTROL) && o2Input.IsKeyDown(VK_SHIFT))
            {
                key.position = Math::Round(position.x*10.0f)/10.0f;
                key.value = Math::Round(position.y*10.0f)/10.0f;
            }
            else if (o2Input.IsKeyDown(VK_CONTROL))
            {
                key.position = position.x;
                key.value = initialDragPoint.y;
            }
            else if (o2Input.IsKeyDown(VK_SHIFT))
            {
                key.position = initialDragPoint.x;
                key.value = position.y;
            }
            else
            {
                key.position = position.x;
                key.value = position.y;
            }
        }
        else
        {
            key.position = position.x;
            key.value = position.y;
        }

        int newKeyIdx = 0;
        for (auto& k : info->curve->GetKeys())
        {
            if (k.position > key.position)
                break;

            newKeyIdx++;
        }

        if (newKeyIdx > handles->curveKeyIdx)
            newKeyIdx--;

        if (newKeyIdx != handles->curveKeyIdx)
        {
            info->curve->RemoveKeyAt(handles->curveKeyIdx);
            info->curve->InsertKey(key);

            info->handles.RemoveAt(handles->curveKeyIdx);
            info->handles.Insert(handles, newKeyIdx);

            for (int i = 0; i < info->handles.Count(); i++)
            {
                info->handles[i]->curveKeyIdx = i;
                info->handles[i]->curveKeyUid = key.uid;
            }
        }

        info->curve->SetKey(key, handles->curveKeyIdx);

        info->UpdateHandles();
        info->UpdateApproximatedPoints();

        CheckHandlesVisible();
        UpdateTransformFrame();
        RecalculateViewArea();
        mNeedRedraw = true;

        info->CompleteCurveManualChange();
    }

    void CurvesEditor::OnCurveKeyMainHandlePressed(const Ref<CurveInfo>& info, const Ref<KeyHandles>& handles)
    {
        OnTransformBegin();

        if (o2Input.IsKeyDown(VK_MENU))
        {
            info->BeginCurveManualChange();

            Curve::Key key = info->curve->GetKeyAt(handles->curveKeyIdx);

            if (Math::Equals(key.valueRange, 0.0f))
                key.valueRange = 1.0f;
            else
                key.valueRange = 0.0f;

            info->curve->SetKey(key, handles->curveKeyIdx);

            info->UpdateHandles();
            info->UpdateApproximatedPoints();

            CheckHandlesVisible();
            UpdateTransformFrame();
            RecalculateViewArea();
            mNeedRedraw = true;

            info->CompleteCurveManualChange();
        }
    }

    void CurvesEditor::OnCurveKeyLeftSupportHandleDragged(const Ref<CurveInfo>& info, const Ref<KeyHandles>& handles, const Vec2F& position)
    {
        info->BeginCurveManualChange();

        Curve::Key key = info->curve->GetKeyAt(handles->curveKeyIdx);
        Curve::Key prevKey = info->curve->GetKeyAt(Math::Max(handles->curveKeyIdx - 1, 0));

        if (o2Input.IsKeyDown(VK_CONTROL) && o2Input.IsKeyDown(VK_SHIFT))
        {
            Vec2F initialDragPoint = handles->leftSupportHandle->GetDraggingBeginPosition();
            float dst = (initialDragPoint - Vec2F(key.position, key.value)).Length();
            Vec2F v = (position - Vec2F(key.position, key.value)).Normalized()*dst;
            key.leftSupportPosition = v.x;
            key.leftSupportValue = v.y;
        }
        else if (o2Input.IsKeyDown(VK_CONTROL))
        {
            key.leftSupportPosition = position.x - key.position;
            key.leftSupportValue = 0;
        }
        else if (o2Input.IsKeyDown(VK_SHIFT))
        {
            Vec2F initialDragPoint = handles->leftSupportHandle->GetDraggingBeginPosition();

            Vec2F v = initialDragPoint - Vec2F(key.position, key.value);
            float dst = (position - Vec2F(key.position, key.value)).Length();
            v = v.Normalized()*dst;
            key.leftSupportPosition = v.x;
            key.leftSupportValue = v.y;
        }
        else
        {
            key.leftSupportPosition = position.x - key.position;
            key.leftSupportValue = position.y - key.value;
        }

        if (key.supportsType == Curve::Key::Type::Flat || key.supportsType == Curve::Key::Type::Discrete ||
            key.supportsType == Curve::Key::Type::Smooth)
        {
            key.supportsType = Curve::Key::Type::Free;
        }

        if (key.supportsType == Curve::Key::Type::Free && handles->curveKeyIdx < info->curve->GetKeys().Count() - 1)
        {
            Curve::Key nextKey = info->curve->GetKeyAt(Math::Min(handles->curveKeyIdx + 1,
                                                       info->curve->GetKeys().Count() - 1));

            key.rightSupportPosition = -key.leftSupportPosition;
            key.rightSupportValue = -key.leftSupportValue;
        }

        info->curve->SetKey(key, handles->curveKeyIdx);

        info->UpdateHandles();
        info->UpdateApproximatedPoints();

        CheckHandlesVisible();
        RecalculateViewArea();
        mNeedRedraw = true;

        info->CompleteCurveManualChange();
    }

    void CurvesEditor::OnCurveKeyRightSupportHandleDragged(const Ref<CurveInfo>& info, const Ref<KeyHandles>& handles, const Vec2F& position)
    {
        info->BeginCurveManualChange();

        Curve::Key key = info->curve->GetKeyAt(handles->curveKeyIdx);
        Curve::Key nextKey = info->curve->GetKeyAt(Math::Min(handles->curveKeyIdx + 1, info->curve->GetKeys().Count() - 1));

        if (o2Input.IsKeyDown(VK_CONTROL) && o2Input.IsKeyDown(VK_SHIFT))
        {
            Vec2F initialDragPoint = handles->rightSupportHandle->GetDraggingBeginPosition();
            float dst = (initialDragPoint - Vec2F(key.position, key.value)).Length();
            Vec2F v = (position - Vec2F(key.position, key.value)).Normalized()*dst;
            key.rightSupportPosition = v.x;
            key.rightSupportValue = v.y;
        }
        else if (o2Input.IsKeyDown(VK_CONTROL))
        {
            key.rightSupportPosition = position.x - key.position;
            key.rightSupportValue = 0;
        }
        else if (o2Input.IsKeyDown(VK_SHIFT))
        {
            Vec2F initialDragPoint = handles->rightSupportHandle->GetDraggingBeginPosition();

            Vec2F v = initialDragPoint - Vec2F(key.position, key.value);
            float dst = (position - Vec2F(key.position, key.value)).Length();
            v = v.Normalized()*dst;
            key.rightSupportPosition = v.x;
            key.rightSupportValue = v.y;
        }
        else
        {
            key.rightSupportPosition = position.x - key.position;
            key.rightSupportValue = position.y - key.value;
        }

        if (key.supportsType == Curve::Key::Type::Flat || key.supportsType == Curve::Key::Type::Discrete ||
            key.supportsType == Curve::Key::Type::Smooth)
        {
            key.supportsType = Curve::Key::Type::Free;
        }

        if (key.supportsType == Curve::Key::Type::Free && handles->curveKeyIdx > 0)
        {
            Curve::Key prevKey = info->curve->GetKeyAt(Math::Max(handles->curveKeyIdx - 1, 0));

            Vec2F rightSupportVector = Vec2F(key.rightSupportPosition, key.rightSupportValue);

            key.leftSupportPosition = -key.rightSupportPosition;
            key.leftSupportValue = -key.rightSupportValue;

            handles->leftSupportHandle->position = Vec2F(key.position + key.leftSupportPosition,
                                                        key.value + key.leftSupportValue);
        }

        info->curve->SetKey(key, handles->curveKeyIdx);

        info->UpdateHandles();
        info->UpdateApproximatedPoints();

        CheckHandlesVisible();
        RecalculateViewArea();
        mNeedRedraw = true;

        info->CompleteCurveManualChange();
    }

    void CurvesEditor::OnCurveKeyTopRangeHandleDragged(const Ref<CurveInfo>& info, const Ref<KeyHandles>& handles, const Vec2F& position)
    {
        info->BeginCurveManualChange();

        Curve::Key key = info->curve->GetKeyAt(handles->curveKeyIdx);

        key.valueRange = (position.y - key.value)*2.0f;

        info->curve->SetKey(key, handles->curveKeyIdx);

        info->UpdateHandles();
        info->UpdateApproximatedPoints();

        CheckHandlesVisible();
        RecalculateViewArea();
        mNeedRedraw = true;

        info->CompleteCurveManualChange();
    }

    void CurvesEditor::OnCurveKeyBottomRangeHandleDragged(const Ref<CurveInfo>& info, const Ref<KeyHandles>& handles, const Vec2F& position)
    {
        info->BeginCurveManualChange();

        Curve::Key key = info->curve->GetKeyAt(handles->curveKeyIdx);

        key.valueRange = (key.value - position.y)*2.0f;

        info->curve->SetKey(key, handles->curveKeyIdx);

        info->UpdateHandles();
        info->UpdateApproximatedPoints();

        CheckHandlesVisible();
        RecalculateViewArea();
        mNeedRedraw = true;

        info->CompleteCurveManualChange();
    }

    Vec2F CurvesEditor::CheckLeftSupportHandlePosition(const Ref<CurveInfo>& info, const Ref<KeyHandles>& handles, const Vec2F& position)
    {
        Curve::Key key = info->curve->GetKeyAt(handles->curveKeyIdx);
        Curve::Key prevKey = info->curve->GetKeyAt(Math::Max(handles->curveKeyIdx - 1, 0));

        Vec2F relativePos = position - Vec2F(key.position, key.value);

        if (relativePos.x > 0.0f)
            relativePos.x = 0.0f;

        if (relativePos.x < prevKey.position - key.position)
            relativePos *= (prevKey.position - key.position)/relativePos.x;

        return Vec2F(key.position, key.value) + relativePos;
    }

    Vec2F CurvesEditor::CheckRightSupportHandlePosition(const Ref<CurveInfo>& info, const Ref<KeyHandles>& handles, const Vec2F& position)
    {
        Curve::Key key = info->curve->GetKeyAt(handles->curveKeyIdx);
        Curve::Key nextKey = info->curve->GetKeyAt(Math::Min(handles->curveKeyIdx + 1, info->curve->GetKeys().Count() - 1));

        Vec2F relativePos = position - Vec2F(key.position, key.value);

        if (relativePos.x < 0.0f)
            relativePos.x = 0.0f;

        if (relativePos.x > nextKey.position - key.position)
            relativePos *= (nextKey.position - key.position)/relativePos.x;

        return Vec2F(key.position, key.value) + relativePos;
    }

    Vec2F CurvesEditor::CheckTopRangeHandlePosition(const Ref<CurveInfo>& info, const Ref<KeyHandles>& handles, const Vec2F& position)
    {
        Curve::Key key = info->curve->GetKeyAt(handles->curveKeyIdx);

        return Vec2F(key.position, position.y);
    }

    Vec2F CurvesEditor::CheckBottomRangeHandlePosition(const Ref<CurveInfo>& info, const Ref<KeyHandles>& handles, const Vec2F& position)
    {
        Curve::Key key = info->curve->GetKeyAt(handles->curveKeyIdx);

        return Vec2F(key.position, position.y);
    }

    void CurvesEditor::OnCursorDblClicked(const Input::Cursor& cursor)
    {
        const float createPointDistanceThreshold = 7;

        Ref<CurveInfo> clickedCurveInfo;
        Curve::Key newKey;
        Vec2F localCursorPos = cursor.position;

        for (auto& info : mCurves)
        {
            auto& keys = info->curve->GetKeys();
            int keyIdx = 0;
            for (auto& key : keys)
            {
                if (keyIdx == 0)
                {
                    keyIdx++;
                    continue;
                }

                const ApproximationValue* points = key.GetTopApproximatedPoints();
                for (int i = 1; i < key.GetApproximatedPointsCount(); i++)
                {
                    Vec2F a = CurveViewToLocal(points[i - 1], info->viewScale, info->viewOffset);
                    Vec2F b = CurveViewToLocal(points[i], info->viewScale, info->viewOffset);

                    Vec2F ab = b - a;
                    float abl = ab.Length();
                    Vec2F abn = ab/abl;
                    Vec2F abnp = abn.Perpendicular();
                    Vec2F ac = cursor.position - LocalToScreenPoint(a);
                    Vec2F lac = ScreenToLocalPoint(cursor.position) - a;

                    float pointDistance = (LocalToScreenPoint(b) - cursor.position).Length();
                    float lineDistance = Math::Abs(abnp.Dot(ac));
                    float proj = abn.Dot(lac);

                    if (pointDistance < createPointDistanceThreshold ||
                        (lineDistance < createPointDistanceThreshold && proj > 0.0f && proj < abl))
                    {
                        Vec2F p = abnp*(abnp.Dot(localCursorPos - a)) + localCursorPos;
                        Vec2F pc = LocalToCurveView(p, info->viewScale, info->viewOffset);
                        newKey.position = pc.x;
                        newKey.value = pc.y;
                        newKey.supportsType = Curve::Key::Type::Smooth;

                        clickedCurveInfo = info;

                        break;
                    }
                }

                if (clickedCurveInfo)
                    break;

                keyIdx++;
            }

            if (clickedCurveInfo)
                break;
        }

        bool addedKey = false;
        if (clickedCurveInfo)
        {
            clickedCurveInfo->disableChangesHandling = true;

            int idx = clickedCurveInfo->curve->InsertKey(newKey);

            clickedCurveInfo->disableChangesHandling = false;

            AddCurveKeyHandles(clickedCurveInfo, idx);
            clickedCurveInfo->UpdateApproximatedPoints();

            mNeedRedraw = true;

            SelectHandle(clickedCurveInfo->handles[idx]->mainHandle);
            CheckHandlesVisible();

            addedKey = true;
        }
        else if (mCurves.Count() == 1)
        {
            clickedCurveInfo = mCurves[0];

            Vec2F curveViewCursorPos = LocalToCurveView(localCursorPos, clickedCurveInfo->viewScale, clickedCurveInfo->viewOffset);
            newKey.position = curveViewCursorPos.x;
            newKey.value = curveViewCursorPos.y;

            clickedCurveInfo->disableChangesHandling = true;

            int idx = clickedCurveInfo->curve->InsertKey(newKey);

            clickedCurveInfo->disableChangesHandling = false;

            AddCurveKeyHandles(mCurves[0], idx);
            clickedCurveInfo->UpdateApproximatedPoints();

            mNeedRedraw = true;

            SelectHandle(clickedCurveInfo->handles[idx]->mainHandle);
            CheckHandlesVisible();

            addedKey = true;
        }

        if (addedKey)
        {
            Vector<CurveKeysInfo> keyInfos;
            keyInfos.Add(CurveKeysInfo());
            keyInfos.Last().curveId = clickedCurveInfo->curveId;
            keyInfos.Last().keys.Add(newKey);

            auto action = mmake<CurveAddKeysAction>(keyInfos, Ref(this));
            DoneAction(action);
        }
    }

    void CurvesEditor::SmoothKey(const Ref<CurveInfo>& info, int idx)
    {
        Curve::Key key = info->curve->GetKeyAt(idx);
        Vec2F thisKeyPoint(key.position, key.value);

        const float autoSmoothCoef = 0.5f;

        if (idx == 0)
        {
            Curve::Key nextKey = info->curve->GetKeyAt(Math::Min(idx + 1, info->curve->GetKeys().Count()));

            Vec2F nextKeyPoint(nextKey.position, nextKey.value);
            Vec2F supportVec = (nextKeyPoint - thisKeyPoint)*autoSmoothCoef;

            key.rightSupportPosition = supportVec.x;
            key.rightSupportValue = supportVec.y;
        }
        else if (idx == info->curve->GetKeys().Count() - 1)
        {
            Curve::Key lastKey = info->curve->GetKeyAt(Math::Max(0, idx - 1));

            Vec2F lastKeyPoint(lastKey.position, lastKey.value);
            Vec2F supportVec = (lastKeyPoint - thisKeyPoint)*autoSmoothCoef;

            key.leftSupportPosition = supportVec.x;
            key.leftSupportValue = supportVec.y;
        }
        else
        {
            Curve::Key lastKey = info->curve->GetKeyAt(Math::Max(0, idx - 1));
            Curve::Key nextKey = info->curve->GetKeyAt(Math::Min(idx + 1, info->curve->GetKeys().Count()));

            Vec2F lastKeyPoint(lastKey.position, lastKey.value);
            Vec2F thisKeyPoint(key.position, key.value);
            Vec2F nextKeyPoint(nextKey.position, nextKey.value);

            Vec2F thisToLast = lastKeyPoint - thisKeyPoint;
            Vec2F thisToNext = nextKeyPoint - thisKeyPoint;

            float supportLength = Math::Min(thisToNext.Length(), thisToLast.Length())*autoSmoothCoef;
            Vec2F supportVec = Math::CalculateEllipseTangent(lastKeyPoint, thisKeyPoint, nextKeyPoint)*supportLength;

            key.leftSupportPosition = -supportVec.x; key.leftSupportValue = -supportVec.y;
            key.rightSupportPosition = supportVec.x; key.rightSupportValue = supportVec.y;
        }

        info->curve->SetKey(key, idx);

        info->UpdateApproximatedPoints();

        info->handles[idx]->leftSupportHandle->SetPosition(Vec2F(key.position + key.leftSupportPosition,
                                                          key.value + key.leftSupportValue));

        info->handles[idx]->rightSupportHandle->SetPosition(Vec2F(key.position + key.rightSupportPosition,
                                                           key.value + key.rightSupportValue));

        if (idx > 0)
        {
            Curve::Key lastKey = info->curve->GetKeyAt(Math::Max(0, idx - 1));

            info->handles[idx - 1]->rightSupportHandle->SetPosition(
                Vec2F(lastKey.position + lastKey.rightSupportPosition, lastKey.value + lastKey.rightSupportValue));
        }

        if (idx < info->curve->GetKeys().Count() - 1)
        {
            Curve::Key nextKey = info->curve->GetKeyAt(Math::Min(idx + 1, info->curve->GetKeys().Count()));

            info->handles[idx + 1]->leftSupportHandle->SetPosition(
                Vec2F(nextKey.position + nextKey.leftSupportPosition, nextKey.value + nextKey.leftSupportValue));
        }

        RecalculateViewArea();

        mNeedRedraw = true;
    }

    void CurvesEditor::OnCursorPressed(const Input::Cursor& cursor)
    {
        Focus();

        mSelectingPressedPoint = cursor.position;

        if (!o2Input.IsKeyDown(VK_CONTROL))
        {
            DeselectAll();
            mSupportHandlesGroup->DeselectAll();
        }
    }

    void CurvesEditor::OnCursorReleased(const Input::Cursor& cursor)
    {
        for (auto& handle : mSelectingHandlesBuf)
        {
            SetHandleSelectedState(handle, false);
            handle->SetSelected(true);
        }

        mSelectingHandlesBuf.Clear();
        UpdateTransformFrame();
        CheckHandlesVisible();
    }

    void CurvesEditor::OnCursorStillDown(const Input::Cursor& cursor)
    {
        for (auto& handle : mSelectingHandlesBuf)
            SetHandleSelectedState(handle, false);

        mSelectingHandlesBuf.Clear();

        RectF selectionLocalRect(mSelectingPressedPoint, cursor.position);

        for (auto& handle : mHandles)
        {
            auto curveHandle = DynamicCast<CurveHandle>(handle);
            if (handle->IsEnabled() && selectionLocalRect.IsInside(curveHandle->GetLocalPosition()) &&
                !mSelectedHandles.Contains(handle))
            {
                mSelectingHandlesBuf.Add(curveHandle);
                SetHandleSelectedState(handle, true);
            }
        }

        for (auto& handle : mSupportHandlesGroup->GetAllHandles())
        {
            auto curveHandle = DynamicCast<CurveHandle>(handle);
            if (handle->IsEnabled() && selectionLocalRect.IsInside(curveHandle->GetLocalPosition()) &&
                !mSupportHandlesGroup->GetSelectedHandles().Contains(handle))
            {
                mSelectingHandlesBuf.Add(curveHandle);
                SetHandleSelectedState(handle, true);
            }
        }
    }

    void CurvesEditor::OnCursorRightMouseStayDown(const Input::Cursor& cursor)
    {
        if (cursor.delta.Length() > 0.1f)
            mIsViewScrolling = true;

        ScrollView::OnCursorRightMouseStayDown(cursor);
    }

    void CurvesEditor::OnCursorRightMouseReleased(const Input::Cursor& cursor)
    {
        Focus();

        if (!mIsViewScrolling)
        {
            Curve::Key::Type supportsType = Curve::Key::Type::Free;
            bool supportsDifferent = false;
            bool first = true;
            bool someSelected = false;

            for (auto& curve : mCurves)
            {
                for (auto& handles : curve->handles)
                {
                    if (!handles->mainHandle->IsSelected())
                        continue;

                    someSelected = true;

                    if (first)
                    {
                        first = false;
                        supportsType = curve->curve->GetKeyAt(handles->curveKeyIdx).supportsType;
                    }
                    else if (supportsType != curve->curve->GetKeyAt(handles->curveKeyIdx).supportsType)
                    {
                        supportsDifferent = true;
                        break;
                    }
                }

                if (supportsDifferent)
                    break;
            }

            mContextMenu->Show();

            if (supportsDifferent || !someSelected)
            {
                for (int i = 0; i < 5; i++)
                    mContextMenu->SetItemChecked(i + 1, false);
            }
            else
            {
                for (int i = 0; i < 5; i++)
                    mContextMenu->SetItemChecked(i + 1, i == (int)supportsType);
            }
        }

        mIsViewScrolling = false;

        ScrollView::OnCursorRightMouseReleased(cursor);
    }

    void CurvesEditor::CheckHandlesVisible()
    {
        for (auto& info : mCurves)
        {
            for (auto& handles : info->handles)
            {
                Curve::Key key = info->curve->GetKeyAt(handles->curveKeyIdx);

                handles->leftSupportHandle->enabled = (handles->mainHandle->IsSelected() ||
                                                      handles->leftSupportHandle->IsSelected() ||
                                                       handles->rightSupportHandle->IsSelected()) && handles->curveKeyIdx > 0;

                handles->rightSupportHandle->enabled = (handles->mainHandle->IsSelected() ||
                                                        handles->leftSupportHandle->IsSelected() ||
                                                        handles->rightSupportHandle->IsSelected()) && handles->curveKeyIdx < info->handles.Count() - 1;

                handles->topRangeHandle->enabled = handles->mainHandle->IsSelected() && !Math::Equals(key.valueRange, 0.0f);
                handles->bottomRangeHandle->enabled = handles->mainHandle->IsSelected() && !Math::Equals(key.valueRange, 0.0f);
            }
        }
    }

    void CurvesEditor::UpdateTransformFrame()
    {
        mTransformFrameVisible = IsTransformFrameVisible();

        if (!mTransformFrameVisible)
            return;

        auto selectedHandle0 = DynamicCast<CurveHandle>(mSelectedHandles[0]);
        RectF aabb(selectedHandle0->GetLocalPosition(), selectedHandle0->GetLocalPosition());

        for (auto& handle : mSelectedHandles)
        {
            auto curveHandle = DynamicCast<CurveHandle>(handle);

            aabb.left = Math::Min(curveHandle->GetLocalPosition().x, aabb.left);
            aabb.right = Math::Max(curveHandle->GetLocalPosition().x, aabb.right);
            aabb.top = Math::Max(curveHandle->GetLocalPosition().y, aabb.top);
            aabb.bottom = Math::Min(curveHandle->GetLocalPosition().y, aabb.bottom);
        }

        mTransformFrameBasis = Basis(aabb.LeftBottom(), Vec2F::Right()*aabb.Width(), Vec2F::Up()*aabb.Height());
    }

    bool CurvesEditor::IsTransformFrameVisible() const
    {
        int selectedMainHandles = 0;
        for (auto& handle : mSelectedHandles)
        {
            auto curveHandle = DynamicCast<CurveHandle>(handle);
            if (!mSupportHandles.Contains(curveHandle))
                selectedMainHandles++;
        }

        return selectedMainHandles > 1;
    }

    void CurvesEditor::OnHandleCursorReleased(const Ref<DragHandle>& handle, const Input::Cursor& cursor)
    {
        SelectableDragHandlesGroup::OnHandleCursorReleased(handle, cursor);
        UpdateTransformFrame();
        CheckHandlesVisible();
    }

    void CurvesEditor::OnHandleBeganDragging(const Ref<DragHandle>& handle)
    {
        if (mSupportHandles.Contains(DynamicCast<CurveHandle>(handle)))
            return;

        SelectableDragHandlesGroup::OnHandleBeganDragging(handle);
    }

    void CurvesEditor::OnHandleMoved(const Ref<DragHandle>& handle, const Vec2F& cursorPos)
    {
        if (mSupportHandles.Contains(DynamicCast<CurveHandle>(handle)))
        {
            if (!handle->IsSelected())
                return;

            for (auto& handle : mSupportHandles)
            {
                if (!handle->IsSelected())
                    continue;

                handle->SetDragPosition(handle->screenToLocalTransformFunc(cursorPos) + handle->GetDraggingOffset());
                handle->onChangedPos(handle->GetPosition());
            }

            return;
        }

        SelectableDragHandlesGroup::OnHandleMoved(handle, cursorPos);
    }

    void CurvesEditor::OnHandleCompletedChange(const Ref<DragHandle>& handle)
    {
        OnTransformCompleted();
    }

    void CurvesEditor::OnSelectionChanged()
    {
        mNeedRedraw = true;
    }

    void CurvesEditor::SetSelectedKeysSupportsType(Curve::Key::Type type)
    {
        for (auto& info : mCurves)
        {
            auto keys = info->curve->GetKeys();

            for (auto& handles : info->handles)
            {
                if (handles->mainHandle->IsSelected())
                {
                    Curve::Key& key = keys[handles->curveKeyIdx];
                    key.supportsType = type;

                    switch (type)
                    {
                        case Curve::Key::Type::Flat:
                        key.leftSupportValue = 0;
                        key.rightSupportValue = 0;
                        break;

                        case Curve::Key::Type::Free:
                        key.rightSupportPosition = -key.leftSupportPosition;
                        key.rightSupportValue = -key.leftSupportValue;
                        break;

                        case Curve::Key::Type::Linear:
                        key.leftSupportPosition = 0;
                        key.rightSupportPosition = 0;
                        key.leftSupportValue = 0;
                        key.rightSupportValue = 0;
                        break;
                    }
                }
            }

            info->BeginCurveManualChange();

            info->curve->SetKeys(keys);
            info->UpdateHandles();
            info->UpdateApproximatedPoints();

            info->CompleteCurveManualChange();
        }

        CheckHandlesVisible();
        RecalculateViewArea();
        mNeedRedraw = true;
    }

    void CurvesEditor::OnTransformFrameTransformed(const Basis& basis)
    {
        Vec2F border(10, 10);
        Basis localBasis(ScreenToLocalPoint(basis.origin + border),
            (basis.xv - Vec2F(border.x*2.0f, 0))*mViewCamera.GetScale(),
                         (basis.yv - Vec2F(0, border.y*2.0f))*mViewCamera.GetScale());

        Basis lastTransformBasis = mTransformFrameBasis;
        Basis delta = mTransformFrameBasis.Inverted()*localBasis;

        if (delta.origin.Length() > 0.01f || delta.xv != Vec2F(1, 0) || delta.yv != Vec2F(0, 1))
        {
            for (auto& handle : mSelectedHandles)
            {
                auto curveHandle = DynamicCast<CurveHandle>(handle);
                curveHandle->SetPosition(curveHandle->LocalToCurveView(curveHandle->GetLocalPosition()*delta));
                curveHandle->onChangedPos(handle->GetPosition());
            }

            if (o2Input.IsKeyDown(VK_CONTROL))
            {
                float right = lastTransformBasis.origin.x + lastTransformBasis.xv.x;
                float left = lastTransformBasis.origin.x;

                float rightOffset = localBasis.origin.x + localBasis.xv.x - right;
                float leftOffset = localBasis.origin.x - left;

                bool rightChanged = !Math::Equals(rightOffset, 0.0f) && rightOffset > FLT_EPSILON;
                bool leftChanged = !Math::Equals(leftOffset, 0.0f) && leftOffset < FLT_EPSILON;

                for (auto& handle : mHandles)
                {
                    if (mSelectedHandles.Contains(handle))
                        continue;

                    if (handle->GetPosition().x >= right && rightChanged)
                    {
                        handle->position += Vec2F(rightOffset, 0);
                        handle->onChangedPos(handle->GetPosition());
                    }

                    if (handle->GetPosition().x <= left && leftChanged)
                    {
                        handle->position += Vec2F(leftOffset, 0);
                        handle->onChangedPos(handle->GetPosition());
                    }
                }
            }

            UpdateTransformFrame();
        }
    }

    void CurvesEditor::OnTransformBegin()
    {
        mBeforeTransformKeys.Clear();

        CurveKeysInfo keysInfo;
        for (auto& curveInfo : mCurves)
        {
            keysInfo.keys.Clear();
            keysInfo.selectedHandles.Clear();

            for (auto& handles : curveInfo->handles)
            {
                if (handles->IsSomeHandleSelected())
                {
                    keysInfo.keys.Add(curveInfo->curve->GetKeyAt(handles->curveKeyIdx));

                    keysInfo.selectedHandles.Add(SelectedHandlesInfo());
                    keysInfo.selectedHandles.Last().index = handles->curveKeyIdx;
                    keysInfo.selectedHandles.Last().mainHandle = handles->mainHandle->IsSelected();
                    keysInfo.selectedHandles.Last().leftSupportHandle = handles->leftSupportHandle->IsSelected();
                    keysInfo.selectedHandles.Last().rightSupportHandle = handles->rightSupportHandle->IsSelected();
                }
            }

            if (!keysInfo.keys.IsEmpty())
            {
                keysInfo.curveId = curveInfo->curveId;
                mBeforeTransformKeys.Add(keysInfo);
            }
        }
    }

    void CurvesEditor::OnTransformCompleted()
    {
        bool changed = false;
        for (auto& keysInfo : mBeforeTransformKeys)
        {
            auto curveInfo = mCurves.FindOrDefault([&](auto& x) { return x->curveId == keysInfo.curveId; });
            if (!curveInfo)
                continue;

            for (auto& key : curveInfo->curve->GetKeys())
            {
                if (!keysInfo.keys.Contains(key))
                {
                    changed = true;
                    break;
                }
            }

            if (changed)
                break;
        }

        if (changed)
        {
            Vector<CurveKeysChangeAction::KeysInfo> actionKeysInfos;
            for (auto& keysInfo : mBeforeTransformKeys)
            {
                auto curveInfo = mCurves.FindOrDefault([&](auto& x) { return x->curveId == keysInfo.curveId; });
                if (!curveInfo)
                    continue;

                actionKeysInfos.Add(CurveKeysChangeAction::KeysInfo());
                auto& actionCurveKeysInfo = actionKeysInfos.Last();

                actionCurveKeysInfo.curveId = keysInfo.curveId;
                actionCurveKeysInfo.beforeKeys = keysInfo.keys;
                actionCurveKeysInfo.selectedHandles = keysInfo.selectedHandles;

                for (auto& handles : curveInfo->handles)
                {
                    if (handles->IsSomeHandleSelected())
                        actionCurveKeysInfo.afterKeys.Add(curveInfo->curve->GetKeyAt(handles->curveKeyIdx));
                }
            }

            auto action = mmake<CurveKeysChangeAction>(actionKeysInfos, Ref(this));
            DoneAction(action);
        }
    }

    Vec2F CurvesEditor::CurveViewToLocal(const Vec2F& point, const Vec2F& viewScale, const Vec2F& viewOffset) const
    {
        return (point + viewOffset)*viewScale;
    }

    Vec2F CurvesEditor::LocalToCurveView(const Vec2F& point, const Vec2F& viewScale, const Vec2F& viewOffset) const
    {
        return point/viewScale - viewOffset;
    }

    void CurvesEditor::DoneAction(const Ref<IAction>& action)
    {
        if (actionsListDelegate) {
            actionsListDelegate->DoneAction(action);
        }
    }

    void CurvesEditor::OnEditPressed()
    {
        Curve::Key key;
        Ref<CurveInfo> curveInfo;
        int keyIdx = 0;

        for (auto& info : mCurves)
        {
            for (auto& handles : info->handles)
            {
                if (handles->mainHandle->IsSelected())
                {
                    curveInfo = info;
                    keyIdx = handles->curveKeyIdx;
                    key = info->curve->GetKeys()[keyIdx];

                    break;
                }
            }

            if (curveInfo)
                break;
        }

        if (!curveInfo)
            return;

        KeyEditDlg::Show(key, [=](const Curve::Key& newKey) {
            curveInfo->curve->SetKey(newKey, keyIdx);
        });
    }

    void CurvesEditor::SetHandlesTypesChecked(Curve::Key::Type type)
    {
        for (int i = 0; i < 5; i++)
            mContextMenu->SetItemChecked(i + 1, i == (int)type);

        SetSelectedKeysSupportsType(type);
    }

    void CurvesEditor::OnCopyPressed()
    {
        if (!IsFocused())
            return;

        Vector<Ref<CurveCopyInfo>> copyKeys;

        for (auto& curve : mCurves)
        {
            auto copyInfo = mmake<CurveCopyInfo>();
            copyInfo->curveId = curve->curveId;

            for (auto& handles : curve->handles)
            {
                if (!handles->mainHandle->IsSelected())
                    continue;

                copyInfo->keys.Add(curve->curve->GetKeyAt(handles->curveKeyIdx));
            }

            if (!copyInfo->keys.IsEmpty())
                copyKeys.Add(copyInfo);
        }

        DataDocument copyData;
        copyData = copyKeys;
        String copyDataStr = copyData.SaveAsString();

        Clipboard::SetText(copyDataStr);
    }

    void CurvesEditor::OnCutPressed()
    {
        if (!IsFocused())
            return;

        OnCopyPressed();
        OnDeletePressed();
    }

    void CurvesEditor::OnPastePressed()
    {
        if (!IsFocused())
            return;

        float insertPos = ScreenToLocalPoint(o2Input.cursorPos).x;
        DataDocument data;
        data.LoadFromData(Clipboard::GetText());

        Vector<CurveCopyInfo*> copyKeys;
        copyKeys = data;

        Vector<CurveKeysInfo> keyInfos;

        for (auto& curve : copyKeys)
        {
            auto curveInfo = mCurves.FindOrDefault([=](auto& x) { return x->curveId == curve->curveId; });

            if (curveInfo == nullptr)
            {
                if (copyKeys.Count() == 1 && mCurves.Count() == 1)
                    curveInfo = mCurves[0];
            }

            if (!curveInfo)
                continue;

            curveInfo->BeginCurveManualChange();

            keyInfos.Add(CurveKeysInfo());
            keyInfos.Last().curveId = curveInfo->curveId;

            float positionDelta = insertPos - curve->keys[0].position;
            for (auto& key : curve->keys)
            {
                key.position += positionDelta;
                key.uid = Math::Random();

                int position = curveInfo->curve->InsertKey(key);
                AddCurveKeyHandles(curveInfo, position);

                keyInfos.Last().keys.Add(key);
            }

            delete curve;

            curveInfo->UpdateApproximatedPoints();
            curveInfo->UpdateHandles();
            mNeedRedraw = true;

            curveInfo->CompleteCurveManualChange();
        }

        if (!keyInfos.IsEmpty())
        {
            auto action = mmake<CurveAddKeysAction>(keyInfos, Ref(this));
            DoneAction(action);
        }
    }

    void CurvesEditor::OnDeletePressed()
    {
        if (!IsFocused())
            return;

        Vector<CurveKeysInfo> keyInfos;

        for (auto& curveInfo : mCurves)
        {
            curveInfo->BeginCurveManualChange();

            auto copyInfo = mmake<CurveCopyInfo>();
            copyInfo->curveId = curveInfo->curveId;

            Vector<int> removingIdxs;
            for (auto& handles : curveInfo->handles)
            {
                if (!handles->mainHandle->IsSelected())
                    continue;

                removingIdxs.Add(handles->curveKeyIdx);
            }

            if (!removingIdxs.IsEmpty())
            {
                keyInfos.Add(CurveKeysInfo());
                keyInfos.Last().curveId = curveInfo->curveId;
            }

            for (int i = removingIdxs.Count() - 1; i >= 0; i--)
            {
                keyInfos.Last().keys.Add(curveInfo->curve->GetKeyAt(removingIdxs[i]));

                curveInfo->curve->RemoveKeyAt(removingIdxs[i]);
                RemoveCurveKeyHandles(curveInfo, removingIdxs[i]);
            }

            curveInfo->UpdateApproximatedPoints();
            curveInfo->UpdateHandles();
            mNeedRedraw = true;

            curveInfo->CompleteCurveManualChange();
        }

        mSelectedHandles.Clear();
        mSupportHandlesGroup->DeselectAll();
        UpdateTransformFrame();

        if (!keyInfos.IsEmpty())
        {
            auto action = mmake<CurveDeleteKeysAction>(keyInfos, Ref(this));
            DoneAction(action);
        }
    }

    void CurvesEditor::OnInsertPressed()
    {
        if (!IsFocused())
            return;

        float insertPos = ScreenToLocalPoint(o2Input.cursorPos).x;
        DataDocument data;
        data.LoadFromData(Clipboard::GetText());

        Vector<CurveCopyInfo*> copyKeys;
        copyKeys = data;

        for (auto& curve : copyKeys)
        {
            auto curveInfo = mCurves.FindOrDefault([=](auto& x) { return x->curveId == curve->curveId; });

            if (curveInfo == nullptr)
            {
                if (copyKeys.Count() == 1 && mCurves.Count() == 1)
                    curveInfo = mCurves[0];
            }

            if (!curveInfo)
                continue;

            float length = curve->keys.Last().position - curve->keys[0].position;
            curveInfo->curve->MoveKeysFrom(insertPos, length);

            float positionDelta = insertPos - curve->keys[0].position;
            for (auto& key : curve->keys)
            {
                key.position += positionDelta;

                int position = curveInfo->curve->InsertKey(key);
                AddCurveKeyHandles(curveInfo, position);
            }

            delete curve;
        }
    }

    void CurvesEditor::OnUndoPressed()
    {
        if (actionsListDelegate)
            actionsListDelegate->UndoAction();
    }

    void CurvesEditor::OnRedoPressed()
    {
        if (actionsListDelegate)
            actionsListDelegate->RedoAction();
    }

    CurvesEditor::CurveInfo::CurveInfo()
    { }

    CurvesEditor::CurveInfo::~CurveInfo()
    {
        for (auto& x : handles)
        {
            x->mainHandle->SetSelectionGroup(nullptr);
            x->leftSupportHandle->SetSelectionGroup(nullptr);
            x->rightSupportHandle->SetSelectionGroup(nullptr);
        }

        if (curve)
            curve->onKeysChanged -= MakeSubscription(this, &CurveInfo::OnCurveChanged, []() {});
    }

    void CurvesEditor::CurveInfo::UpdateHandles()
    {
        for (int i = 0; i < handles.Count(); i++)
        {
            Curve::Key key = curve->GetKeyAt(i);
            handles[i]->mainHandle->position = Vec2F(key.position, key.value);

            if (!Math::Equals(key.valueRange, 0.0f))
            {
                handles[i]->topRangeHandle->position = Vec2F(key.position, key.value + key.valueRange*0.5f);
                handles[i]->bottomRangeHandle->position = Vec2F(key.position, key.value - key.valueRange*0.5f);

                handles[i]->topRangeHandle->enabled = handles[i]->mainHandle->IsSelected();
                handles[i]->bottomRangeHandle->enabled = handles[i]->mainHandle->IsSelected();
            }
            else
            {
                handles[i]->topRangeHandle->enabled = false;
                handles[i]->bottomRangeHandle->enabled = false;
            }

            if (i > 0)
            {
                handles[i]->leftSupportHandle->position = Vec2F(key.position + key.leftSupportPosition,
                                                               key.value + key.leftSupportValue);

                handles[i]->leftSupportHandle->enabled = handles[i]->leftSupportHandle->IsSelected() ||
                    handles[i]->mainHandle->IsSelected();
            }
            else 
                handles[i]->leftSupportHandle->enabled = false;

            if (i < handles.Count() - 1)
            {
                handles[i]->rightSupportHandle->position = Vec2F(key.position + key.rightSupportPosition,
                                                                key.value + key.rightSupportValue);

                handles[i]->rightSupportHandle->enabled = handles[i]->rightSupportHandle->IsSelected() ||
                    handles[i]->mainHandle->IsSelected();
            }
            else 
                handles[i]->rightSupportHandle->enabled = false;
        }
    }

    void CurvesEditor::CurveInfo::UpdateApproximatedPoints()
    {
        approximatedPoints.Clear();

        auto keys = curve->GetKeys();
        for (int i = 1; i < keys.Count(); i++)
        {
            auto& leftKey = keys[i - 1];
            auto& rightKey = keys[i];

            const ApproximationValue* keyTopPoints = rightKey.GetTopApproximatedPoints();
            const ApproximationValue* keyBottomPoints = rightKey.GetBottomApproximatedPoints();

            for (int j = 0; j < rightKey.GetApproximatedPointsCount(); j++)
            {
                Vec2F pointTop = ((Vec2F)keyTopPoints[j] + viewOffset)*viewScale;
                Vec2F pointBottom = ((Vec2F)keyBottomPoints[j] + viewOffset)*viewScale;

                approximatedPoints.Add({ pointTop, pointBottom });
            }
        }
    }

    void CurvesEditor::CurveInfo::AdjustScale()
    {
        if (editor.Lock()->mAdjustCurvesScale)
        {
            RectF rect = curve->GetRect();
            viewScale = Vec2F(1, Math::Min(100.0f, rect.Height() < FLT_EPSILON ? 1.0f : 1.0f/rect.Height()));
            viewOffset = Vec2F(0, -rect.bottom);
        }
        else
        {
            viewScale = Vec2F(1, 1);
            viewOffset = Vec2F();
        }

        UpdateApproximatedPoints();
        UpdateHandles();
    }

    void CurvesEditor::CurveInfo::OnCurveChanged()
    {
        if (disableChangesHandling)
            return;

        auto editor = this->editor.Lock();

        if (handles.Count() != curve->GetKeys().Count())
        {
            Vector<UInt64> selectedMain, selectedLeft, selectedRight;

            for (auto& handle : handles)
            {
                if (handle->leftSupportHandle->IsSelected())
                {
                    selectedLeft.Add(handle->curveKeyUid);
                    handle->leftSupportHandle->SetSelectionGroup(nullptr);
                }

                editor->mSupportHandles.Remove(handle->leftSupportHandle);

                if (handle->rightSupportHandle->IsSelected())
                {
                    selectedRight.Add(handle->curveKeyUid);
                    handle->rightSupportHandle->SetSelectionGroup(nullptr);
                }

                editor->mSupportHandles.Remove(handle->rightSupportHandle);

                if (handle->mainHandle->IsSelected())
                {
                    selectedMain.Add(handle->curveKeyUid);
                    handle->mainHandle->SetSelected(false);
                }

                editor->mHandles.Remove(handle->mainHandle);
            }

            handles.clear();

            for (int i = 0; i < curve->GetKeys().Count(); i++)
                editor->AddCurveKeyHandles(Ref(this), i);

            for (auto& handle : handles)
            {
                if (selectedMain.Contains(handle->curveKeyUid))
                    handle->mainHandle->Select();

                if (selectedLeft.Contains(handle->curveKeyUid))
                    handle->leftSupportHandle->Select();

                if (selectedRight.Contains(handle->curveKeyUid))
                    handle->rightSupportHandle->Select();
            }
        }

        AdjustScale();
        UpdateHandles();
    }

    void CurvesEditor::CurveInfo::BeginCurveManualChange()
    {
        disableChangesHandling = true;
    }

    void CurvesEditor::CurveInfo::CompleteCurveManualChange()
    {
        disableChangesHandling = false;
    }

    CurvesEditor::KeyHandles::KeyHandles(const CurveHandle& mainSample, const CurveHandle& supportSample,
                                         const CurveHandle& rangeSample,
                                         const Ref<CurvesEditor>& editor, const Color4& color):
        mainHandle(mmake<CurveHandle>(mainSample)), leftSupportHandle(mmake<CurveHandle>(supportSample)), 
        rightSupportHandle(mmake<CurveHandle>(supportSample)), topRangeHandle(mmake<CurveHandle>(rangeSample)),
        bottomRangeHandle(mmake<CurveHandle>(rangeSample)), curveEditor(editor)
    {
        mainHandle->SetDrawablesColor(color);
    }

    void CurvesEditor::KeyHandles::Draw(const RectF& camRect)
    {
        mainHandle->UpdateScreenPosition();

        if (leftSupportHandle->enabled)
        {
            leftSupportHandle->UpdateScreenPosition();
            o2Render.DrawAALine(mainHandle->GetScreenPosition(), leftSupportHandle->GetScreenPosition(), curveEditor.Lock()->mGridColor);
        }

        if (rightSupportHandle->enabled)
        {
            rightSupportHandle->UpdateScreenPosition();
            o2Render.DrawAALine(mainHandle->GetScreenPosition(), rightSupportHandle->GetScreenPosition(), curveEditor.Lock()->mGridColor);
        }

        mainHandle->Draw(camRect);
        leftSupportHandle->Draw(camRect);
        rightSupportHandle->Draw(camRect);
        topRangeHandle->Draw(camRect);
        bottomRangeHandle->Draw(camRect);
    }

    bool CurvesEditor::KeyHandles::IsSomeHandleSelected() const
    {
        return mainHandle->IsSelected() || leftSupportHandle->IsSelected() || rightSupportHandle->IsSelected();
    }

    bool CurvesEditor::CurveKeysInfo::operator==(const CurveKeysInfo& other) const
    {
        return curveId == other.curveId && keys == other.keys;
    }

    bool CurvesEditor::SelectedHandlesInfo::operator==(const SelectedHandlesInfo& other) const
    {
        return index == other.index && mainHandle == other.mainHandle && leftSupportHandle == other.leftSupportHandle &&
            rightSupportHandle == other.rightSupportHandle;
    }

    CurvesEditor::CurveHandle::CurveHandle()
    { }

    CurvesEditor::CurveHandle::CurveHandle(const Ref<Sprite>& regular, 
                                           const Ref<Sprite>& hover /*= nullptr*/, 
                                           const Ref<Sprite>& pressed /*= nullptr*/, 
                                           const Ref<Sprite>& selected /*= nullptr*/, 
                                           const Ref<Sprite>& selectedHovered /*= nullptr*/, 
                                           const Ref<Sprite>& selectedPressed /*= nullptr*/):
        DragHandle(regular, hover, pressed, selected, selectedHovered, selectedPressed)
    { }

    CurvesEditor::CurveHandle::CurveHandle(const CurveHandle& other) :
        DragHandle(other)
    { }

    CurvesEditor::CurveHandle::~CurveHandle()
    { }

    CurvesEditor::CurveHandle& CurvesEditor::CurveHandle::operator=(const CurveHandle& other)
    {
        DragHandle::operator=(other);
        return *this;
    }

    Vec2F CurvesEditor::CurveHandle::GetLocalPosition() const
    {
        return CurveViewToLocal(GetPosition());
    }

    Vec2F CurvesEditor::CurveHandle::LocalToCurveView(const Vec2F& point) const
    {
        return curveInfo->editor.Lock()->LocalToCurveView(point, curveInfo->viewScale, curveInfo->viewOffset);
    }

    Vec2F CurvesEditor::CurveHandle::CurveViewToLocal(const Vec2F& point) const
    {
        return curveInfo->editor.Lock()->CurveViewToLocal(point, curveInfo->viewScale, curveInfo->viewOffset);
    }

    Vec2F CurvesEditor::CurveHandle::ScreenToLocal(const Vec2F& point)
    {
        return LocalToCurveView(curveInfo->editor.Lock()->ScreenToLocalPoint(point));
    }

    Vec2F CurvesEditor::CurveHandle::LocalToScreen(const Vec2F& point)
    {
        return curveInfo->editor.Lock()->LocalToScreenPoint(CurveViewToLocal(point));
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::CurvesEditor>);
// --- META ---

DECLARE_CLASS(Editor::CurvesEditor, Editor__CurvesEditor);

DECLARE_CLASS(Editor::CurvesEditor::CurveHandle, Editor__CurvesEditor__CurveHandle);

DECLARE_CLASS(Editor::CurvesEditor::CurveCopyInfo, Editor__CurvesEditor__CurveCopyInfo);
// --- END META ---
