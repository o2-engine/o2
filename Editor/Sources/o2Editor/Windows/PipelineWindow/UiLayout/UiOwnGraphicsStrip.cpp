#include "o2Editor/stdafx.h"
#include "UiOwnGraphicsStrip.h"

#include <cmath>
#include <map>

#include "o2/Assets/Types/FontAsset.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocOwn.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineImageTheme.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"

namespace Editor
{
    using namespace PipelineControls;

    UiOwnStripGeometry::UiOwnStripGeometry(float rowWidth, int n):
        count(n)
    {
        if (n <= 0)
            return;

        cols = Math::Max(1, Math::Min(n, (int)std::floor((rowWidth + gap)/(cellMin + gap))));
        cell = Math::Max(24.0f, Math::Min(cellMax, std::floor((rowWidth - gap*(cols - 1))/cols)));
        rows = (n + cols - 1)/cols;
        height = rows*(cell + captionGap + caption) + (rows - 1)*gap;
    }

    Vec2F UiOwnStripGeometry::Corner(int index) const
    {
        int col = index % Math::Max(1, cols), row = index/Math::Max(1, cols);
        return Vec2F(col*(cell + gap) + cell, row*(cell + captionGap + caption + gap));
    }

    Vector<UiOwnCell> UiOwnCellsOf(const PipelineNode& node)
    {
        Vector<UiOwnCell> cells;
        bool any = node.outputs.Any([](const PipelinePort& p) { return !UiDoc::IsLayoutFixedOutput(p.name); });
        if (!any)
            return cells;

        auto own = UiDoc::LayoutOwnPorts(UiDoc::FromDataValue(node.config));
        for (auto& port : node.outputs)
        {
            if (UiDoc::IsLayoutFixedOutput(port.name))
                continue;

            String lower = port.name.ToLowerCase();
            const UiDoc::LayoutOwnPort* found = own.Find([&](const UiDoc::LayoutOwnPort& o) { return o.name.ToLowerCase() == lower; });
            if (!found)
                continue;

            const auto& a = found->asset;
            String kind = a.assetType == "sliced" ? String("9-slice") : a.assetType;
            String made = a.from && *a.from == "cut" ? String("cut from the reference") : a.from && *a.from == "drawn" ? String("drawn by the image model") : String();
            String about = a.type + " \xC2\xB7 " + kind + " \xC2\xB7 " + UiDoc::JsNum(a.width) + "\xC3\x97" + UiDoc::JsNum(a.height);
            if (!made.IsEmpty())
                about += "\n" + made;
            cells.Add(UiOwnCell{ port.id, port.name, a.uploadId, about });
        }
        return cells;
    }

    // The thumbnail of an upload, decoded once
    static Ref<Bitmap> UploadPicture(const String& uploadId)
    {
        static auto* cache = new std::map<std::string, Ref<Bitmap>>();
        auto it = cache->find(uploadId.Data());
        if (it != cache->end())
            return it->second;

        if (cache->size() > 200)
            cache->clear();

        Ref<Bitmap> bitmap;
        String bytes = PipelineUtils::ReadFileBytes(PipelineUtils::GetUploadPath(uploadId));
        UiLayout::UiImage image;
        if (!bytes.IsEmpty() && UiLayout::UiImage::Decode(bytes, image) && !image.IsEmpty())
            bitmap = image.ToBitmap();
        (*cache)[uploadId.Data()] = bitmap;
        return bitmap;
    }

    UiOwnGraphicsStrip::UiOwnGraphicsStrip(RefCounter* refCounter):
        Widget(refCounter)
    {}

    void UiOwnGraphicsStrip::SetCells(const Vector<UiOwnCell>& cells)
    {
        if (cells == mCells && mWidgets.Count() == cells.Count())
            return;

        PushEditorScopeOnStack scope;
        mCells = cells;
        for (auto& w : mWidgets)
            RemoveChild(w.root);
        mWidgets.Clear();

        auto& theme = PipelineTheme::Get();
        for (auto& c : mCells)
        {
            Cell cell;
            cell.root = mmake<Widget>();
            cell.root->name = "own cell " + c.name;
            cell.image = mmake<PipelineCellImage>();
            cell.image->name = "own picture";
            cell.image->SetHint("\xC2\xB7");
            cell.image->SetBitmap(UploadPicture(c.uploadId));
            cell.root->AddChild(cell.image);
            cell.label = MakeLabel(c.name, true);
            cell.label->SetFontAsset(AssetRef<FontAsset>(PipelineTheme::MonoFont()));
            cell.label->height = PipelineTheme::Size(9.5f);
            cell.label->color = theme.fgDim;
            cell.label->horOverflow = Label::HorOverflow::Dots;
            cell.label->horAlign = HorAlign::Middle;
            cell.label->verAlign = VerAlign::Middle;
            cell.root->AddChild(cell.label);
            AddChild(cell.root);
            mWidgets.Add(cell);
        }
        LayoutCells();
    }

    int UiOwnGraphicsStrip::GetCellIndex(const String& portId) const
    {
        return mCells.IndexOf([&](const UiOwnCell& c) { return c.portId == portId; });
    }

    bool UiOwnGraphicsStrip::GetCellCorner(int index, Vec2F& corner) const
    {
        if (index < 0 || index >= mWidgets.Count() || mWidgets[index].image->layout->GetWidth() <= 0.0f)
            return false;

        RectF rect = mWidgets[index].image->layout->GetWorldRect();
        corner = Vec2F(Math::Max(rect.left, rect.right), Math::Max(rect.top, rect.bottom));
        return true;
    }

    bool UiOwnGraphicsStrip::HasPicture(int index) const
    {
        return index >= 0 && index < mWidgets.Count() && mWidgets[index].image->HasImage();
    }

    void UiOwnGraphicsStrip::UpdateSelfTransform()
    {
        Widget::UpdateSelfTransform();
        LayoutCells();
    }

    void UiOwnGraphicsStrip::LayoutCells()
    {
        float width = layout->GetWidth();
        if (width <= 0.0f || mWidgets.IsEmpty())
            return;

        UiOwnStripGeometry g(width, mWidgets.Count());
        for (int i = 0; i < mWidgets.Count(); i++)
        {
            auto& cell = mWidgets[i];
            Vec2F corner = g.Corner(i);
            float x = corner.x - g.cell, y = corner.y;
            *cell.root->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(x, -(y + g.cell + g.captionGap + g.caption)), Vec2F(x + g.cell, -y));
            *cell.image->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, g.cell, 0);
            *cell.label->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 0, 0, g.caption, 0);
        }
    }
}
