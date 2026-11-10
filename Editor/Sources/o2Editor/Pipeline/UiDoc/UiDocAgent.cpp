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
            "Look: kit=<graphic name> picks a UI kit graphic; without it a block takes a kit graphic of its type (icons/images only by a word of their id).\n"
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
            "@screen [name=<Name>] [size=WxH]";
        return text;
    }

    // How to use the kit, then the layout rules
    static const String& Rules()
    {
        static const String text =
            "How to use the UI kit \xE2\x80\x94 the most important part:\n"
            "- The kit sheet picture shows every kit graphic with its name. Before writing, match each element of the request (and of the reference) to a kit graphic by its picture, name and description, and put kit=<exact name> on that block.\n"
            "- Kit graphics are finished pieces: a framed avatar, a button, an icon, a panel are drawn whole. Place each as ONE block with kit=\xE2\x80\xA6; never rebuild one out of nested panels and colours.\n"
            "- Icons come from the kit (kit=icon_\xE2\x80\xA6). Never draw an icon with text characters or emoji (\xE2\x99\xA5 \xE2\x98\x85 \xE2\x9A\x99 \xE2\x9C\x95 + C \xE2\x80\xA6).\n"
            "- A button graphic is the button's background; its label is the button's \"text\". Sliced (9-slice) graphics stretch to any size; simple ones keep their aspect \xE2\x80\x94 give their blocks the graphic's aspect ratio (see its size).\n"
            "- The kit's sizes are already in this screen's design pixels (its dpi): a graphic looks as drawn at that size \xE2\x80\x94 icons and pictures at about their listed size, a sliced block never smaller than its corners. Keep the screen's size and dpi.\n"
            "- What the kit has no graphic for (a wooden plate, a frame, background art\xE2\x80\xA6) stays a plain block without kit \xE2\x80\x94 a placeholder the user will fill. Don't fake its look with color=; name what is missing in your reply.\n"
            "- color= only tints a kit graphic.\n"
            "- References: take the composition, hierarchy and proportions from them; the look comes from the kit.\n"
            "\n"
            "Layout rules:\n"
            "- Coordinates are pixels of the design size. Keep 3\xE2\x80\x93" "5 % side margins; tap targets at least 88 px high at 1080 wide.\n"
            "- Build adaptively: backgrounds anchor=stretch; bars and HUD attach to their edges (top-stretch, bottom-stretch, top-right\xE2\x80\xA6); menus, lists and nav bars are column / row / grid layouts, not hand-placed blocks.\n"
            "- Keep the tree shallow: what reads as one piece is one block.\n"
            "- Ids: short, unique, snake_case, meaningful (play_button, coins_label). Keep existing ids when editing.\n"
            "- Every label must fit its box: give text blocks enough width and height for their style size.\n"
            "- Change only what the request asks; keep the rest as it is.";
        return text;
    }

    // 1-based numbers of the pictures a request sends, in order: screenshot, kit sheet, references (0 = not sent)
    struct ImageNumbers
    {
        int shot = 0, sheet = 0, firstRef = 1;
    };

    static ImageNumbers ImageNumbersOf(bool withScreenshot, bool withKitSheet)
    {
        ImageNumbers out;
        int n = 1;
        if (withScreenshot)
            out.shot = n++;
        if (withKitSheet)
            out.sheet = n++;
        out.firstRef = n;
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
        ImageNumbers num = ImageNumbersOf(ctx.withScreenshot, ctx.withKitSheet);
        int img = num.firstRef;
        Vector<String> refLines;
        for (auto& r : ctx.references)
        {
            if (r.kind == "image")
                refLines.Add("- reference picture \"" + r.name + "\" (image #" + JsNum(img++) + ")");
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
            Rules(),
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
        lines.Add("");
        lines.Add("## Conversation so far");
        lines.Add(History(ctx.steps));
        lines.Add("");
        lines.Add("## Request");
        lines.Add("<request>");
        lines.Add(JsTrim(ctx.prompt));
        lines.Add("</request>");
        lines.Add("");
        lines.Add("Answer with a short message to the user (1\xE2\x80\x93" "3 sentences, in the language of the request) about what you "
                  "did, then exactly one fenced block:");
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
            "- give a block the graphic that fits it (@update <id> kit=<name>), checking the picture on the kit sheet;",
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

    // --- the answer --------------------------------------------------------------------------------------

    namespace
    {
        // A fenced block of the answer: its language, body and byte range
        struct AnswerBlock
        {
            String lang;
            String body;
            size_t start = 0, end = 0;
        };

        bool IsWordChar(char c)
        {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
        }

        // Byte offsets where a line starts in multi-line mode (after \n, \r, U+2028, U+2029)
        std::vector<size_t> LineStarts(const std::string& s)
        {
            std::vector<size_t> out = { 0 };
            for (size_t i = 0; i < s.size(); i++)
            {
                if (s[i] == '\n' || s[i] == '\r')
                    out.push_back(i + 1);
                else if ((unsigned char)s[i] == 0xE2 && i + 2 < s.size() && (unsigned char)s[i + 1] == 0x80 &&
                         ((unsigned char)s[i + 2] == 0xA8 || (unsigned char)s[i + 2] == 0xA9))
                    out.push_back(i + 3);
            }
            return out;
        }

        // The JavaScript whitespace code point starting at i, its byte length; 0 when there is none
        size_t SpaceAt(const std::string& s, size_t i)
        {
            if (i >= s.size())
                return 0;
            unsigned char c = (unsigned char)s[i];
            int len = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : 4;
            if (i + len > s.size())
                return 0;
            std::u32string cps = Utf8ToCodePoints(String(s.substr(i, len)));
            return cps.size() == 1 && IsJsSpace(cps[0]) ? len : 0;
        }

        // /^@\w+/m: where the first such line starts, npos when none
        size_t FindCommandLine(const std::string& s)
        {
            for (size_t at : LineStarts(s))
            {
                if (at + 1 < s.size() && s[at] == '@' && IsWordChar(s[at + 1]))
                    return at;
            }
            return std::string::npos;
        }

        // /^screen\s+\S+/m: where the first such line starts, npos when none
        size_t FindScreenLine(const std::string& s)
        {
            for (size_t at : LineStarts(s))
            {
                if (s.compare(at, 6, "screen") != 0)
                    continue;
                size_t i = at + 6, spaces = 0;
                while (size_t len = SpaceAt(s, i))
                {
                    i += len;
                    spaces++;
                }
                if (spaces > 0 && i < s.size())
                    return at;
            }
            return std::string::npos;
        }

        // /^\s*@\w+/ on a body
        bool StartsWithCommand(const std::string& s)
        {
            size_t i = 0;
            while (size_t len = SpaceAt(s, i))
                i += len;
            return i + 1 < s.size() && s[i] == '@' && IsWordChar(s[i + 1]);
        }

        // text.replace(/\n{3,}/g, '\n\n')
        std::string CollapseBlankLines(const std::string& s)
        {
            std::string out;
            size_t run = 0;
            for (char c : s)
            {
                if (c == '\n')
                {
                    run++;
                    continue;
                }
                out.append(run >= 3 ? 2 : run, '\n');
                run = 0;
                out += c;
            }
            out.append(run >= 3 ? 2 : run, '\n');
            return out;
        }
    }

    UiAgentAnswer ParseAgentAnswer(const String& answerText, const UiDoc& doc)
    {
        const std::string& answer = answerText;
        std::vector<AnswerBlock> blocks;

        // /```([A-Za-z0-9_-]*)[^\n]*\n([\s\S]*?)(```|$)/g
        size_t pos = 0;
        for (;;)
        {
            size_t s = answer.find("```", pos);
            if (s == std::string::npos)
                break;
            size_t i = s + 3;
            while (i < answer.size() && (IsWordChar(answer[i]) || answer[i] == '-'))
                i++;
            String lang = JsToLower(String(answer.substr(s + 3, i - s - 3)));
            size_t nl = answer.find('\n', i);
            if (nl == std::string::npos)
                break;
            size_t bodyStart = nl + 1;
            size_t close = answer.find("```", bodyStart);
            size_t bodyEnd = close == std::string::npos ? answer.size() : close;
            size_t end = close == std::string::npos ? answer.size() : close + 3;
            blocks.push_back(AnswerBlock{ lang, String(answer.substr(bodyStart, bodyEnd - bodyStart)), s, end });
            pos = end;
        }

        std::string reply = answer;
        for (auto it = blocks.rbegin(); it != blocks.rend(); ++it)
            reply = reply.substr(0, it->start) + reply.substr(it->end);
        reply = JsTrim(String(CollapseBlankLines(reply)));

        // No fences: a bare answer that is plainly edits or a screen
        if (blocks.empty())
        {
            std::string t = JsTrim(answerText);
            bool anyCommandLine = false;
            for (auto& line : JsSplit(String(t), "\n"))
                anyCommandLine = anyCommandLine || StartsWith(JsTrim(line), "@");

            size_t firstCmd = FindCommandLine(t);
            size_t screenAt = FindScreenLine(t);
            if (firstCmd != std::string::npos && anyCommandLine)
            {
                blocks.push_back(AnswerBlock{ "uiml-edit", String(t.substr(firstCmd)), 0, 0 });
                reply = JsTrim(String(t.substr(0, firstCmd)));
            }
            else if (screenAt != std::string::npos)
            {
                blocks.push_back(AnswerBlock{ "uiml", String(t.substr(screenAt)), 0, 0 });
                reply = JsTrim(String(t.substr(0, screenAt)));
            }
        }

        UiAgentAnswer result;
        const AnswerBlock* block = nullptr;
        for (auto& b : blocks)
        {
            if (((const std::string&)b.lang).find("edit") != std::string::npos || StartsWithCommand(b.body))
            {
                block = &b;
                break;
            }
        }
        if (!block)
        {
            for (auto& b : blocks)
            {
                const std::string& l = b.lang;
                if (l.find("ui") != std::string::npos || l.find("text") != std::string::npos || l.empty())
                {
                    block = &b;
                    break;
                }
            }
        }

        if (block)
        {
            if (((const std::string&)block->lang).find("edit") != std::string::npos || StartsWithCommand(block->body))
            {
                UimlEditsResult res = ParseEdits(block->body, doc);
                result.ops = res.ops;
                result.errors = res.errors;
            }
            else
            {
                UimlParseResult res = ParseUiml(block->body, &doc);
                result.ops.Add(MakeDocOp(res.doc));
                result.errors = res.errors;
            }
        }

        // Report edits that will not apply, so the step says so
        if (!result.ops.IsEmpty())
            result.errors.Add(ApplyOps(doc, result.ops).errors);

        result.reply = !reply.empty() ? String(reply) : !result.ops.IsEmpty() ? String("Done.") : String();
        return result;
    }
}
