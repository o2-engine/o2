// Golden references, round 6 (AssetsLine ea79bbc, 2026-11-17): a text style's effects as a layer stack - the canonical
// order (stackEffects), where "add" puts a new effect (insertEffect), fxStack written only for a list in an order of its
// own, merges and scaling that keep it, the model never shown it - the gradient outline (normalized, as JSON, in the
// look, in the style check's changes) and ui_structure.json's text extras (gradient, effects, letterSpacing) and `fit`.
// The backend modules load its billing database: no-sqlite.mjs stands in for better-sqlite3 (a Linux build on the host).
//   AL=<AssetsLine checkout>; ASSETSLINE=$AL $AL/backend/node_modules/.bin/tsx --import ./no-sqlite.mjs cases6.mts /tmp/golden6.json
//   python3 gen_tests.py /tmp/golden6.json ../UiDocGolden6.inc kUiDocGolden6Parts
import fs from 'node:fs';
const A = process.env.ASSETSLINE ?? '/Users/andreizenkovich/work/zenkovich.space/AssetsLine';
const ts = await import(`${A}/shared/textStyle.ts`);
const uiml = await import(`${A}/shared/uiDoc/uiml.ts`);
const structure = await import(`${A}/shared/uiDoc/structure.ts`);
const kitm = await import(`${A}/shared/uiKit.ts`);
const check = await import(`${A}/backend/src/uiKit/styleCheck.ts`);

const out: Record<string, unknown> = {};

// --- the stack ----------------------------------------------------------------------------------------------------------
const S = { type: 'stroke', color: '#111111', width: 3 };
const S2 = { type: 'stroke', color: '#222222', width: 5 };
const E = { type: 'extrude', color: '#333333', y: 6 };
const SH = { type: 'shadow', color: '#444444', y: 4, blur: 6 };
const G = { type: 'glow', color: '#555555', size: 9 };
const IN = { type: 'innerShadow', color: '#666666', y: 2 };
const B = { type: 'bevel', depth: 3 };
const LISTS: unknown[][] = [
  [], [S], [SH, S], [G, E, S2, SH, S], [IN, SH, B, S, E], [B, IN], [SH, IN, G, S, B, S2],
  [E, { ...S, off: true }, S2], [S, E, SH, G, IN, B], [IN, B, S, E, SH, G], [G, SH], [SH, G],
];
out.stack = LISTS.map((list) => {
  const fx = ts.normalizeEffects(list);
  return { list: JSON.stringify(list), stacked: JSON.stringify(ts.stackEffects(fx)), inner: fx.map((e: any) => ts.isInnerEffect(e)) };
});
out.insert = LISTS.map((list) => {
  const fx = ts.normalizeEffects(list);
  return ts.TEXT_EFFECT_TYPES.map((type: string) => JSON.stringify(ts.insertEffect(fx, { ...ts.defaultEffect(type, ts.TEXT_STYLE_DEFAULT), ...(type === 'bevel' ? {} : { color: '#ABCDEF' }) })));
});

// --- styles -------------------------------------------------------------------------------------------------------------
const GRAD = { angle: 45, stops: [{ color: '#ff0000', at: 0 }, { color: '#00ff00', at: 0.5 }, { color: '#0000FF', at: 1 }] };
const STYLE_RAWS: unknown[] = [
  { name: 'canon', effects: [SH, S, IN, E] },
  { name: 'own', effects: [SH, S, IN, E], fxStack: true },
  { name: 'own_canonical', effects: [S, E, SH], fxStack: true },
  { name: 'own_string', effects: [SH, S], fxStack: 'true' },
  { name: 'own_no_list', fxStack: true, stroke: { color: '#000', width: 2 }, shadow: { y: 3 } },
  { name: 'grad_outline', effects: [{ type: 'stroke', color: '#123456', width: 4, fill: GRAD }, { type: 'stroke', width: 2, fill: { stops: ['#fff'] } }] },
  { name: 'grad_colors', effects: [{ type: 'outline', size: 3, opacity: 0.5, fill: { colors: ['#fa0', '#a0f'] }, off: true }] },
  { name: 'grad_bad', effects: [{ type: 'stroke', width: 3, color: '#abc', fill: 'red' }, { type: 'stroke', width: 3, fill: null }] },
  { name: 'shadow_over', size: 80, fill: { stops: ['#fff', '#fc0'] }, effects: [SH, { type: 'stroke', width: 6, fill: GRAD }, S2, B], fxStack: true },
];
const styles = STYLE_RAWS.map((r) => ts.normalizeTextStyle(r, 'fb'));
out.styles = STYLE_RAWS.map((r, i) => ({ raw: r, style: JSON.stringify(styles[i]) }));
out.looks = styles.map((s) => ts.styleLook(s));
out.scaled = styles.flatMap((s) => [0.5, 2.731].map((k) => JSON.stringify(ts.scaleTextStyle(s, k))));
out.reach = styles.map((s) => ts.effectsReach(s));

const MERGES: Record<string, unknown>[] = [
  { effects: [SH, S] },
  { effects: [SH, S], fxStack: true },
  { effects: [S, SH] },
  { stroke: { color: '#ff00ff', width: 3 } },
  { shadow: null },
  { color: '#00ff00', fxStack: true },
  { effects: [{ type: 'stroke', width: 2, fill: GRAD }, G] },
];
out.merges = [0, 1, 8].flatMap((cur) => MERGES.map((m) => ({ cur, raw: m, style: JSON.stringify(ts.mergeTextStyle(styles[cur], m)) })));

const CONFIGS: Record<string, unknown>[] = [
  { name: 'Card Title', effects: [SH, S], fxStack: true, uiThing: 1 },
  { name: 'x', effects: [SH, S], fxStack: null },
  { name: 'y', effects: [S, SH], fxStack: true },
];
out.configs = CONFIGS.map((c) => ({ config: c, style: JSON.stringify(ts.textStyleFromConfig(c)) }));

// The model is shown the stack without fxStack
out.prompt = ts.buildStylePrompt(styles[8], 'more gold', 0);

// The style check's description of changes
out.changes = [
  check.styleChanges(styles[0], styles[5]),
  check.styleChanges(styles[5], styles[8]),
  check.styleChanges(styles[1], styles[0]),
];

// --- ui_structure.json --------------------------------------------------------------------------------------------------
{
  const png = 'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8DwHwAFBQIAX8jx0gAAAABJRU5ErkJggg==';
  const kit = kitm.parseKit(JSON.stringify({ name: 'k', elements: [
    { name: 'icon_star', type: 'icon', assetType: 'simple', width: 64, height: 64, png },
    { name: 'bg_sky', type: 'background', assetType: 'simple', width: 64, height: 96, png },
    { name: 'panel_wood', type: 'panel', assetType: 'sliced', width: 96, height: 96, nineSlice: { left: 16, top: 16, right: 16, bottom: 16 }, png },
    { name: 'pic_tile', type: 'image', assetType: 'tiled', width: 32, height: 32, png },
  ], textStyles: [
    { name: 'title', role: 'title', font: 'Lilita One', size: 96, letterSpacing: 3, fill: GRAD, effects: [SH, { type: 'stroke', width: 6, fill: GRAD }, S2, { ...B, off: true }], fxStack: true },
    { name: 'body', role: 'body', size: 40, effects: [{ type: 'glow', size: 7, opacity: 0.4 }] },
  ] }));
  const SRC = [
    'screen Fit 1080x1920',
    '  background sky anchor=stretch kit=bg_sky',
    '  icon star anchor=center size=200x100 kit=icon_star',
    '  icon star_cover anchor=top-left size=100x200 kit=icon_star fit=cover',
    '  icon star_stretch anchor=top-right size=100x200 kit=icon_star fit=stretch',
    '  panel wood anchor=bottom-stretch h=300 kit=panel_wood fit=contain',
    '  image tile anchor=top-stretch pos=0,300 h=200 kit=pic_tile fit=cover',
    '  image plain anchor=top-left pos=0,600 size=300x300 kit=bg_sky',
    '  text title anchor=top-stretch pos=40,100 h=160 style=title "GOLDEN TITLE"',
    '  text tinted anchor=top-stretch pos=40,300 h=160 style=title textColor=#00FFAA "TINTED"',
    '  text small anchor=top-left pos=40,500 size=200x60 style=title "A long title that shrinks"',
    '  text body anchor=top-stretch pos=40,700 h=80 "Body text"',
  ].join('\n');
  const doc = uiml.parseUiml(SRC).doc;
  const r = structure.toUiStructure(doc, kit);
  out.structure = { src: SRC, kit: JSON.stringify(kit), json: JSON.stringify(JSON.parse(r.json)), warnings: r.warnings };
}

fs.writeFileSync(process.argv[2], JSON.stringify(out));
console.log(Object.keys(out).join(', '));
