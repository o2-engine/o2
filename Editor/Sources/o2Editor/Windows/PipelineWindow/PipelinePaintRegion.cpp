#include "o2Editor/stdafx.h"
#include "PipelinePaintEditor.h"

#include "o2/Render/Render.h"
#include "o2/Render/Text.h"
#include "o2Editor/Pipeline/PipelineEditRegion.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"

// The region box of the drawing stage: the extract node's part boxes and the image edit node's optional frame

namespace Editor
{
    void PipelinePaintEditor::SetRegions(const Vector<RegionBox>& others, int selectedIndex, bool removable)
    {
        mOtherRegions = others;
        mSelectedIndex = selectedIndex;
        mRegionRemovable = removable;
    }

    RectF PipelinePaintEditor::BoxRect(float x, float y, float w, float h, const RectF& stage) const
    {
        float left = stage.left + stage.Width() * x;
        float top = stage.top - stage.Height() * y;
        return RectF(left, top, left + stage.Width() * w, top - stage.Height() * h);
    }

    const PipelinePaintEditor::RegionBox* PipelinePaintEditor::OtherRegionAt(const Vec2F& point) const
    {
        RectF stage = GetStageRect();
        for (int i = mOtherRegions.Count() - 1; i >= 0; i--)
        {
            auto& box = mOtherRegions[i];
            if (BoxRect(box.x, box.y, box.w, box.h, stage).IsInside(point))
                return &mOtherRegions[i];
        }
        return nullptr;
    }

    void PipelinePaintEditor::DrawBadge(const RectF& rect, int index, const Color4& color)
    {
        RectF badge(rect.left, rect.top, rect.left + 14.0f, rect.top - 12.0f);
        o2Render.DrawFilledPolygon({ badge.LeftBottom(), Vec2F(badge.left, badge.top), badge.RightTop(), Vec2F(badge.right, badge.bottom) }, color);
        mBadgeText->text = (String)index;
        mBadgeText->rect = badge;
        mBadgeText->Draw();
    }

    void PipelinePaintEditor::RemoveOptionalRegion()
    {
        if (!mNode)
            return;

        mNode->SetConfigString("drawTool", "brush");
        mNode->config[mRegionKey.Data()].SetNull();
        UpdateToolbar();
        if (onConfigChanged)
        {
            onConfigChanged("drawTool", true);
            onConfigChanged(mRegionKey, true);
        }
    }

    bool PipelinePaintEditor::ReadRegion(PipelineImageOps::CropRect& region) const
    {
        if (!mNode || !mWithRegion)
            return false;

        if (mOptionalRegion)
            return PipelineEditRegion::Read(mNode->GetConfigValue(mRegionKey), region);

        // A box over the whole image reads as no crop, yet it is the part's box
        auto value = mNode->GetConfigValue(mRegionKey);
        PipelineImageOps::ParseCrop(value, region);
        if (!value || !value->IsObject())
        {
            region.x = 0.1f; region.y = 0.1f; region.w = 0.8f; region.h = 0.8f;
        }
        return true;
    }

    bool PipelinePaintEditor::HasRegion() const
    {
        PipelineImageOps::CropRect region;
        return ReadRegion(region);
    }

    RectF PipelinePaintEditor::GetRegionRectangle() const
    {
        PipelineImageOps::CropRect region;
        if (!ReadRegion(region))
            return RectF();

        return BoxRect(region.x, region.y, region.w, region.h, GetStageRect());
    }

    void PipelinePaintEditor::SyncRegionFromConfig()
    {
        if (!mNode)
            return;

        PipelineImageOps::CropRect roi;
        if (!ReadRegion(roi))
            return;

        RectF stage = GetStageRect();
        float left = stage.left + stage.Width() * roi.x;
        float top = stage.top - stage.Height() * roi.y;
        float w = stage.Width() * roi.w;
        float h = stage.Height() * roi.h;
        mRegionSyncing = true;
        mRegionFrame->SetBasis(Basis(Vec2F(left, top - h), Vec2F(w, 0), Vec2F(0, h)));
        mRegionSyncing = false;
    }

    void PipelinePaintEditor::OnRegionTransformed(const Basis& basis)
    {
        if (mRegionSyncing || !mNode)
            return;

        RectF stage = GetStageRect();
        if (stage.Width() <= 0 || stage.Height() <= 0)
            return;

        RectF frame(basis.origin, basis.origin + basis.xv + basis.yv);
        float x = Math::Clamp((frame.left - stage.left) / stage.Width(), 0.0f, 1.0f);
        float right = Math::Clamp((frame.right - stage.left) / stage.Width(), 0.0f, 1.0f);
        float y = Math::Clamp((stage.top - frame.top) / stage.Height(), 0.0f, 1.0f);
        float bottom = Math::Clamp((stage.top - frame.bottom) / stage.Height(), 0.0f, 1.0f);

        // An optional region smaller than the minimum would read as none and vanish under the cursor
        float minSize = mOptionalRegion ? PipelineEditRegion::minSize : 0.005f;
        x = Math::Min(x, 1.0f - minSize);
        y = Math::Min(y, 1.0f - minSize);
        auto& roi = mNode->config[mRegionKey.Data()];
        roi.SetObject();
        roi["x"] = x;
        roi["y"] = y;
        roi["w"] = Math::Max(minSize, right - x);
        roi["h"] = Math::Max(minSize, bottom - y);

        if (onConfigChanged)
            onConfigChanged(mRegionKey, false);
    }

    void PipelinePaintEditor::OnRegionCompleted()
    {
        if (onConfigChanged)
            onConfigChanged(mRegionKey, true);
    }
}
