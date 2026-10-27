#include "o2Editor/stdafx.h"
#include "UiLayoutWidgets.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "o2/Scene/ActorCreationMode.h"
#include "o2/Scene/SceneLayer.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/GridLayout.h"
#include "o2/Scene/UI/Widgets/HorizontalProgress.h"
#include "o2/Scene/UI/Widgets/Image.h"
#include "o2/Scene/UI/Widgets/VerticalProgress.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

namespace Editor::UiLayout
{
    using namespace PyValue;

    // --- the plan ----------------------------------------------------------------------------------------------

    namespace
    {
        double Factor(const String& value)
        {
            if (value == "center" || value == "middle")
                return 0.5;
            if (value == "right" || value == "bottom")
                return 1.0;
            return 0.0;
        }

        String AssetKey(const String& asset)
        {
            String key = asset;
            key.ReplaceAll("\\", "/");
            int slash = key.FindLast("/");
            if (slash >= 0)
                key = key.SubStr(slash + 1);
            return key.ToLowerCase();
        }

        struct Planner
        {
            const Map<String, UiSpriteInfo>& sprites;
            const Vector<UiResolvedNode>&    boxes;
            UiWidgetPlan&                    plan;
            int                              next = 0;

            void Warn(const String& text)
            {
                if (!plan.warnings.Contains(text))
                    plan.warnings.Add(text);
            }

            // A native GridLayout when o2 reproduces the grid: one spacing for both axes, no per-cell nudges
            bool NativeGrid(const Json& node, UiPlanNode& p)
            {
                const Json* layout = node.Get("layout");
                const Json* children = node.Get("children");
                if (!layout || Str(layout, "type") != "grid" || !children || !children->IsArray() || children->Items().IsEmpty())
                    return false;

                int columns = (int)Num(layout, "columns", 1);
                const Json* cell = layout->Get("cellSize");
                const Json* spacing = layout->Get("spacing");
                int gx = ToInt(spacing ? spacing->Get("x") : nullptr), gy = ToInt(spacing ? spacing->Get("y") : nullptr);
                int n = children->Items().Count();
                int rows = (n + columns - 1)/Math::Max(1, columns);
                if (rows > 1 && columns > 1 && gx != gy)
                {
                    Warn(p.path + ": grid spacing " + (String)gx + "x" + (String)gy + " baked as positions: o2's GridLayout has one spacing");
                    return false;
                }
                for (auto& c : children->Items())
                {
                    if (c.Has("offset") || c.Has("responsive"))
                    {
                        Warn(p.path + ": grid cells with offsets baked as positions");
                        return false;
                    }
                }

                p.gridCell = Vec2F((float)ToInt(cell ? cell->Get("width") : nullptr), (float)ToInt(cell ? cell->Get("height") : nullptr));
                p.gridSpacing = (float)(columns > 1 ? gx : gy);
                p.gridColumns = columns;
                return true;
            }

            void Transform(const Json& node, UiPlanNode& p, double pw, double ph)
            {
                if (const Json* responsive = node.Get("responsive"); responsive && responsive->IsObject())
                {
                    const Json* mn = responsive->Get("min");
                    const Json* mx = responsive->Get("max");
                    const Json* omn = responsive->Get("offsetMin");
                    const Json* omx = responsive->Get("offsetMax");
                    p.anchorMin = Vec2F((float)Num(mn, "x"), (float)(1.0 - Num(mx, "y")));
                    p.anchorMax = Vec2F((float)Num(mx, "x"), (float)(1.0 - Num(mn, "y")));
                    p.offsetMin = Vec2F((float)Num(omn, "x"), (float)-Num(omx, "y"));
                    p.offsetMax = Vec2F((float)Num(omx, "x"), (float)-Num(omn, "y"));
                    if (Truthy(responsive->Get("safeArea")))
                        Warn(p.path + ": the safe area is not applied, the anchors follow the whole parent");
                    return;
                }

                const Json* anchor = node.Get("anchor");
                double ax = Factor(Str(anchor, "horizontal")), ay = 1.0 - Factor(Str(anchor, "vertical"));
                const Box& r = p.rel;
                p.anchorMin = p.anchorMax = Vec2F((float)ax, (float)ay);
                p.offsetMin = Vec2F((float)(r.x - ax*pw), (float)(ph - r.y - r.h - ay*ph));
                p.offsetMax = Vec2F((float)(r.x + r.w - ax*pw), (float)(ph - r.y - ay*ph));
            }

            void Node(const Json& node, int parent, const Json* parentProgress, double pw, double ph, const String& widgetPath)
            {
                int index = next++;
                const UiResolvedNode& box = boxes[index];
                UiPlanNode p;
                p.path = box.path;
                p.widgetPath = widgetPath;
                p.parent = parent;
                p.rel = box.rel;
                p.abs = box.abs;
                p.name = node.Get("name") ? node.Get("name")->ToJsString() : String("?");
                p.type = Str(&node, "type");
                if (p.type.IsEmpty())
                    p.type = "container";

                if (parent < 0)
                {
                    float w = (float)box.rel.w, h = (float)box.rel.h;
                    p.anchorMin = p.anchorMax = Vec2F(0.5f, 0.5f);
                    p.offsetMin = Vec2F(-w/2, -h/2);
                    p.offsetMax = Vec2F(w/2, h/2);
                }
                else
                    Transform(node, p, pw, ph);

                p.color = Str(&node, "color");
                p.opacity = Math::Clamp(Num(&node, "opacity", 1.0), 0.0, 1.0);
                p.visible = !(node.Get("visible") && node.Get("visible")->IsBool() && !node.Get("visible")->Bool());

                bool isFill = parentProgress && Str(parentProgress, "fill") == p.name;
                if (p.type == "text")
                {
                    p.widget = "Label";
                    p.text = node.Get("text") ? node.Get("text")->ToJsString() : String();
                    p.fontFile = FontFileOf(WeightOfFont(Str(&node, "fontFamily")));
                    p.fontSize = Num(&node, "fontSize", 20);
                    p.alignment = node.Has("alignment") ? Str(&node, "alignment") : String("left");
                    p.textVAlign = node.Has("textVAlign") ? Str(&node, "textVAlign") : String("middle");
                    if (p.color.IsEmpty())
                        p.color = "#000000";
                    p.strokeWidth = Num(&node, "strokeWidth", 0);
                    p.strokeColor = p.strokeWidth > 0 ? (node.Has("strokeColor") ? Str(&node, "strokeColor") : String("#000000")) : String();

                    String family = Str(&node, "fontFamily");
                    if (!family.IsEmpty() && !family.ToLowerCase().Contains("roboto"))
                        Warn(p.path + ": font " + family + " drawn with " + p.fontFile);
                    if (node.Has("lineHeight"))
                        Warn(p.path + ": lineHeight is not applied");
                    if (Num(&node, "textScaleX", 1.0) != 1.0)
                        Warn(p.path + ": textScaleX is not applied");
                }
                else if (isFill)
                {
                    String direction = Str(parentProgress, "direction");
                    bool vertical = direction == "top-to-bottom" || direction == "bottom-to-top";
                    p.widget = vertical ? "VerticalProgress" : "HorizontalProgress";
                    p.progressOrientation = direction == "right-to-left" ? "Left" : direction == "top-to-bottom" ? "Down"
                        : direction == "bottom-to-top" ? "Up" : "Right";
                    double low = Num(parentProgress, "min", 0), high = Num(parentProgress, "max", 1);
                    p.progressValue = high > low ? Math::Clamp((Num(parentProgress, "value", 0) - low)/(high - low), 0.0, 1.0) : 1.0;
                    if (node.Get("children") && node.Get("children")->Count() > 0)
                        Warn(p.path + ": the children of a progress fill are not clipped");
                }
                else if (p.type == "button")
                    p.widget = "Button";
                else if (p.type == "image" || p.type == "rect" || p.type == "overlay")
                    p.widget = "Image";
                else if (node.Has("layout") && NativeGrid(node, p))
                    p.widget = "GridLayout";
                else
                    p.widget = "Widget";

                if (p.type != "text" && p.type != "rect")
                {
                    String asset = Str(&node, "asset");
                    if (asset.StartsWith("./"))
                        asset = asset.SubStr(2);
                    p.asset = asset;
                    if (!asset.IsEmpty())
                    {
                        UiSpriteInfo info;
                        bool known = sprites.TryGetValue(AssetKey(asset), info);
                        p.nine = NineSliceOf(node.Get("nineSlice"), known ? &info : nullptr);
                        if (!known)
                            Warn(p.path + ": missing sprite " + asset);
                    }
                }

                if (Num(&node, "hueShift", 0) != 0)
                    Warn(p.path + ": hueShift is not applied");
                if (const Json* layout = node.Get("layout"); layout && p.widget != "GridLayout")
                {
                    String type = Str(layout, "type");
                    if (type == "row" || type == "column")
                        Warn(p.path + ": " + type + " layout baked as positions");
                }
                if (const Json* state = node.Get("state"); state && state->Get("variants") && state->Get("variants")->Count() > 1)
                {
                    Warn(p.path + ": state \"" + Str(state, "current") + "\" baked, the other " +
                         (String)(state->Get("variants")->Count() - 1) + " variant(s) are not exported");
                }
                if (node.Has("scroll"))
                    Warn(p.path + ": scroll viewport baked at its offset, no ScrollArea");

                int self = plan.nodes.Count();
                plan.nodes.Add(p);

                const Json* children = node.Get("children");
                if (!children || !children->IsArray())
                    return;

                const Json* progress = node.Get("progress");
                for (auto& child : children->Items())
                {
                    String childName = child.Get("name") ? child.Get("name")->ToJsString() : String("?");
                    Node(child, self, progress && progress->IsObject() ? progress : nullptr, box.rel.w, box.rel.h,
                         widgetPath + "/" + childName);
                }
            }
        };
    }

    bool PlanWidgets(const Json& structure, const Map<String, UiSpriteInfo>& sprites, UiWidgetPlan& out, String* error /*= nullptr*/)
    {
        out = UiWidgetPlan();
        Json scene = SnapshotStructure(structure);
        Vector<UiResolvedNode> boxes;
        if (!ResolveStructure(scene, boxes, error))
            return false;

        const Json* canvas = scene.Get("canvas");
        out.rootName = Str(canvas, "name");
        if (out.rootName.IsEmpty())
            out.rootName = "Screen";
        out.size = Vec2F((float)boxes[0].rel.w, (float)boxes[0].rel.h);

        static const Json empty = Json::Object();
        const Json* root = scene.Get("root");
        Planner planner{ sprites, boxes, out };
        planner.Node(root ? *root : empty, -1, nullptr, boxes[0].rel.w, boxes[0].rel.h, out.rootName);
        return true;
    }

    // --- the widgets -------------------------------------------------------------------------------------------

    static Color4 ColorOfHex(const String& hex, const Color4& def, double opacity = 1.0)
    {
        Color4 color = def;
        if (!PipelineUtils::ParseHexColor(hex, color))
            color = def;
        color.a = (int)JsRound(255*opacity);
        return color;
    }

    // Nine-slice margins usable on a sprite drawn at a size: at least one pixel of centre is left on both axes
    static void FitMargins(int& a, int& b, int limit)
    {
        a = Math::Max(0, a);
        b = Math::Max(0, b);
        if (a + b > limit - 1)
        {
            double k = (limit - 1.0)/Math::Max(1, a + b);
            a = (int)std::floor(a*k);
            b = (int)std::floor(b*k);
        }
    }

    UiWidgetTree BuildWidgets(const UiWidgetPlan& plan, UiWidgetResources& resources, const UiWidgetOptions& options)
    {
        PushEditorScopeOnStack scope;
        auto prevMode = Actor::GetDefaultCreationMode();
        Actor::SetDefaultCreationMode(ActorCreateMode::NotInScene);

        UiWidgetTree tree;
        tree.warnings = plan.warnings;
        Ref<SceneLayer> layer;
        if (!options.layer.IsEmpty())
        {
            layer = mmake<SceneLayer>();
            layer->SetName(options.layer);
        }

        float depth = 0.0f;
        Vector<Ref<Widget>> built;
        for (auto& p : plan.nodes)
        {
            Ref<Widget> widget;
            Ref<Label> label;
            if (p.widget == "Label")
            {
                label = mmake<Label>();
                widget = label;
            }
            else if (p.widget == "HorizontalProgress")
                widget = mmake<HorizontalProgress>();
            else if (p.widget == "VerticalProgress")
                widget = mmake<VerticalProgress>();
            else if (p.widget == "Button")
                widget = mmake<Button>();
            else if (p.widget == "Image")
                widget = mmake<Image>();
            else if (p.widget == "GridLayout")
            {
                auto grid = mmake<GridLayout>();
                const UiPlanNode* first = nullptr;
                for (auto& c : plan.nodes)
                {
                    if (c.parent == built.Count())
                    {
                        first = &c;
                        break;
                    }
                }
                grid->SetBaseCorner(BaseCorner::LeftTop);
                grid->SetSpacing(p.gridSpacing);
                grid->SetCellSize(p.gridCell);
                grid->SetArrangeAxis(TwoDirection::Horizontal);
                grid->SetArrangeAxisMaxCells(p.gridColumns);
                grid->SetBorderLeft(first ? (float)first->rel.x : 0.0f);
                grid->SetBorderTop(first ? (float)first->rel.y : 0.0f);
                grid->SetBorderRight(0);
                grid->SetBorderBottom(0);
                grid->SetFitByChildren(false);
                widget = grid;
            }
            else
                widget = mmake<Widget>();

            widget->SetName(p.parent < 0 ? plan.rootName : p.name);
            widget->layout->anchorMin = p.anchorMin;
            widget->layout->anchorMax = p.anchorMax;
            widget->layout->offsetMin = p.offsetMin;
            widget->layout->offsetMax = p.offsetMax;

            if (layer)
                widget->SetLayer(layer);
            if (options.explicitDepths)
                widget->SetDrawingDepth(depth++);
            else
                widget->SetDrawingDepthInheritFromParent(true);
            SceneUID id;
            if (options.keepIds.TryGetValue(p.widgetPath, id))
                widget->SetID(id);

            bool hasChildren = plan.nodes.Any([&](const UiPlanNode& c) { return c.parent == built.Count(); });
            Ref<WidgetLayer> visualLayer;
            if (label)
            {
                resources.SetFont(label, p.fontFile);
                label->SetHeight(HeightPoints(p.fontSize));
                label->SetHorAlign(p.alignment == "right" ? HorAlign::Right : p.alignment == "center" ? HorAlign::Middle : HorAlign::Left);
                label->SetVerAlign(p.textVAlign == "top" ? VerAlign::Top : p.textVAlign == "bottom" ? VerAlign::Bottom : VerAlign::Middle);
                label->SetHorOverflow(Label::HorOverflow::None);
                label->SetVerOverflow(Label::VerOverflow::None);
                label->SetText(WString(p.text));
                Color4 color = ColorOfHex(p.color, Color4::Black());
                if (!p.strokeColor.IsEmpty())
                {
                    Color4 stroke = ColorOfHex(p.strokeColor, Color4::Black());
                    resources.SetStroke(label, color, stroke, Math::Max(1, (int)JsRound(p.strokeWidth)));
                    label->SetColor(Color4::White());
                }
                else
                    label->SetColor(color);
            }
            else
            {
                Ref<Sprite> sprite;
                bool flat = p.type == "rect" || ((p.type == "overlay" || p.type == "container" || p.type == "button") && p.asset.IsEmpty() &&
                                                 !p.color.IsEmpty());
                if (flat)
                {
                    sprite = resources.WhiteSprite();
                    if (sprite)
                        sprite->SetColor(ColorOfHex(p.color, Color4::White()));
                }
                else if (!p.asset.IsEmpty())
                {
                    sprite = resources.SpriteOf(p.asset);
                    if (!sprite)
                        tree.warnings.Add(p.path + ": missing sprite " + p.asset);
                    else
                    {
                        if (!p.color.IsEmpty())
                            sprite->SetColor(ColorOfHex(p.color, Color4::White()));
                        if (p.nine)
                        {
                            Vec2I size = sprite->GetTextureSrcRect().Size();
                            int l = p.nine->left, r = p.nine->right, t = p.nine->top, b = p.nine->bottom;
                            FitMargins(l, r, Math::Min(size.x, (int)p.rel.w));
                            FitMargins(t, b, Math::Min(size.y, (int)p.rel.h));
                            if (l || r || t || b)
                            {
                                sprite->SetMode(SpriteMode::Sliced);
                                sprite->SetSliceBorder(BorderI(l, b, r, t));
                            }
                        }
                    }
                }

                if (sprite)
                {
                    if (auto image = DynamicCast<Image>(widget))
                    {
                        image->SetImage(sprite);
                        visualLayer = image->GetLayer("image");
                    }
                    else if (p.widget == "HorizontalProgress" || p.widget == "VerticalProgress")
                        visualLayer = widget->AddLayer("bar", sprite);
                    else if (p.widget == "Button")
                        visualLayer = widget->AddLayer("back", sprite);
                    else
                        visualLayer = widget->AddLayer("image", sprite);
                }
                else if (auto image = DynamicCast<Image>(widget))
                    image->SetImage(mmake<Sprite>(Color4(0, 0, 0, 0)));
            }

            if (auto bar = DynamicCast<HorizontalProgress>(widget))
            {
                bar->SetOrientation(p.progressOrientation == "Left" ? HorizontalProgress::Orientation::Left : HorizontalProgress::Orientation::Right);
                bar->SetValueRange(0.0f, 1.0f);
                bar->SetValueForcible((float)p.progressValue);
            }
            else if (auto bar = DynamicCast<VerticalProgress>(widget))
            {
                bar->SetOrientation(p.progressOrientation == "Up" ? VerticalProgress::Orientation::Up : VerticalProgress::Orientation::Down);
                bar->SetValueRange(0.0f, 1.0f);
                bar->SetValueForcible((float)p.progressValue);
            }

            // Opacity is the node's own: a container's children keep theirs
            if (p.opacity < 1.0)
            {
                if (!hasChildren)
                    widget->SetTransparency((float)p.opacity);
                else if (visualLayer)
                    visualLayer->transparency = (float)p.opacity;
            }
            if (!p.visible)
                widget->SetEnabled(false);

            if (p.parent >= 0)
                built[p.parent]->AddChild(widget);
            else
                tree.root = widget;
            built.Add(widget);
            tree.widgets.Add(widget);
        }

        Actor::SetDefaultCreationMode(prevMode);
        return tree;
    }

    static void Settle(const Ref<Actor>& actor)
    {
        if (auto bar = DynamicCast<HorizontalProgress>(actor))
            bar->SetValueForcible(bar->GetValue());
        if (auto bar = DynamicCast<VerticalProgress>(actor))
            bar->SetValueForcible(bar->GetValue());

        for (auto& child : actor->GetChildren())
            Settle(child);
    }

    void UpdateWidgetTree(const Ref<Widget>& root)
    {
        PushEditorScopeOnStack scope;
        for (int pass = 0; pass < 2; pass++)
        {
            root->Update(0.0f);
            root->UpdateChildren(0.0f);
            root->UpdateSelfTransform();
            root->UpdateChildrenTransforms();
            Settle(root);
        }
    }

    static void CollectIds(const Ref<Actor>& actor, const String& prefix, Map<String, SceneUID>& out)
    {
        String path = prefix.IsEmpty() ? actor->GetName() : prefix + "/" + actor->GetName();
        out[path] = actor->GetID();
        for (auto& child : actor->GetChildren())
            CollectIds(child, path, out);
    }

    Map<String, SceneUID> CollectActorIds(const Ref<Actor>& root)
    {
        Map<String, SceneUID> out;
        if (root)
            CollectIds(root, "", out);
        return out;
    }
}
