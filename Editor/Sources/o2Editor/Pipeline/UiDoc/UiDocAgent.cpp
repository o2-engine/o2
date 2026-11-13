#include "o2Editor/stdafx.h"
#include "UiDocAgent.h"

#include "o2Editor/Pipeline/UiDoc/UiDocUiml.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc
{
    const char* const UI_AGENT_DEFAULT_MODEL = "gpt-6.1-sol";

    // The texts below are the web's constants with the type list and the anchor presets written out (UiNodeTypes() without
    // screen, AnchorPresets()); the golden test compares whole prompts with the web's

    const String& UimlSpec()
    {
        static const String text =
            "UIML \xE2\x80\x94 a screen as indented text, one block per line, children indented 2 spaces under their parent:\n"
            "\n"
            "screen <Name> <W>x<H>\n"
            "  <type> <id> key=value ... \"text\"\n"
            "\n"
            "Types: group, panel, background, image, icon, button, text, progress, toggle, slider, input, tab, badge, divider, slot; row / column / grid are groups with that layout.\n"
            "- group: holds children, draws nothing. panel: a surface (window, card, popup). background: fills behind content.\n"
            "- image: a picture; icon: a small symbol; button / tab / badge: draw their \"text\" as a label.\n"
            "- text: a label (\"text\" + style=). progress: bar with value=0..1. toggle: on=true|false. slider: value=0..1. input: placeholder=\"...\" or \"text\".\n"
            "- divider: a line. slot: an inventory / grid cell.\n"
            "\n"
            "Placement inside a parent WITHOUT layout \xE2\x80\x94 by anchor:\n"
            "- anchor=<preset>: top-left, top-center, top-right, middle-left, center, middle-right, bottom-left, bottom-center, bottom-right, top-stretch, middle-stretch, bottom-stretch, stretch-left, stretch-center, stretch-right, stretch (vertical-horizontal; \"stretch\" on an axis = fill it).\n"
            "- pos=X,Y: offset from the anchor point, inwards from the anchored edge (for center: + = right/down).\n"
            "- size=WxH (or w= / h=): the size along non-stretched axes.\n"
            "- margin=T,R,B,L (CSS order; 1, 2 or 4 numbers): gaps to the parent's edges along stretched axes.\n"
            "Layout of children (any block): layout=row|column|grid (or the row/column/grid type),\n"
            "- gap=N (grid: gap=X,Y), pad=T,R,B,L, align=start|center|end|stretch (cross axis; default stretch = fill when the child gives no size there),\n"
            "- justify=start|center|end|between|around|evenly (main axis), grow=N on a child takes a share of free space.\n"
            "- grid: cols=N, cell=WxH (default: width split evenly, square cells); children fill their cell.\n"
            "- inside a layout a child's pos=X,Y only nudges it.\n"
            "- rotation=DEG turns a block clockwise about the centre of its box (negative = counter-clockwise), its children with it;"
            " size= stays the block's own unturned size (a tilted 120x120 tile is size=120x120 rotation=-12), and its box is placed as if"
            " it were not turned. Use it for elements that are tilted on purpose (scattered tiles, stickers, a slanted badge), never to"
            " fix a layout.\n"
            "Look: kit=<graphic name> picks a UI kit graphic; without it a block takes a kit graphic of its type (icons/images only by a word of their id).\n"
            "  kit=none draws no graphic: a plain block of its color= (a translucent layer dimming the screen under a popup: panel dim"
            " anchor=stretch kit=none color=#000000 opacity=0.6; a flat strip), or nothing.\n"
            ""
            "  variant=primary|secondary|... picks among graphics of a type; style=<text style name> for labels; color=#RRGGBB tints; opacity=0..1; visible=false hides; fit=contain|cover|stretch for pictures.\n"
            "Text: \"quoted text\", textAlign=left|center|right, fontSize=N (overrides the style), textColor=#RRGGBB. note=\"...\" = what a block is for (not drawn).";
        return text;
    }

    const String& UiEditsSpec()
    {
        static const String text =
            "Edit commands (one per line; blocks under @add/@replace are UIML lines):\n"
            "@update <id> key=value ... \"new text\"        set props (key=- removes one; type=..., id=new_id rename)\n"
            "@add <parentId> [after=<id> | before=<id> | index=N]\n"
            "  <uiml block lines>\n"
            "@remove <id> [<id> ...]\n"
            "@move <id> to=<parentId> [after=<id> | before=<id> | index=N]\n"
            "@replace <id>\n"
            "  <uiml block lines (one block with its children)>\n"
            "@screen [name=<Name>] [size=WxH]\n"
            "The screen itself is the block \"root\": @add root \xE2\x80\xA6, @move <id> to=root. Later siblings are drawn over earlier"
            " ones \xE2\x80\x94 reorder with @move \xE2\x80\xA6 before= / after= / index=.";
        return text;
    }

    static const char* const MISSING_PLACEHOLDER =
        "- What the kit has no graphic for (a wooden plate, a frame, background art\xE2\x80\xA6) stays a plain block without kit"
        " \xE2\x80\x94 a placeholder the user will fill. Don't fake its look with color=; name what is missing in your reply.";

    static const char* const MISSING_DRAW =
        "- What the kit has no graphic for (or there is no kit): ask for it with @asset (below) and put kit=<that name> on its block"
        " \xE2\x80\x94 it is drawn for you and joins the kit. One @asset per distinct piece of artwork (all buttons of one look"
        " share one), at most 12; never ask for what a kit graphic can stand for (go through the kit sheet first: a graphic of that"
        " role, even of another colour, is used instead), and never fake a look with color=.";

    // How to use the kit (what it lacks: placeholders, or graphics drawn for the node), then the layout rules
    static String Rules(bool draw)
    {
        return String(
            "How to use the UI kit \xE2\x80\x94 the most important part:\n"
            "- The kit sheet picture shows every kit graphic with its name. Before writing, match each element of the request (and of"
            " the reference) to a kit graphic by its purpose, picture, name and description, and put kit=<exact name> on that block.\n"
            "- Pick graphics by purpose: \"use: \xE2\x80\xA6\" in the list below says what a graphic is made for (the background of"
            " popups, the backing of the main action button, the frame of an avatar\xE2\x80\xA6). A block gets the graphic made for"
            " its role \xE2\x80\x94 not merely one of the same type \xE2\x80\x94 and a graphic is not used against its purpose while"
            " another one fits.\n"
            "- Kit graphics are finished pieces: a framed avatar, a button, an icon, a panel are drawn whole. Place each as ONE block"
            " with kit=\xE2\x80\xA6; never rebuild one out of nested panels and colours.\n"
            "- Icons come from the kit (kit=icon_\xE2\x80\xA6). Never draw an icon with text characters or emoji (\xE2\x99\xA5 \xE2\x98\x85"
            " \xE2\x9A\x99 \xE2\x9C\x95 + C \xE2\x80\xA6).\n"
            "- A button graphic is the button's background; its label is the button's \"text\". Sliced (9-slice) graphics stretch to"
            " any size; simple ones keep their aspect \xE2\x80\x94 give their blocks the graphic's aspect ratio (see its size).\n"
            "- The kit's sizes are already in this screen's design pixels (its dpi): a graphic looks as drawn at that size \xE2\x80\x94"
            " icons and pictures at about their listed size, a sliced block never smaller than its corners. Keep the screen's size and"
            " dpi.\n"
            "") +
            (draw ? MISSING_DRAW : MISSING_PLACEHOLDER) +
            "\n"
            "- color= only tints a kit graphic (a plain colour block is kit=none color=\xE2\x80\xA6: for flat shapes like a dimming"
            " layer, never as a stand-in for artwork).\n"
            "- References: take the composition, hierarchy, proportions and texts from them; the look comes from the kit \xE2\x80\x94"
            " a kit graphic of the right role is used even where its colours or details differ from the reference.\n"
            "\n"
            "Layout rules:\n"
            "- Coordinates are pixels of the design size. Keep 3\xE2\x80\x93" "5 % side margins; tap targets at least 88 px high at"
            " 1080 wide.\n"
            "- Build adaptively: backgrounds anchor=stretch; bars and HUD attach to their edges (top-stretch, bottom-stretch, top-right\xE2\x80\xA6);"
            " menus, lists and nav bars are column / row / grid layouts, not hand-placed blocks.\n"
            "- Keep the tree shallow: what reads as one piece is one block.\n"
            "- Ids: short, unique, snake_case, meaningful (play_button, coins_label). Keep existing ids when editing.\n"
            "- Every label must fit its box: give text blocks enough width and height for their style size.\n"
            "- Every label uses a text style: style=<name> of a kit style that fits its look and role. When none fits, define one with"
            " @style (below) and use it \xE2\x80\x94 titles, button labels and numbers never stay in the default style.\n"
            "- Change only what the request asks; keep the rest as it is.";
    }

    String UiKitAdditionsSpec(bool draw)
    {
        Vector<String> fonts;
        for (auto& f : UiFonts())
        {
            Vector<String> weights;
            for (auto& face : f.faces)
                weights.Add(JsNum(face.weight));
            fonts.Add(f.family + " (" + f.look + "; " + Join(weights, "/") + ")");
        }

        String text = String(
            "Kit additions (lines of their own inside the fenced block, before the blocks that use them):\n"
            "@style <name> role=") + Join(TextRoles(), "|") +
            " font=\"<family>\" weight=<400|500|700|900> size=<px> color=#RRGGBB [gradient=#RRGGBB[,#RRGGBB\xE2\x80\xA6][,<angle>deg]]"
            " [stroke=#RRGGBB,<width px>]\xE2\x80\xA6 [extrude=#RRGGBB,<x>,<y>] [shadow=#RRGGBB,<x>,<y>,<blur>,<opacity 0..1>]\xE2\x80\xA6"
            " [glow=#RRGGBB,<size px>,<opacity>] [inner=#RRGGBB,<x>,<y>,<blur>,<opacity>] [bevel=#light,#dark,<depth px>,<opacity>]"
            " [spacing=<px>] [uppercase=true] [align=left|center|right] \"what it is for\"\n"
            "  defines a text style (the same name again redefines it). Read the lettering off the reference picture when there is one"
            " \xE2\x80\x94 the closest font below, its size in this screen's design pixels (about 1.4 \xC3\x97 the height of a capital"
            " letter), then its layers from the inside out: the fill (gradient = the colours that follow color= down the letters, evenly"
            " spaced: one for a plain fade, more for bands; an angle other than top-to-bottom as e.g. 0deg = left to right), light and"
            " dark edges inside the letters (bevel=, inner= \xE2\x80\x94 an inner shadow: y > 0 shades the top, y < 0 the bottom), every"
            " outline (stroke= may repeat: the first is next to the letters, the next goes around it), the solid depth under them (extrude="
            " \xE2\x80\x94 the dark lip of cartoon titles), shadows (blur 0 = a hard copy) and a glow \xE2\x80\x94 else design one that"
            " suits the screen.\n"
            "  Fonts: " + Join(fonts, "; ") + ".";
        if (draw)
        {
            text += String("\n"
            "@asset <name> type=<") + Join(UiElementTypes(), "|") +
                "> asset=simple|sliced|tiled size=<W>x<H> [box=<x>,<y>,<w>,<h>] [purpose=\"where it is used\"] \"what to draw\"\n"
                "  asks for a graphic the kit lacks: the artwork alone \xE2\x80\x94 never the text, numbers or icons that sit on it (those"
                " are their own blocks). size = how large it is on this screen, design pixels. sliced for surfaces that stretch (buttons,"
                " panels, frames, bars, fields), simple for icons and pictures.\n"
                "  box = where that artwork is in the FIRST reference picture, in pixels of that picture at the full size given in \"References\""
                " \xE2\x80\x94 give it whenever the reference shows the element, so it is cut from there instead of drawn anew.";
        }
        return text;
    }

    // 1-based numbers of the pictures a request sends, in order: screenshot, kit sheet, references, enlarged differences
    // (0 = not sent)
    struct ImageNumbers
    {
        int shot = 0, sheet = 0, firstRef = 1, zoom = 0;
    };

    static ImageNumbers ImageNumbersOf(const UiAgentContext& ctx)
    {
        ImageNumbers out;
        int n = 1;
        if (ctx.withScreenshot)
            out.shot = n++;
        if (ctx.withKitSheet)
            out.sheet = n++;
        out.firstRef = n;
        n += ctx.references.Count([](const UiAgentReference& r) { return r.kind == "image"; });
        if (ctx.run && ctx.run->compare && ctx.run->compare->zoom)
            out.zoom = n;
        return out;
    }

    Vector<String> BlocksAt(const UiDoc& doc, double x, double y)
    {
        UiRects rects = SolveLayout(doc);
        Vector<String> out;
        String hit = HitTest(doc, rects, x, y);
        if (hit.IsEmpty())
            return out;

        Vector<String> chain;
        Function<bool(const UiNode&, const Vector<String>&)> find = [&](const UiNode& n, const Vector<String>& path) {
            if (n.id == hit)
            {
                chain.Add(path);
                chain.Add(n.id);
                return true;
            }
            Vector<String> next = path;
            if (&n != &doc.root)
                next.Add(n.id);
            for (auto& c : n.children)
            {
                if (find(c, next))
                    return true;
            }
            return false;
        };
        find(doc.root, {});
        for (int i = chain.Count() - 1; i >= 0 && out.Count() < 3; i--)
            out.Add(chain[i]);
        return out;
    }

    // The places the user pointed at, each with the blocks under it
    static Vector<String> PointLines(const UiAgentContext& ctx)
    {
        Vector<String> out;
        if (ctx.points.IsEmpty())
            return out;

        UiRects rects = SolveLayout(ctx.doc);
        bool noted = ctx.points.Any([](const UiStepPoint& p) { return !p.text.IsEmpty(); });
        out.Add("");
        out.Add("## Points \xE2\x80\x94 the user marked " + JsNum(ctx.points.Count()) + " place(s) on the screen" +
                (ctx.pinned ? " (the numbered pins on the screenshot; the same pins are on the first reference picture)" : "") +
                ". The request names them by number, as [1], [2]\xE2\x80\xA6: what it says beside a number is about what is at "
                "that point" + (noted ? "; a point with a note of its own: the note is about what is at it" : "") + ".");
        for (int i = 0; i < ctx.points.Count(); i++)
        {
            const UiStepPoint& p = ctx.points[i];
            Vector<String> at;
            for (auto& id : BlocksAt(ctx.doc, p.x, p.y))
            {
                if (rects.Has(id))
                {
                    UiRect r = rects.Get(id);
                    at.Add(id + " (" + JsNum(JsRound(r.x)) + "," + JsNum(JsRound(r.y)) + " " + JsNum(JsRound(r.w)) + "x" +
                           JsNum(JsRound(r.h)) + ")");
                }
                else
                    at.Add(id);
            }
            out.Add(JsNum(i + 1) + ". at " + JsNum(p.x) + "," + JsNum(p.y) +
                    (!at.IsEmpty() ? " \xE2\x80\x94 on " + Join(at, " in ") : String(" \xE2\x80\x94 on the empty screen")) +
                    (!p.text.IsEmpty() ? ": <note>" + p.text + "</note>" : String()));
        }
        out.Add("Do what the request asks at each place it names; leave the rest of the screen as it is.");
        return out;
    }

    // Where the run stands, what the check of the last turn found, and how to end
    static Vector<String> RunLines(const UiAgentContext& ctx, const ImageNumbers& num)
    {
        Vector<String> out;
        if (!ctx.run)
            return out;

        const UiAgentRunState& run = *ctx.run;
        out.Add("");
        out.Add("## Your work on this request so far");
        out.Add("A request is worked in turns: after each answer the screen is rendered again and shown to you. This is turn " +
                JsNum(run.turn) + " of at most " + JsNum(run.maxTurns) + (run.turn >= run.maxTurns ? " \xE2\x80\x94 the last one" : "") + ".");
        for (auto& l : run.log)
            out.Add("- " + l);
        if (run.built)
            out.Add("The current layout was drafted from the reference picture: every element the reference was read into is a block "
                    "at its measured place, drawn by the kit graphic chosen for it; artwork the kit lacks was cut out of the reference "
                    "and is in the kit list as the node's own graphics. Your job now is to make the screen match the reference.");
        if (run.compare && num.shot)
        {
            const UiAgentCompare& c = *run.compare;
            out.Add("");
            out.Add("## The screen against the reference");
            out.Add("Image #" + JsNum(num.shot) + " (the screen now) and image #" + JsNum(num.firstRef) + " (the reference) carry the "
                    "same measuring grid: a line every " + JsNum(c.gridStep) + " design pixels, with its coordinate at the top and "
                    "left edges. Read positions and sizes off the grid.");
            out.Add("They are " + JsNum(JsRound(c.score*100)) + "% alike (a rough pixel measure)." +
                    (!c.regions.IsEmpty() ? " The places that differ most (x,y WxH in design pixels \xE2\x80\x94 the blocks there):"
                                          : " No place stands out."));
            for (int i = 0; i < c.regions.Count(); i++)
            {
                auto& r = c.regions[i];
                out.Add(JsNum(i + 1) + ". " + JsNum(JsRound(r.box.x)) + "," + JsNum(JsRound(r.box.y)) + " " + JsNum(JsRound(r.box.w)) +
                        "x" + JsNum(JsRound(r.box.h)) +
                        (!r.blocks.IsEmpty() ? " \xE2\x80\x94 " + Join(r.blocks, ", ")
                                             : String(" \xE2\x80\x94 no block there (something is missing?)")));
            }
            if (num.zoom)
                out.Add("Image #" + JsNum(num.zoom) + " shows these places enlarged: the reference on the left, the screen now on "
                        "the right.");
            out.Add("Go over the two pictures part by part \xE2\x80\x94 the top bar, each window, each row, the bottom bar \xE2\x80\x94 "
                    "and fix what is off: a block in the wrong place or of the wrong size, a missing or an extra block, the wrong "
                    "order (what covers what), a label with the wrong text, size, alignment or style, a block drawn by the wrong kit "
                    "graphic or by none.");
            out.Add("The kit decides the look: a kit graphic that differs from the reference in colour or detail is right as it is "
                    "\xE2\x80\x94 never swap it for a new @asset to chase the reference's pixels. Differences that come only from "
                    "that are not worth a turn.");
            out.Add("A label must read well on the kit graphic under it: where that graphic is darker or lighter than the reference's "
                    "surface, give the label a style (or textColor=) that contrasts with the kit graphic rather than the reference's "
                    "colour.");
            out.Add("Do not go back and forth: a value you changed in an earlier turn (see the list above) is changed again only when "
                    "the pictures clearly show it is still wrong.");
        }
        if (!run.findings.IsEmpty())
        {
            out.Add("");
            out.Add("## A check of the screen found");
            for (auto& f : run.findings)
                out.Add("- " + f);
        }
        if (run.turn > 1 || run.built)
        {
            out.Add("");
            out.Add("When nothing worth fixing is left, do not write a block: answer with your message to the user (what the screen is "
                    "now, what the kit lacked) and the single line @done.");
        }
        if (run.canMarkup)
        {
            out.Add("");
            out.Add("To rebuild the WHOLE screen from the reference picture (image #" + JsNum(num.firstRef) + ") \xE2\x80\x94 when "
                    "the request asks to redo, rebuild or re-lay the screen by the reference \xE2\x80\x94 answer with the single line "
                    "@markup and nothing else: the reference is then read into elements with measured boxes and matched to the kit, "
                    "and you go on from that draft.");
        }
        return out;
    }

    // Every block's solved box: "id: x,y wxh", at most 160 lines then an ellipsis
    static String Boxes(const UiDoc& doc)
    {
        UiRects rects = SolveLayout(doc);
        Vector<String> lines;
        for (auto& id : rects.Ids())
        {
            if (id == "root")
                continue;
            UiRect r = rects.Get(id);
            lines.Add(id + ": " + JsNum(JsRound(r.x)) + "," + JsNum(JsRound(r.y)) + " " + JsNum(JsRound(r.w)) + "x" + JsNum(JsRound(r.h)));
            if (lines.Count() >= 160)
            {
                lines.Add("\xE2\x80\xA6");
                break;
            }
        }
        return Join(lines, "\n");
    }

    // The last 10 steps as the conversation so far
    static String History(const Vector<UiStep>& steps)
    {
        if (steps.IsEmpty())
            return "(none \xE2\x80\x94 this is the first request)";

        Vector<String> out;
        for (int i = std::max(0, steps.Count() - 10); i < steps.Count(); i++)
        {
            const UiStep& s = steps[i];
            if (s.By() == "agent")
            {
                out.Add("User: " + s.Prompt().value_or("") + "\nYou: " + s.Reply().value_or(""));
                continue;
            }
            String what = UserStepText(s);
            if (what.IsEmpty())
                what = JsSplit(OpsToText(s.Ops()), "\n")[0];
            out.Add("(the user edited by hand: " + JsSlice(what, 0, 200) + ")");
        }
        return Join(out, "\n\n");
    }

    String BuildAgentPrompt(const UiAgentContext& ctx)
    {
        ImageNumbers num = ImageNumbersOf(ctx);
        int img = num.firstRef;
        Vector<String> refLines;
        for (auto& r : ctx.references)
        {
            if (r.kind == "image")
                refLines.Add("- reference picture \"" + r.name + "\" (image #" + JsNum(img++) +
                             (r.w && r.h ? ", full size " + JsNum(r.w) + "x" + JsNum(r.h) + " px" : String()) + ")");
            else
                refLines.Add("- reference text \"" + r.name + "\":\n<reference>\n" + JsSlice(r.text, 0, 6000) + "\n</reference>");
        }
        String refs = Join(refLines, "\n");
        bool empty = ctx.doc.root.children.empty();
        UiKit emptyKit;

        Vector<String> lines = {
            "You are the layout agent of AssetsLine's \"AI UI layout\" node. You design game and app screens as adaptive, "
            "hierarchical layouts in UIML and edit them on request. A UI kit (graphics + text styles) is laid over the layout to "
            "produce the final screen.",
            "",
            UimlSpec(),
            "",
            UiEditsSpec(),
            "",
            UiKitAdditionsSpec(ctx.canDraw),
            "",
            Rules(ctx.canDraw),
            "",
            "## UI kit",
        };
        if (num.sheet)
            lines.Add("Image #" + JsNum(num.sheet) + " is the kit sheet: every graphic below with its name under it.");
        lines.Add(KitSummary(ctx.kit ? *ctx.kit : emptyKit, DocDpi(ctx.doc)));
        lines.Add("");
        lines.Add("## References");
        lines.Add(refs.IsEmpty() ? String("(none)") : refs);
        lines.Add("");
        lines.Add("## Current layout");
        if (empty)
        {
            lines.Add("(empty) screen " + ctx.doc.name + " " + JsNum(ctx.doc.width) + "x" + JsNum(ctx.doc.height) + " dpi=" +
                      JsNum(DocDpi(ctx.doc)));
        }
        else
        {
            lines.Add(JsTrimEnd(DocToUiml(ctx.doc)));
            lines.Add("");
            lines.Add("Solved boxes at the design size (id: x,y wxh):");
            lines.Add(Boxes(ctx.doc));
        }
        if (num.shot && !empty)
        {
            lines.Add("");
            lines.Add("Image #" + JsNum(num.shot) + " is the screen as it renders now (dashed blocks have no kit graphic).");
        }
        if (!ctx.selected.IsEmpty())
        {
            lines.Add("");
            lines.Add("The user has block \"" + ctx.selected + "\" selected (\"this\", \"it\" likely mean it).");
        }
        lines.Add(PointLines(ctx));
        lines.Add("");
        lines.Add("## Conversation so far");
        lines.Add(History(ctx.steps));
        lines.Add("");
        lines.Add("## Request");
        lines.Add("<request>");
        lines.Add(JsTrim(ctx.prompt));
        lines.Add("</request>");
        lines.Add(RunLines(ctx, num));
        lines.Add("");
        lines.Add("Answer with a short message to the user (1\xE2\x80\x93" "3 sentences, " +
                  (!ctx.lang.IsEmpty() ? "in " + ctx.lang : String("in the language of the request")) +
                  ") about what you did, then exactly one fenced block:");
        lines.Add("```uiml-edit  \xE2\x80\x94 the edit commands, for changes to the current layout;");
        lines.Add("```uiml       \xE2\x80\x94 the whole screen, when building from scratch or rewriting most of it.");
        lines.Add("A question that needs no change: answer without a block.");
        return Join(lines, "\n");
    }

    String BuildReviewPrompt(const UiDoc& doc, const UiKit& kit, const String& request, const Vector<Pair<String, String>>& missing,
                             const Vector<String>& unused, const Vector<String>& glyphs)
    {
        Vector<String> missingText;
        for (auto& m : missing)
            missingText.Add(m.first + " (" + m.second + ")");
        String unusedText = Join(unused, ", ");
        String missingJoined = Join(missingText, ", ");

        Vector<String> lines = {
            "You laid out a screen from the request below with a UI kit. Now check how it uses the kit.",
            "Image #1 is the screen rendered with the kit: blocks drawn dashed have no kit graphic. Image #2 is the kit sheet: "
            "every graphic with its name.",
            "",
            UimlSpec(),
            "",
            UiEditsSpec(),
            "",
            "## UI kit",
            KitSummary(kit, DocDpi(doc)),
            "",
            "Kit graphics not used: " + (unusedText.IsEmpty() ? String("(none)") : unusedText),
            "Blocks without a kit graphic: " + (missingJoined.IsEmpty() ? String("(none)") : missingJoined),
        };
        if (!glyphs.IsEmpty())
            lines.Add("Blocks that draw an icon with characters: " + Join(glyphs, ", "));
        Vector<String> rest = {
            "",
            "Fix the kit usage only:",
            "- give a block the graphic that fits it (@update <id> kit=<name>), by the graphic's purpose (\"use: \xE2\x80\xA6\" above) "
            "and its picture on the kit sheet;",
            "- turn a character icon into an icon block with its kit graphic (@replace or @update type=icon kit=\xE2\x80\xA6 \"\" with "
            "text=-);",
            "- where nested blocks rebuild what one kit graphic already shows (panels and colours around an avatar\xE2\x80\xA6), @replace "
            "them with one block drawing that graphic;",
            "- a simple (non-sliced) graphic keeps its aspect: size its block to the graphic's proportions;",
            "- leave a block a placeholder when the kit really has nothing for it. Do not change the layout otherwise.",
            "",
            "## Current layout",
            JsTrimEnd(DocToUiml(doc)),
            "",
            "## The original request",
            "<request>",
            JsTrim(request),
            "</request>",
            "",
            "Answer with one short sentence (in the language of the request) on what you fixed and what the kit still lacks, "
            "then one ```uiml-edit block \xE2\x80\x94 or no block when nothing fits.",
        };
        lines.Add(rest);
        return Join(lines, "\n");
    }

    String BuildReviewPrompt(const UiDoc& doc, const UiKit& kit, const String& request, const UiKitReport& report)
    {
        return BuildReviewPrompt(doc, kit, request, report.missing, report.unused, report.glyphs);
    }
}
