#include "o2Editor/stdafx.h"
#include "PipelineKitCard.h"

#include "o2/Render/Render.h"
#include "o2/Utils/System/Time/Time.h"
#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineDraw.h"
#include "o2Editor/Windows/PipelineWindow/PipelineImageTheme.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsPickers.h"

namespace Editor
{
    using namespace UiWsDraw;
    namespace Kit = PipelineKitLayout;

    namespace PipelineKitCards
    {
        String TargetKind(const UiDoc::UiTarget& target)
        {
            auto device = UiDoc::DeviceById(target.id);
            return device ? device->kind : String("custom");
        }

        String TargetName(const UiDoc::UiTarget& target)
        {
            return target.id.IsEmpty() ? String("Custom") : target.name;
        }

        String ScreenText(const UiDoc::UiScreen& screen)
        {
            return UiDoc::JsNum(screen.w) + "\xC3\x97" + UiDoc::JsNum(screen.h) + " \xC2\xB7 " + UiDoc::JsNum(UiDoc::JsRound(screen.dpi)) + " dpi";
        }

        String DpiText(double dpi)
        {
            return UiDoc::JsNum(UiDoc::JsRound(dpi*10.0)/10.0);
        }

        String TypeName(const String& type)
        {
            static const Map<String, String> names = {
                { "image", "picture" }, { "progressBar", "progress bar" }, { "progressFill", "progress fill" },
                { "sliderHandle", "slider knob" }, { "input", "input field" }, { "header", "header / ribbon" },
            };
            String name;
            return names.TryGetValue(type, name) ? name : type;
        }

        String AssetName(const String& asset)
        {
            return asset == "sliced" ? String("9-slice") : asset;
        }
    }

    // --- the grid -------------------------------------------------------------------------------

    PipelineKitGrid::PipelineKitGrid(RefCounter* refCounter):
        UiWsPanel(refCounter)
    {}

    void PipelineKitGrid::SetCells(const Vector<PipelineKitCell>& cells, const String& openId, bool canAdd)
    {
        mCells = cells;
        mOpen = openId;
        mCanAdd = canAdd;

        // A texture per picture shown, made again only when the picture changes
        Map<String, Pic> pics;
        for (auto& cell : mCells)
        {
            if (!cell.picture)
                continue;

            Pic pic;
            if (mPics.TryGetValue(cell.id, pic) && pic.bitmap == cell.picture)
            {
                pics[cell.id] = pic;
                continue;
            }

            pic.bitmap = cell.picture;
            pic.texture = TextureRef(*cell.picture);
            pic.size = cell.picture->GetSize();
            pics[cell.id] = pic;
        }
        mPics = pics;
    }

    PipelineKitLayout::Grid PipelineKitGrid::GetGrid() const
    {
        return Kit::Of(layout->GetWidth() + 2.0f*Kit::padX, mCells.Count());
    }

    RectF PipelineKitGrid::GetCellRect(const String& id) const
    {
        int index = id.IsEmpty() ? mCells.Count() : mCells.IndexOf([&](const PipelineKitCell& c) { return c.id == id; });
        if (index < 0)
            return RectF();

        auto g = GetGrid();
        auto b = Kit::CellInGrid(g, index);
        return Box(layout->GetWorldRect()).R(b.x, b.y, b.w, b.h);
    }

    RectF PipelineKitGrid::GetCellArea(const String& id) const
    {
        RectF picture = GetCellRect(id);
        auto g = GetGrid();
        return RectF(picture.left, picture.top, picture.right, picture.top - g.cellH);
    }

    bool PipelineKitGrid::IsUnderPoint(const Vec2F& point)
    {
        if (!Inside(layout->GetWorldRect(), point))
            return false;

        // A cell's top-left corner is its input's port: a press there starts or takes a link
        auto g = GetGrid();
        Box box(layout->GetWorldRect());
        for (int i = 0; i < mCells.Count(); i++)
        {
            auto b = Kit::CellInGrid(g, i);
            if ((box.P(b.x, b.y) - point).Length() <= 12.0f)
                return false;
        }
        return true;
    }

    void PipelineKitGrid::DrawPanel(const Box& box)
    {
        auto g = GetGrid();
        for (int i = 0; i < mCells.Count(); i++)
            DrawCell(box, g, i, mCells[i]);

        if (mCanAdd)
            DrawAddCell(box, g);

        // No inputs yet: a hint beside the add cell
        if (mCells.IsEmpty() && g.cols > 1 && !PipelineControls::IsFarView())
        {
            TextStyle hint(11.5f, 400, T().fg3);
            hint.wrap = true;
            hint.lineHeight = 17.0f;
            float left = g.cell + Kit::cellGap + 6.0f;
            Text("Add pictures (buttons, panels, frames, icons, bars) and text styles: each becomes a cell of the kit. Click a cell "
                 "for its settings", box.R(left, 0, box.W() - left, g.cell), hint);
        }
    }

    void PipelineKitGrid::DrawCell(const Box& box, const PipelineKitLayout::Grid& g, int index, const PipelineKitCell& cell)
    {
        auto& t = T();
        bool far = PipelineControls::IsFarView();
        auto b = Kit::CellInGrid(g, index);
        RectF rect = box.R(b.x, b.y, b.w, b.h);
        String hot = "cell:" + cell.id, removeHot = "remove:" + cell.id;
        bool open = cell.id == mOpen;
        bool hover = IsHover(hot) || IsHover(removeHot);

        PipelineImageTheme::DrawCellPlate(rect, open, !far);
        RectF inner(rect.left + 1.0f, rect.top - 1.0f, rect.right - 1.0f, rect.bottom + 1.0f);
        RectF pad(rect.left + 4.0f, rect.top - 4.0f, rect.right - 4.0f, rect.bottom + 4.0f);
        auto fit = [&](const Vec2I& size)
        {
            float k = Math::Min(pad.Width()/Math::Max(1, size.x), pad.Height()/Math::Max(1, size.y));
            Vec2F c = pad.Center(), half(size.x*k*0.5f, size.y*k*0.5f);
            return RectF(c.x - half.x, c.y + half.y, c.x + half.x, c.y - half.y);
        };

        Pic pic;
        bool hasPic = mPics.TryGetValue(cell.id, pic);
        if (cell.style)
        {
            // "Aa" in the style's look on the sample's dark paper
            Fill(inner, 6.0f, Color4(0x20, 0x23, 0x2B, 255));
            if (hasPic)
                Picture(pic.texture, pic.size, fit(pic.size));
        }
        else if (cell.element)
            KitThumb(*cell.element, pad, 3.0f, false);
        else if (hasPic)
            Picture(pic.texture, pic.size, fit(pic.size));
        else if (!far)
            Text((String)(index + 1), rect, TextStyle(12.0f, 500, t.fg3, HorAlign::Middle));

        if (far)
        {
            AddHot(box.R(b.x, b.y, b.w, g.cellH), hot);
            return;
        }

        if (cell.busy)
        {
            Fill(inner, 6.0f, Color4(0, 0, 0, 89));
            Vec2F center = rect.Center();
            float radius = 8.0f, turn = 0.8f;
            float phase = Math::Mod((float)o2Time.GetApplicationTime(), turn)/turn;
            o2Render.DrawAACircle(center, radius, Color4(255, 255, 255, 64), 32, Math::Max(1.0f, 2.0f/Math::Max(0.0001f, o2Render.GetCamera().GetScale2D().x)));
            Vector<Vec2F> arc;
            float start = Math::PI()*0.25f - phase*Math::PI()*2.0f;
            for (int i = 0; i <= 8; i++)
            {
                float angle = start + Math::PI()*0.5f*(float)i/8.0f;
                arc.Add(center + Vec2F(Math::Cos(angle), Math::Sin(angle))*radius);
            }
            o2Render.DrawAALine(arc, t.accent, 2.0f);
        }

        if (cell.ai)
            Icon("uiws_sparkle", Vec2F(rect.right - 4.0f - 5.5f, rect.bottom + 3.0f + 5.5f), 11.0f, t.accent);
        if (cell.soft)
            Icon("uiws_warn", Vec2F(rect.left + 4.0f + 5.5f, rect.bottom + 3.0f + 5.5f), 11.0f, t.yellow);

        float alpha = cell.linked ? 1.0f : 0.55f;
        TextStyle name(10.5f, 500, A(t.fg, alpha), HorAlign::Middle);
        Text(cell.name, box.R(b.x, b.y + b.h + Kit::capGap, b.w, Kit::nameH), name);
        TextStyle meta(9.5f, 400, A(t.fg3, alpha), HorAlign::Middle);
        Text(cell.meta, box.R(b.x + 2.0f, b.y + b.h + Kit::capGap + Kit::nameH, b.w - 4.0f, Kit::metaH), meta);

        AddHot(box.R(b.x, b.y, b.w, g.cellH), hot);

        // The bin on the top-right corner while the cell is hovered: the input's port has the top-left one
        if (hover)
        {
            RectF tool(rect.right - 21.0f, rect.top - 1.0f, rect.right - 1.0f, rect.top - 19.0f);
            PipelineDraw::Corners corners;
            corners.rt = 6.0f;
            corners.lb = 7.0f;
            PipelineDraw::Fill(tool, corners, IsHover(removeHot) ? t.red : Color4(0, 0, 0, 158));
            Icon("uiws_trash", tool.Center(), 10.0f, Color4::White());
            AddHot(tool, removeHot);
        }
    }

    void PipelineKitGrid::DrawAddCell(const Box& box, const PipelineKitLayout::Grid& g)
    {
        if (PipelineControls::IsFarView())
            return;

        auto& t = T();
        auto b = Kit::CellInGrid(g, mCells.Count());
        RectF rect = box.R(b.x, b.y, b.w, b.h);
        bool hover = IsHover("add");
        Color4 color = hover ? t.accent : t.fg3;
        DashedFrame(rect, hover ? t.accent : t.border, 4.0f, 3.0f);
        Text("+", rect, TextStyle(26.0f, 300, color, HorAlign::Middle));
        Text("+ input", box.R(b.x, b.y + b.h + Kit::capGap, b.w, Kit::nameH), TextStyle(10.5f, 500, t.fgDim, HorAlign::Middle));
        Text("picture \xC2\xB7 style", box.R(b.x, b.y + b.h + Kit::capGap + Kit::nameH, b.w, Kit::metaH), TextStyle(9.5f, 400, t.fg3, HorAlign::Middle));
        AddHot(box.R(b.x, b.y, b.w, g.cellH), "add");
    }

    void PipelineKitGrid::OnHotClick(const String& id, const Input::Cursor& cursor)
    {
        if (id.StartsWith("remove:"))
        {
            if (onRemove)
                onRemove(id.SubStr(7));
        }
        else if (id.StartsWith("cell:"))
        {
            if (onOpen)
                onOpen(id.SubStr(5));
        }
        else if (id == "add" && onAdd)
            onAdd();
    }
}
