#include "o2Editor/stdafx.h"
#include "PipelineKitCard.h"

#include <cstdlib>

#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Pipeline/PipelinePngMeta.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsPickers.h"

namespace Editor
{
    using namespace UiWsDraw;
    using UiDoc::Json;

    namespace
    {
        const float pad = 10.0f, headH = 28.0f, rowH = 26.0f, rowGap = 6.0f, labelW = 76.0f, sourceH = 170.0f;

        String JsonText(const Json& item, const char* key)
        {
            auto value = item.Get(key);
            return value && !value->IsNull() ? value->ToJsString() : String();
        }

        // A name the layout can refer to: letters, digits, _ and -
        String CleanName(const String& text)
        {
            String clean;
            for (auto c : text)
            {
                if (isalnum((unsigned char)c) || c == '_' || c == '-')
                    clean += c;
            }
            return clean;
        }
    }

    PipelineKitPopup::PipelineKitPopup(RefCounter* refCounter):
        UiWsPanel(refCounter)
    {}

    void PipelineKitPopup::Draw()
    {
        // Drawn above the cards and links only: the body's own pass skips it
        if (!overlay)
            return;

        UiWsPanel::Draw();
    }

    void PipelineKitPopup::OnKeyPressed(const Input::Key& key)
    {
        auto self = body.Lock();
        auto editor = self ? self->GetEditor() : nullptr;
        if (key.keyCode != VK_ESCAPE || !self || !IsEnabled() || HasFocusedField())
            return;

        if (editor && editor->GetOptionPicker() && editor->GetOptionPicker()->IsOpen())
            return;

        self->OpenCell("");
    }

    void PipelineKitPopup::OnCursorPressedOutside(const Input::Cursor& cursor)
    {
        auto self = body.Lock();
        auto editor = self ? self->GetEditor() : nullptr;
        if (!self || !self->GetGrid() || self->GetOpenCell() != portId)
            return;

        // The menus it opens (the type list) live outside it
        if (editor && editor->GetOptionPicker() && editor->GetOptionPicker()->IsOpen())
            return;

        if (UiWsDraw::Inside(self->GetGrid()->GetCellArea(portId), cursor.position))
            return;

        self->OpenCell("");
    }

    bool PipelineKitPopup::HasFocusedField() const
    {
        for (auto& kv : mFields)
        {
            if (kv.second->IsFocused())
                return true;
        }
        return false;
    }

    Ref<EditBox> PipelineKitPopup::GetField(const String& key) const
    {
        Ref<EditBox> field;
        mFields.TryGetValue(key, field);
        return field;
    }

    Ref<EditBox> PipelineKitPopup::AddText(const String& key, const String& text, const String& placeholder, bool multiline,
                                           const Function<void(const String&)>& onDone)
    {
        // Applies when the field is left (or Enter on one line): a change only, so a field left as it was writes nothing
        String shown = text;
        auto field = UiWsControls::TextField(text, placeholder, multiline, {}, [onDone, shown](const String& value)
        {
            if (value != shown && onDone && !o2Input.IsKeyDown(VK_ESCAPE))
                onDone(value);
        });
        field->name = "kit " + key;
        AddChild(field);
        mFields[key] = field;
        return field;
    }

    Ref<EditBox> PipelineKitPopup::AddNumber(const String& key, const String& letter, std::optional<double> value, double min,
                                             const Function<void(std::optional<double>)>& onValue)
    {
        // Applies on Enter or when the field is left, only a change; Escape gives the field its number back (AssetsLine
        // DraftNumber lazy)
        auto field = UiWsControls::NumberField(letter, value, "", {}, min);
        field->name = "kit " + key;
        String shown = value ? UiWsControls::NumberText(*value) : String();
        field->onChangeCompleted = [onValue, shown, min](const WString& text)
        {
            // Escape drops what was typed: held while the field is left (a menu closing on it takes the focus away first)
            String s = ((String)text).Trimed(" \t");
            if (s == shown || !onValue || o2Input.IsKeyDown(VK_ESCAPE))
                return;

            if (s.IsEmpty())
            {
                onValue(std::nullopt);
                return;
            }

            char* end = nullptr;
            double v = std::strtod(s.Data(), &end);
            if (end && *end == '\0' && std::isfinite(v) && v >= min)
                onValue(v);
        };
        AddChild(field);
        mFields[key] = field;
        return field;
    }

    void PipelineKitPopup::Field(const String& key, const RectF& rect)
    {
        Ref<EditBox> field;
        if (!mFields.TryGetValue(key, field))
            return;

        RectF world = layout->GetWorldRect();
        float left = rect.left - world.left, top = world.top - rect.top;
        WidgetLayout next(Vec2F(0, 1), Vec2F(0, 1), Vec2F(left, -(top + rect.Height())), Vec2F(left + rect.Width(), -top));
        if (field->layout->GetOffsetMin() != next.GetOffsetMin() || field->layout->GetOffsetMax() != next.GetOffsetMax())
            *field->layout = next;
    }

    void PipelineKitPopup::Rebuild()
    {
        PushEditorScopeOnStack scope;
        for (auto& kv : mFields)
            RemoveChild(kv.second);
        mFields.Clear();

        auto self = body.Lock();
        auto port = self ? self->GetNode()->FindInput(portId) : nullptr;
        if (!port)
            return;

        WeakRef<PipelineKitPopup> weakThis(this);
        String id = portId;
        auto set = [weakThis, id](const Vector<Pair<String, Json>>& fields, const Vector<String>& removed)
        {
            auto me = weakThis.Lock();
            auto owner = me ? me->body.Lock() : nullptr;
            if (owner)
                owner->SetItemFields(id, fields, removed);
        };

        auto item = self->ItemOf(portId);
        int index = self->GetNode()->inputs.IndexOf([&](const PipelinePort& p) { return p.id == id; });
        bool isStyle = port->portType == PipelinePortType::TextStyle;
        String placeholder = isStyle ? String("style") : self->NameOf(*port, index);
        if (isStyle)
        {
            auto value = self->InputValue(portId);
            if (value.type == PipelinePortType::TextStyle)
            {
                Json raw;
                Json::Parse(value.data, raw);
                placeholder = UiDoc::NormalizeTextStyle(raw).name;
            }
        }
        AddText("name", JsonText(item, "name"), placeholder, false, [set](const String& v) { set({ { "name", Json(CleanName(v)) } }, {}); });

        if (!isStyle)
        {
            AddText("variant", JsonText(item, "variant"), "variant", false, [set](const String& v) { set({ { "variant", Json(v) } }, {}); });

            // The size on the device and the texture's own density
            auto el = self->ElementOf(portId);
            auto size = item.Get("size");
            std::optional<UiDoc::UiPair> typed;
            if (size && size->IsObject() && size->Get("w") && size->Get("h") && size->Get("w")->Num() > 0 && size->Get("h")->Num() > 0)
                typed = UiDoc::UiPair{ size->Get("w")->Num(), size->Get("h")->Num() };
            auto now = typed ? typed : el ? el->inches : std::nullopt;
            AddNumber("sizeW", "W", now ? std::optional<double>(now->w) : std::nullopt, 0.005,
                      [weakThis](std::optional<double> v) { if (auto me = weakThis.Lock()) me->SetSide(true, v); });
            AddNumber("sizeH", "H", now ? std::optional<double>(now->h) : std::nullopt, 0.005,
                      [weakThis](std::optional<double> v) { if (auto me = weakThis.Lock()) me->SetSide(false, v); });
            if (auto own = UiDoc::CleanDpi(item.Get("dpi")))
            {
                AddNumber("dpi", "", *own, 36.0, [set](std::optional<double> v)
                {
                    auto dpi = v ? UiDoc::CleanDpi(*v) : std::nullopt;
                    if (dpi)
                        set({ { "dpi", Json(*dpi) } }, {});
                });
            }

            // The borders typed, in input pixels
            String asset = JsonText(item, "assetType");
            String type = JsonText(item, "type");
            if (type.IsEmpty())
                type = UiDoc::GuessElementType(self->NameOf(*port, index));
            if (asset.IsEmpty())
                asset = UiDoc::DefaultAssetType(type);
            if (asset == "sliced")
            {
                auto setNine = UiDoc::NormalizeNine(item.Get("nineSlice"));
                std::optional<UiDoc::UiNine> detected;
                if (el && el->source.IsObject() && el->source.Get("detected"))
                    detected = UiDoc::NormalizeNine(el->source.Get("detected"));
                auto nine = setNine ? setNine : detected;
                const char* keys[] = { "nineL", "nineT", "nineR", "nineB" };
                const char* letters[] = { "L", "T", "R", "B" };
                for (int side = 0; side < 4; side++)
                {
                    std::optional<double> value;
                    if (nine)
                        value = side == 0 ? nine->left : side == 1 ? nine->top : side == 2 ? nine->right : nine->bottom;
                    auto base = setNine ? *setNine : detected.value_or(UiDoc::UiNine());
                    AddNumber(keys[side], letters[side], value, 0.0, [set, base, side](std::optional<double> v)
                    {
                        auto n = base;
                        double px = Math::Max(0.0, UiDoc::JsRound(v.value_or(0.0)));
                        (side == 0 ? n.left : side == 1 ? n.top : side == 2 ? n.right : n.bottom) = px;
                        set({ { "nineSlice", UiDoc::NineToJson(n) } }, { "nineFrom" });
                    });
                }
            }

            // A purpose typed here is the user's: the AI describe never replaces it
            AddText("purpose", JsonText(item, "purpose"), "Where to use it, e.g. background of popups and dialogs, backing of the main "
                    "action button", true, [set](const String& v) { set({ { "purpose", Json(v.ReplacedAll("\n", " ")) } }, { "purposeFrom" }); });
            AddText("description", JsonText(item, "description"), "What it is and what it looks like, e.g. glossy green button with a "
                    "gold rim", true, [set](const String& v) { set({ { "description", Json(v) } }, {}); });

            // The source with its trim box and the guides
            if (!mGuides)
            {
                mGuides = mmake<PipelineKitGuides>();
                mGuides->name = "kit source";
                AddChild(mGuides);
            }
            String bg = JsonText(item, "bg");
            auto value = self->InputValue(portId);
            String sourcePng = value.IsImage() ? value.GetPngBytes() : String();
            auto prepared = item.Get("prepared");
            String uploadId = prepared && prepared->IsObject() && prepared->Get("uploadId") ? prepared->Get("uploadId")->ToJsString() : String();
            if (bg == "ai" && !uploadId.IsEmpty())
            {
                String bytes = PipelineUtils::ReadFileBytes(PipelineUtils::GetUploadPath(uploadId));
                if (!bytes.IsEmpty())
                    sourcePng = bytes;
            }
            int sw = 0, sh = 0;
            if (!sourcePng.IsEmpty() && PipelinePngMeta::Size(sourcePng, sw, sh))
                mGuides->SetBitmap(DecodeImageBytes(sourcePng));
            else
                mGuides->SetHint("connect a picture");
            if (el && el->source.IsObject())
            {
                sw = (int)el->source.Get("w")->Num(sw);
                sh = (int)el->source.Get("h")->Num(sh);
            }
            auto setNine = UiDoc::NormalizeNine(item.Get("nineSlice"));
            std::optional<UiDoc::UiNine> detected;
            if (el && el->source.IsObject() && el->source.Get("detected"))
                detected = UiDoc::NormalizeNine(el->source.Get("detected"));
            mGuides->SetGuides(sw, sh, el && el->source.IsObject() && el->source.Get("trim") ? *el->source.Get("trim") : Json(),
                               asset == "sliced" ? (setNine ? setNine : detected) : std::nullopt, !setNine.has_value());
            mGuides->onSet = [set](const UiDoc::UiNine& n) { set({ { "nineSlice", UiDoc::NineToJson(n) } }, { "nineFrom" }); };
        }
        else if (mGuides)
        {
            RemoveChild(mGuides);
            mGuides = nullptr;
        }

        // Fields placed at once: the first frame draws them where they belong
        Box box(layout->GetWorldRect());
        mHeight = Lay(box, false);
    }

    void PipelineKitPopup::Place(const RectF& cell, const RectF& grid, const RectF& visible)
    {
        auto self = body.Lock();
        auto editor = self ? self->GetEditor() : nullptr;
        if (!self || !editor)
            return;

        // Beside the whole grid, level with the cell: to its right when that fits, else to its left, else wherever the view
        // has room - the popup covers none of the other cells, so another one can still be clicked to move it there
        float unit = Math::Max(0.0001f, editor->GetCamera().GetScale2D().x), margin = 8.0f*unit;
        float w = width, h = mHeight;
        float right = grid.right + gapToCell, left = grid.left - gapToCell - w;
        float x = right + w <= visible.right - margin ? right : left >= visible.left + margin ? left :
            Math::Max(visible.left + margin, Math::Min(right, visible.right - w - margin));
        float top = Math::Min(visible.top - margin, Math::Max(cell.top + 12.0f, visible.bottom + margin + h));

        RectF parent = self->layout->GetWorldRect();
        WidgetLayout next = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(w, h), Vec2F(x - parent.left, top - parent.top));
        if (layout->GetOffsetMin() != next.GetOffsetMin() || layout->GetOffsetMax() != next.GetOffsetMax())
            *layout = next;
    }

    void PipelineKitPopup::DrawPanel(const Box& box)
    {
        float height = Lay(box, true);
        if (!Math::Equals(height, mHeight))
            mHeight = height;
    }

    float PipelineKitPopup::Lay(const Box& box, bool draw)
    {
        auto self = body.Lock();
        auto port = self ? self->GetNode()->FindInput(portId) : nullptr;
        if (!port)
            return mHeight;

        if (draw)
            Island(box.world, 10.0f, false);

        float y = pad;
        if (port->portType == PipelinePortType::TextStyle)
            y = LayStyle(box, draw, y);
        else
            y = LayElement(box, draw, y);
        return y + pad;
    }

    float PipelineKitPopup::LayElement(const Box& box, bool draw, float y)
    {
        auto& t = T();
        auto self = body.Lock();
        auto port = self->GetNode()->FindInput(portId);
        int index = self->GetNode()->inputs.IndexOf([&](const PipelinePort& p) { return p.id == portId; });
        auto item = self->ItemOf(portId);
        auto el = self->ElementOf(portId);
        float W = box.W(), inner = W - pad*2.0f;
        String name = self->NameOf(*port, index);
        String type = JsonText(item, "type"), asset = JsonText(item, "assetType"), bg = JsonText(item, "bg");
        if (type.IsEmpty())
            type = UiDoc::GuessElementType(name);
        if (asset.IsEmpty())
            asset = UiDoc::DefaultAssetType(type);
        if (bg.IsEmpty())
            bg = "keep";

        // The head: the name, the type, the close button
        float typeW = 140.0f, nameW = inner - typeW - 6.0f - 26.0f - 6.0f;
        Field("name", box.R(pad, y, nameW, headH));
        RectF typeRect = box.R(pad + nameW + 6.0f, y, typeW, headH);
        if (draw)
        {
            Fill(typeRect, 7.0f, t.field);
            Border(typeRect, 7.0f, IsHover("type") ? t.accent : t.line);
            Text(PipelineKitCards::TypeName(type), RectF(typeRect.left + 9.0f, typeRect.top, typeRect.right - 24.0f, typeRect.bottom), TextStyle(11.5f, 400, t.fg));
            Icon("uiws_chevron_down", Vec2F(typeRect.right - 13.0f, typeRect.Center().y), 11.0f, t.fg3);
            Tool(box.R(W - pad - 26.0f, y + 1.0f, 26.0f, 26.0f), "uiws_close", "close", false, true, 12.0f);
        }
        AddHot(typeRect, "type");
        y += headH + 8.0f;

        // The source with the trim box and the guides, its caption
        if (mGuides)
        {
            RectF area = box.R(pad, y, inner, sourceH);
            RectF world = layout->GetWorldRect();
            float left = area.left - world.left, top = world.top - area.top;
            WidgetLayout next(Vec2F(0, 1), Vec2F(0, 1), Vec2F(left, -(top + sourceH)), Vec2F(left + inner, -top));
            if (mGuides->layout->GetOffsetMin() != next.GetOffsetMin() || mGuides->layout->GetOffsetMax() != next.GetOffsetMax())
                *mGuides->layout = next;
        }
        y += sourceH + 3.0f;
        if (draw)
        {
            String caption = "source";
            if (asset == "sliced")
                caption += item.Get("nineSlice") && !item.Get("nineSlice")->IsNull() ?
                    (JsonText(item, "nineFrom") == "ai" ? " \xC2\xB7 nine-slice measured by AI" : " \xC2\xB7 nine-slice set by hand") :
                    " \xC2\xB7 nine-slice found automatically";
            String meta;
            if (el)
            {
                meta = "in the kit \xC2\xB7 " + UiDoc::JsNum(el->width) + "\xC3\x97" + UiDoc::JsNum(el->height) + " \xC2\xB7 " +
                    UiDoc::JsNum(UiDoc::JsRound(UiDoc::GraphicDpi(el->dpi))) + " dpi";
                if (el->source.IsObject() && el->source.Get("collapsed") && el->source.Get("collapsed")->Bool())
                    meta += " \xC2\xB7 middle shrunk";
            }
            float metaW = Math::Min(inner*0.6f, TextWidth(meta, 10.5f, 400, true));
            Text(caption, box.R(pad, y, inner - metaW - 8.0f, 14.0f), TextStyle(10.5f, 400, t.fgDim));
            Text(meta, box.R(W - pad - metaW, y, metaW, 14.0f), TextStyle(10.5f, 400, t.fg3, HorAlign::Right, true));
        }
        y += 14.0f + 8.0f;

        auto row = [&](const String& label, float h = rowH)
        {
            if (draw)
                Text(label, box.R(pad, y, labelW - 6.0f, rowH), TextStyle(11.0f, 400, t.fgDim));
            RectF r = box.R(pad + labelW, y, inner - labelW, h);
            y += h + rowGap;
            return r;
        };
        auto toggle = [&](const RectF& r, bool on, const String& caption, const String& hot)
        {
            float w = 32.0f + 8.0f + TextWidth(caption, 11.0f) + 2.0f;
            RectF area(r.left, r.top, r.left + w, r.bottom);
            if (draw)
            {
                UiWsParts::Toggle(area, on, IsHover(hot));
                Text(caption, RectF(r.left + 40.0f, r.top, r.right, r.bottom), TextStyle(11.0f, 400, t.fg));
            }
            AddHot(area, hot);
        };

        RectF r = row("Asset");
        if (draw)
            Segments(r, { { "simple", "simple" }, { "sliced", "9-slice" }, { "tiled", "tiled" } }, asset, "asset:");
        r = row("Variant");
        Field("variant", r);
        r = row("Background");
        if (draw)
            Segments(r, { { "keep", "keep" }, { "key", "cut plain" }, { "ai", "AI" } }, bg, "bg:");
        r = row("Margins");
        bool trim = !(item.Get("trim") && item.Get("trim")->IsBool() && !item.Get("trim")->Bool());
        toggle(r, trim, "cut transparent", "trim");

        y = LayDevice(box, draw, y);

        if (asset == "sliced")
        {
            r = row("9-slice");
            bool typedNine = item.Get("nineSlice") && !item.Get("nineSlice")->IsNull();
            float autoW = 46.0f, fieldW = (r.Width() - autoW - 6.0f - 3*4.0f)/4.0f;
            const char* keys[] = { "nineL", "nineT", "nineR", "nineB" };
            for (int side = 0; side < 4; side++)
                Field(keys[side], RectF(r.left + side*(fieldW + 4.0f), r.top, r.left + side*(fieldW + 4.0f) + fieldW, r.bottom));
            if (draw)
                Button(RectF(r.right - autoW, r.top, r.right, r.bottom), "auto", "nineAuto", false, typedNine);
            r = row("Middle");
            bool optimize = !(item.Get("optimize") && item.Get("optimize")->IsBool() && !item.Get("optimize")->Bool());
            toggle(r, optimize, "shrink a uniform stretch", "middle");
        }

        if (bg == "ai" && !(item.Get("prepared") && item.Get("prepared")->IsObject()))
        {
            String note = "Run AI with \"remove backgrounds\" on to make the picture without its background";
            TextStyle style(11.0f, 400, t.fgDim);
            style.wrap = true;
            style.lineHeight = 15.0f;
            float h = TextHeight(note, inner, style);
            if (draw)
                Text(note, box.R(pad, y, inner, h), style);
            y += h + rowGap;
        }

        // What it is for, what it is
        if (draw)
            Text("Purpose", box.R(pad, y, inner, 16.0f), TextStyle(11.0f, 400, t.fgDim));
        y += 16.0f + 3.0f;
        Field("purpose", box.R(pad, y, inner, 44.0f));
        y += 44.0f + rowGap;
        if (draw)
            Text("Description", box.R(pad, y, inner, 16.0f), TextStyle(11.0f, 400, t.fgDim));
        y += 16.0f + 3.0f;
        Field("description", box.R(pad, y, inner, 50.0f));
        y += 50.0f + 8.0f;

        // The foot: the AI for this element, remove it
        if (draw)
        {
            Line(box.P(0, y), box.P(W, y), t.line);
            bool busy = self->GetGrid() && self->GetGrid()->GetCells().Any([&](const PipelineKitCell& c) { return c.id == portId && c.busy; });
            Button(box.R(pad, y + 8.0f, 150.0f, 26.0f), "AI: this element", "ai", false, !busy, "uiws_sparkle");
            Button(box.R(W - pad - 100.0f, y + 8.0f, 100.0f, 26.0f), "Remove", "remove", false, true, "uiws_trash");
        }
        AddHot(box.R(pad, y + 8.0f, 150.0f, 26.0f), "ai");
        AddHot(box.R(W - pad - 100.0f, y + 8.0f, 100.0f, 26.0f), "remove");
        return y + 8.0f + 26.0f;
    }

    void PipelineKitPopup::PickType()
    {
        auto self = body.Lock();
        auto editor = self ? self->GetEditor() : nullptr;
        auto port = self ? self->GetNode()->FindInput(portId) : nullptr;
        if (!editor || !port)
            return;

        auto item = self->ItemOf(portId);
        int index = self->GetNode()->inputs.IndexOf([&](const PipelinePort& p) { return p.id == portId; });
        String type = JsonText(item, "type");
        if (type.IsEmpty())
            type = UiDoc::GuessElementType(self->NameOf(*port, index));

        PipelineOptionPickerRequest request;
        for (auto& option : UiDoc::UiElementTypes())
            request.options.Add(PipelineOption{ option, PipelineKitCards::TypeName(option), UiDoc::UiElementHint(option), "" });
        request.current = type;
        RectF world = HotRect("type");
        Vec2F a = editor->LocalToScreenPoint(Vec2F(world.left, world.bottom));
        Vec2F b = editor->LocalToScreenPoint(Vec2F(world.right, world.top));
        request.anchor = RectF(Math::Min(a.x, b.x), Math::Max(a.y, b.y), Math::Max(a.x, b.x), Math::Min(a.y, b.y));
        request.field = Ref<Widget>(this);
        bool assetSet = item.Get("assetType") && !item.Get("assetType")->IsNull();
        WeakRef<PipelineKitPopup> weakThis(this);
        String id = portId;
        request.onPick = [weakThis, id, assetSet](const String& picked)
        {
            auto me = weakThis.Lock();
            auto owner = me ? me->body.Lock() : nullptr;
            if (!owner)
                return;

            Vector<Pair<String, Json>> fields = { { "type", Json(picked) } };
            if (!assetSet)
                fields.Add({ "assetType", Json(UiDoc::DefaultAssetType(picked)) });
            owner->SetItemFields(id, fields);
        };
        editor->ShowOptionPicker(request);
    }

    void PipelineKitPopup::OnHotClick(const String& id, const Input::Cursor& cursor)
    {
        auto self = body.Lock();
        if (!self)
            return;

        auto item = self->ItemOf(portId);
        auto flag = [&](const char* key) { return !(item.Get(key) && item.Get(key)->IsBool() && !item.Get(key)->Bool()); };
        if (id == "close")
            self->OpenCell("");
        else if (id == "type")
            PickType();
        else if (id.StartsWith("asset:"))
            self->SetItemFields(portId, { { "assetType", Json(id.SubStr(6)) } });
        else if (id.StartsWith("bg:"))
            self->SetItemFields(portId, { { "bg", Json(id.SubStr(3)) } });
        else if (id == "trim")
            self->SetItemFields(portId, { { "trim", Json(!flag("trim")) } });
        else if (id == "middle")
            self->SetItemFields(portId, { { "optimize", Json(!flag("optimize")) } });
        else if (id == "nineAuto")
            self->SetItemFields(portId, { { "nineSlice", Json(nullptr) } }, { "nineFrom" });
        else if (id == "lock")
        {
            mKeptRatio.reset();
            mLocked = !mLocked;
        }
        else if (id == "sizeAuto")
        {
            mKeptRatio.reset();
            self->SetItemFields(portId, { { "size", Json(nullptr) } });
        }
        else if (id.StartsWith("dpi:"))
        {
            auto own = UiDoc::CleanDpi(item.Get("dpi"));
            bool wantOwn = id == "dpi:own";
            if (wantOwn != own.has_value())
                self->SetItemFields(portId, { { "dpi", wantOwn ? Json(*UiDoc::CleanDpi(self->Device().dpi)) : Json(nullptr) } });
        }
        else if (id == "ai")
            self->Analyze(portId);
        else if (id == "remove")
            self->RemoveInput(portId);
    }
}
