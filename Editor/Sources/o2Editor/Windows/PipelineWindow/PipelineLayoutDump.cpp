#include "o2Editor/stdafx.h"
#include "PipelineLayoutDump.h"

#include "o2/Render/Text.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/DropDown.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalProgress.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineMediaViews.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeBody.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"

namespace Editor
{
    namespace
    {
        struct Writer
        {
            DataValue& blocks;
            RectF      card;

            void Add(const String& kind, const String& name, const RectF& rect, const String& text = "")
            {
                if (rect.Width() <= 0.01f && rect.Height() <= 0.01f)
                    return;

                auto& block = blocks.AddElement();
                block.SetObject();
                block["kind"] = kind;
                block["name"] = name;
                block["x"] = Math::Round((rect.left - card.left) * 100.0f) / 100.0f;
                block["y"] = Math::Round((card.top - rect.top) * 100.0f) / 100.0f;
                block["w"] = Math::Round(rect.Width() * 100.0f) / 100.0f;
                block["h"] = Math::Round(rect.Height() * 100.0f) / 100.0f;
                if (!text.IsEmpty())
                    block["text"] = text;
            }

            void AddLayer(const String& kind, const String& name, const Ref<WidgetLayer>& layer)
            {
                if (!layer || !layer->enabled)
                    return;

                String text;
                if (auto drawable = DynamicCast<Text>(layer->GetDrawable()))
                    text = (String)drawable->GetText();
                Add(kind, name, layer->GetRect(), text);
            }

            static String CaptionOf(const Widget& widget)
            {
                if (auto layer = widget.GetLayer("caption"))
                {
                    if (auto text = DynamicCast<Text>(layer->GetDrawable()))
                        return (String)text->GetText();
                }
                return "";
            }

            void Walk(const Ref<Widget>& widget)
            {
                if (!widget || !widget->IsEnabledInHierarchy())
                    return;

                RectF rect = widget->layout->GetWorldRect();
                if (auto slider = DynamicCast<PipelineSlider>(widget))
                {
                    Add("slider", widget->GetName(), rect);
                    AddLayer("slider-label", "", slider->GetLayer("label"));
                    for (auto& child : slider->GetChildWidgets())
                    {
                        if (auto track = DynamicCast<HorizontalProgress>(child))
                        {
                            Add("slider-track", "", track->layout->GetWorldRect());
                            AddLayer("slider-fill", "", track->GetLayer("bar"));
                        }
                    }
                    AddLayer("slider-value", "", slider->GetLayer("value"));
                    return;
                }

                if (DynamicCast<PipelineImageView>(widget))
                {
                    Add("picture", widget->GetName(), rect);
                    return;
                }

                if (DynamicCast<PipelineTextView>(widget))
                {
                    Add("text-out", widget->GetName(), rect);
                    return;
                }

                if (DynamicCast<PipelineColorField>(widget))
                {
                    Add("color", widget->GetName(), rect);
                    return;
                }

                if (DynamicCast<PipelineAudioView>(widget) || DynamicCast<PipelineVideoView>(widget))
                {
                    Add(DynamicCast<PipelineAudioView>(widget) ? "audio" : "video", widget->GetName(), rect);
                    return;
                }

                if (auto edit = DynamicCast<EditBox>(widget))
                {
                    Add("field", widget->GetName(), rect, (String)edit->GetText());
                    return;
                }

                if (DynamicCast<DropDown>(widget))
                {
                    Add("select", widget->GetName(), rect);
                    return;
                }

                if (auto toggle = DynamicCast<Toggle>(widget))
                {
                    bool isSwitch = toggle->GetLayer("trackOn") != nullptr;
                    Add(isSwitch ? "toggle-row" : "segment", widget->GetName(), rect, CaptionOf(*toggle));
                    if (isSwitch)
                    {
                        AddLayer("switch", "", toggle->GetLayer("track"));
                        AddLayer("switch-label", "", toggle->GetLayer("caption"));
                    }
                    return;
                }

                if (auto button = DynamicCast<Button>(widget))
                {
                    String name = widget->GetName();
                    String kind = name == "params" ? "fold" : name == "model" ? "select" : button->GetLayer("caption") ? "button" : "tool";
                    Add(kind, widget->GetName(), rect, CaptionOf(*button));
                    return;
                }

                if (auto label = DynamicCast<Label>(widget))
                {
                    Add("label", widget->GetName(), rect, (String)label->GetText());
                    return;
                }

                if (widget->GetLayer("segBack"))
                    Add("segmented", widget->GetName(), rect);

                for (auto& child : widget->GetChildWidgets())
                    Walk(child);
            }
        };
    }

    void PipelineLayoutDump::Dump(const PipelineNodeWidget& card, DataValue& out)
    {
        out.SetObject();
        RectF rect = card.GetCardRect();
        out["card"].SetObject();
        out["card"]["w"] = rect.Width();
        out["card"]["h"] = rect.Height();

        auto& blocks = out["blocks"];
        blocks.SetArray();
        Writer writer{ blocks, rect };
        writer.Add("card", "", rect);
        writer.AddLayer("icon", "", card.mIconLayer);
        writer.AddLayer("title", "", card.mTitleLayer);
        if (card.mPlayButton && card.mPlayButton->enabled)
            writer.Add("run", "play", card.mPlayButton->layout->GetWorldRect());
        writer.AddLayer("dot", "", card.mDotLayer);
        writer.AddLayer("dot", "hollow", card.mDotRingLayer);

        auto ports = [&](const Vector<PipelineNodeWidget::PortView>& views, bool input)
        {
            for (auto& view : views)
            {
                Vec2F c(rect.left + view.localPos.x, rect.top - view.localPos.y);
                writer.Add(input ? "port-in" : "port-out", view.port.name, RectF(c.x - 5, c.y + 5, c.x + 5, c.y - 5));
                writer.AddLayer("port-label", view.port.name, view.label);
            }
        };
        ports(card.mInputs, true);
        ports(card.mOutputs, false);
        writer.AddLayer("port-label", "add-input", card.mAddLabelLayer);

        if (card.mBody)
        {
            writer.Add("body-stack", "", card.mBody->layout->GetWorldRect());
            for (auto& child : card.mBody->GetChildWidgets())
                writer.Walk(child);
        }
    }
}
