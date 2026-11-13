// Golden references, round 4 (AssetsLine after 77d5b09): the layout agent's run - the markup's kit rules, styles prompt and
// styles answer (backend/src/uiLayout/markup.ts), the skill's first draft and task note (skillBuild.ts), the check of an edit
// (agentRun.ts checkFindings), the measuring grid's step and the blocks of a place (compare.ts).
// The backend modules load its billing database: no-sqlite.mjs stands in for better-sqlite3 (a Linux build on the host).
//   AL=<AssetsLine checkout>; ASSETSLINE=$AL $AL/backend/node_modules/.bin/tsx --import ./no-sqlite.mjs cases4.mts /tmp/golden4.json
//   python3 gen_tests.py /tmp/golden4.json ../UiDocGolden4.inc kUiDocGolden4Parts
import fs from 'node:fs';
const A = process.env.ASSETSLINE ?? '/Users/andreizenkovich/work/zenkovich.space/AssetsLine';
const markup = await import(`${A}/backend/src/uiLayout/markup.ts`);
const skill = await import(`${A}/backend/src/uiLayout/skillBuild.ts`);
const agentRun = await import(`${A}/backend/src/uiLayout/agentRun.ts`);
const compare = await import(`${A}/backend/src/uiLayout/compare.ts`);
const kitm = await import(`${A}/shared/uiKit.ts`);
const uiml = await import(`${A}/shared/uiDoc/uiml.ts`);
const ts = await import(`${A}/shared/textStyle.ts`);

const out: Record<string, unknown> = {};
type J = Record<string, unknown>;

// --- the kit -------------------------------------------------------------------------------------------------------
const el = (name: string, type: string, extra: J = {}) => ({ name, type, assetType: kitm.defaultAssetType(type), width: 200, height: 100, png: 'AAAA', ...extra });
const KIT = { version: 1, name: 'golden4', device: { id: 'iphone-16', name: 'iPhone 16', w: 1179, h: 2556, dpi: 460 }, elements: [
  el('btn_green', 'button', { width: 300, height: 100, dpi: 460, nineSlice: { left: 30, top: 20, right: 30, bottom: 24 }, purpose: 'the main action button of a popup' }),
  el('btn_blue_round', 'button', { assetType: 'simple', width: 120, height: 120, dpi: 920 }),
  el('panel_wood', 'panel', { width: 600, height: 400, dpi: 230, nineSlice: { left: 40, top: 40, right: 40, bottom: 40 }, description: 'a wooden board with nails in the corners' }),
  el('icon_coin', 'icon', { width: 64, height: 64, purpose: 'currency counter icon in the top bar of the screen and the shop' }),
  el('bar_track', 'progressBar', { width: 400, height: 40, nineSlice: { left: 12, top: 0, right: 12, bottom: 0 } }),
  el('bg_menu', 'background', { width: 1080, height: 1920 }),
  el('cut_banner', 'image', { assetType: 'simple', width: 500, height: 120, own: true, purpose: 'a cut', description: 'the banner' }),
], textStyles: [
  { name: 'title', role: 'title', font: 'Lilita One', size: 96, color: '#FFE45C', stroke: { color: '#5A2A00', width: 6 }, description: 'big headers' },
  { name: 'button', role: 'button', font: 'Fira Sans', weight: 900, size: 48, color: '#FFFFFF', stroke: { color: '#1A4A00', width: 4 } },
  { name: 'body', role: 'body', font: 'Roboto', size: 36, color: '#FFFFFF', shadow: { color: '#000000', x: 0, y: 3, blur: 4, opacity: 0.5 } },
] };
const KIT_JSON = JSON.stringify(KIT);
out.kit = KIT_JSON;
const kit = kitm.parseKit(KIT_JSON);
// parseKit drops `own`: the own graphic is marked again as the layout node marks it
for (const e of kit.elements) if (e.name === 'cut_banner') (e as J).own = true;
const EMPTY_KIT = kitm.parseKit(JSON.stringify({ name: 'empty', elements: [], textStyles: [] }));

// --- the markup ----------------------------------------------------------------------------------------------------
out.kitRules = [
  { dpi: 460, sheet: 2, text: markup.kitMarkupRules(kit, 460, 2) },
  { dpi: 480, sheet: 3, text: markup.kitMarkupRules(kit, 480, 3) },
];
const LABELS = [
  { text: 'VICTORY!', box: { x: 120.4, y: 80.5, w: 840, h: 140 } },
  { text: 'Level  12\ncomplete', box: { x: 300, y: 400, w: 480.6, h: 90 } },
  { text: '1 250', box: { x: 700, y: 30, w: 160, h: 50 } },
  { text: 'A very long label that goes on and on past forty characters for sure', box: { x: 10, y: 10, w: 1000, h: 40 } },
  { text: 'Кнопка «ОК»', box: { x: 400, y: 1800, w: 280, h: 70 } },
];
const MANY = Array.from({ length: 83 }, (_, i) => ({ text: `L${i}`, box: { x: i, y: i * 2, w: 10 + i, h: 5 } }));
out.stylesPrompt = [
  { labels: LABELS, kit: true, w: 1179, h: 2112, text: markup.stylesPrompt(LABELS, kit, 1179, 2112) },
  { labels: LABELS, kit: false, w: 768, h: 1376, text: markup.stylesPrompt(LABELS, null, 768, 1376) },
  { labels: MANY, kit: false, w: 100, h: 200, text: markup.stylesPrompt(MANY, EMPTY_KIT, 100, 200) },
];
const ANSWERS = [
  '```json\n{"styles":[{"name":"title","role":"title","font":"Lilita One","size":90,"color":"#ffe45c"},{"name":"counter","font":"Russo One","size":40,"color":"#FFD700","stroke":{"color":"#000","width":3}}],"labels":{"1":"title","2":"Body","3":"counter","4":"nope","5":"button","9":"title","x":"title","0":"title"}}\n```',
  '{"styles":[{"name":"a"},{"name":"A"},{"name":"a_2"},{"name":"b","effects":[{"type":"stroke","width":4},{"type":"glow","size":6}],"fill":{"stops":["#fff","#f80"],"angle":90}}],"labels":{"1":"a","2":"A","3":"a_2","4":"b","5":""}}',
  'not json at all',
  '{"styles":"none","labels":[1,2]}',
  '{"styles":[' + Array.from({ length: 14 }, (_, i) => `{"name":"s${i}","size":${10 + i}}`).join(',') + '],"labels":{"1.4":"s1","2.6":"s13","5":null}}',
];
out.parseStyles = ANSWERS.flatMap((answer) => [true, false].map((withKit) => {
  const r = markup.parseStyles(answer, withKit ? kit : null, 5);
  return { answer, kit: withKit, styles: JSON.stringify(r.styles), byLabel: r.byLabel.map((x: string | undefined) => x ?? null) };
}));

// --- the skill's first draft and note --------------------------------------------------------------------------------
const DRAFT_UIML = [
  '@screen Victory 1179x2112 dpi=460',
  'background backdrop anchor=stretch kit=bg_menu',
  'panel board anchor=center w=900 h=1200 kit=panel_wood',
  '  text title anchor=top-center y=40 w=800 h=140 style=title "VICTORY!"',
  '  button ok anchor=bottom-center y=60 w=420 h=140 kit=btn_green style=button "OK"',
  '  button round anchor=top-right x=20 y=20 w=120 h=120 kit=btn_blue_round',
  '  progress xp anchor=top-center y=400 w=700 h=60 kit=bar_track value=0.4',
  '  icon coin anchor=top-left x=40 y=600 w=80 h=80 kit=icon_coin',
  '  image banner anchor=top-center y=200 w=500 h=120 kit=cut_banner',
  '  text count anchor=top-left x=140 y=600 w=300 h=80 style=body "1 250"',
].join('\n');
const draftDoc = uiml.parseUiml(DRAFT_UIML).doc;
out.draftUiml = DRAFT_UIML;
out.seed = [460, 920, 480].map((dpi) => ({ dpi, text: skill.seedStructure(draftDoc, kit, dpi) }));
out.seedNoKit = skill.seedStructure(draftDoc, null, 460);
const inputs = (own: string[], kitFiles: string[]) => ({ dirs: {}, assetNames: new Map(), fonts: [], seed: '', kitFiles, ownFiles: own });
out.skillNote = [
  { own: ['cut_banner.png', 'bg_backdrop.png'], styles: true, text: skill.skillNote(inputs(['cut_banner.png', 'bg_backdrop.png'], ['btn_green.png']) as any, kit, kit.textStyles) },
  { own: [], styles: false, text: skill.skillNote(inputs([], []) as any, kit, []) },
  { own: [], styles: true, text: skill.skillNote(inputs([], []) as any, null, [ts.normalizeTextStyle({ name: 'x', font: 'Bangers', weight: 400, size: 30, color: '#123456' }, 'x')]) },
];

// --- the check of an edit ------------------------------------------------------------------------------------------
const CHECK_DOCS = [
  DRAFT_UIML,
  ['@screen S 1000x1000', 'panel p x=10 y=10 w=300 h=300', 'button b x=900 y=900 w=300 h=300 "Go"', 'text t x=-500 y=10 w=200 h=40 "★"', 'icon gear x=10 y=400 w=80 h=80 "⚙"'].join('\n'),
  ['@screen S 1000x1000', 'panel p x=10 y=10 w=300 h=300 kit=panel_wood', 'button b x=100 y=500 w=300 h=100 kit=btn_green "Go"'].join('\n'),
  ['@screen S 400x400', ...Array.from({ length: 30 }, (_, i) => `panel off${i} x=${500 + i} y=0 w=10 h=10`)].join('\n'),
];
out.findings = CHECK_DOCS.flatMap((text) => [kit, null].map((k) => ({ uiml: text, kit: !!k, findings: agentRun.checkFindings(uiml.parseUiml(text).doc, k) })));

// --- the grid and the blocks of a place ----------------------------------------------------------------------------
out.gridSteps = [[1179, 2112], [1080, 1920], [300, 200], [768, 1376], [5000, 3000], [20000, 100], [64, 64], [1200, 1200]]
  .map(([w, h]) => ({ w, h, step: compare.gridStep(w, h) }));
const placeDoc = uiml.parseUiml(DRAFT_UIML).doc;
const BOXES = [{ x: 0, y: 0, w: 1179, h: 2112 }, { x: 140, y: 400, w: 900, h: 300 }, { x: 600, y: 1700, w: 100, h: 100 }, { x: 5000, y: 0, w: 10, h: 10 }, { x: 180, y: 520, w: 400, h: 120 }];
out.blocksIn = BOXES.flatMap((box) => [6, 2].map((max) => ({ box, max, blocks: compare.blocksIn(placeDoc, box, max) })));

fs.writeFileSync(process.argv[2] ?? '/tmp/golden4.json', JSON.stringify(out));
console.log('golden4 written');
