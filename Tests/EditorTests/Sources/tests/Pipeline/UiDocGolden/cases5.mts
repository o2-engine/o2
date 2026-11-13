// Golden references, round 5 (AssetsLine 2026-11-12): turned layout blocks (shared/uiDoc/rotate.ts) - the turn as a property
// (cleanTurn, UIML rot= / angle= / rotate=, edits, the root never turned), the maths (turnedBounds, unturnedSize, worldMatrix,
// worldTurn, corners, inBlock), the hit test through turns, ui_structure.json's `rotation`, the Unity prefab of a turned
// block (pivot to the centre, the quaternion and the angle hint; Expand of the Canvas Scaler), keepTurns after the skill
// pass and the markup's rule for tilted elements.
// The backend modules load its billing database: no-sqlite.mjs stands in for better-sqlite3 (a Linux build on the host).
//   AL=<AssetsLine checkout>; ASSETSLINE=$AL $AL/backend/node_modules/.bin/tsx --import ./no-sqlite.mjs cases5.mts /tmp/golden5.json
//   python3 gen_tests.py /tmp/golden5.json ../UiDocGolden5.inc kUiDocGolden5Parts
import fs from 'node:fs';
const A = process.env.ASSETSLINE ?? '/Users/andreizenkovich/work/zenkovich.space/AssetsLine';
const rotate = await import(`${A}/shared/uiDoc/rotate.ts`);
const uiml = await import(`${A}/shared/uiDoc/uiml.ts`);
const model = await import(`${A}/shared/uiDoc/model.ts`);
const ops = await import(`${A}/shared/uiDoc/ops.ts`);
const solve = await import(`${A}/shared/uiDoc/solve.ts`);
const structure = await import(`${A}/shared/uiDoc/structure.ts`);
const fromStructure = await import(`${A}/shared/uiDoc/fromStructure.ts`);
const unity = await import(`${A}/shared/uiExport/unity.ts`);
const agentRun = await import(`${A}/backend/src/uiLayout/agentRun.ts`);

const out: Record<string, unknown> = {};
type J = Record<string, unknown>;

// --- the turn as it is stored ----------------------------------------------------------------------------------------
const VALUES: unknown[] = [370, -190, 180, -180, 0.04, -0.04, 0.05, -0.05, '12.345', 'x', '', ' 7 ', null, true, false, 359.96, -179.96,
  179.96, 180.04, 540, -540, 1e9, 33, -12, 12.25, 12.35, -12.25, -12.35, 720.5, '1e2', [5], {}];
out.cleanTurn = VALUES.map((v) => [v, rotate.cleanTurn(v) ?? null]);

// --- the maths ---------------------------------------------------------------------------------------------------------
const ANGLES = [0, 1, 12, -12, 15, 30, -30, 34.9, 35, 45, 60, 89.5, 90, -90, 120, 135, 170, 180, -180, -170];
out.bounds = [];
for (const [w, h] of [[100, 100], [120, 60], [40, 300], [1, 1]]) {
  for (const deg of ANGLES) (out.bounds as unknown[]).push([w, h, deg, rotate.turnedBounds(w, h, deg)]);
}
out.unturned = [];
for (const [W, H] of [[136.6, 136.6], [130, 89], [54.8, 54.8], [80, 50], [300, 40], [200, 210]]) {
  for (const deg of ANGLES) {
    for (const aspect of [undefined, 1, 2, 0.5, 0]) (out.unturned as unknown[]).push([W, H, deg, aspect ?? null, rotate.unturnedSize(W, H, deg, aspect)]);
  }
}

// Documents with turns: nested, in a row, hidden, near the edges
const DOCS = [
  ['screen S 400x400', '  panel card anchor=center size=200x100 rotation=90', '    button ok anchor=top-left pos=10,10 size=40x20 "OK"',
    '  image tile anchor=top-left pos=300,300 size=60x60 rot=-15'].join('\n'),
  ['screen N 600x800', '  panel outer anchor=center size=400x300 angle=30', '    panel inner anchor=bottom-right pos=20,20 size=160x120 rotation=-45',
    '      icon star anchor=center size=40x40 rotate=200', '      text label anchor=top-stretch margin=4 h=30 "Hi"',
    '    button go anchor=top-left pos=10,10 size=120x50 "Go"', '  row bar anchor=bottom-stretch margin=10 h=80 gap=8',
    '    icon a size=60x60 rotation=12.5', '    icon b size=60x60 rotation=-170', '    icon c size=60x60 hidden rotation=45',
    '  image corner anchor=top-right pos=-30,-30 size=100x40 rotation=75'].join('\n'),
];
out.docs = DOCS.map((src) => {
  const doc = uiml.parseUiml(src).doc;
  const rects = solve.solveLayout(doc);
  const ids: string[] = [];
  model.walk(doc.root, (n: { id: string }) => { ids.push(n.id); });
  const blocks = ids.map((id) => {
    const r = rects.get(id)!;
    return {
      id, rect: r, m: rotate.worldMatrix(doc, rects, id), mParent: rotate.worldMatrix(doc, rects, id, false),
      turn: rotate.worldTurn(doc, id), turnParent: rotate.worldTurn(doc, id, false),
      corners: rotate.corners(r, rotate.worldMatrix(doc, rects, id)),
    };
  });
  // Points off the grid lines so no edge is met exactly
  const hits: unknown[] = [];
  for (let y = 3.37; y < doc.height; y += 13.1) {
    for (let x = 2.71; x < doc.width; x += 12.3) hits.push([x, y, solve.hitTest(doc, rects, x, y) ?? '']);
  }
  const inside: unknown[] = [];
  for (const id of ids.filter((i) => i !== 'root')) {
    for (let y = 1.13; y < doc.height; y += 37.7) {
      for (let x = 1.91; x < doc.width; x += 41.3) if (rotate.inBlock(doc, rects, id, { x, y })) inside.push([id, x, y]);
    }
  }
  return { src, doc: JSON.stringify(doc), uiml: uiml.docToUiml(doc), blocks, hits, inside };
});

// --- the text form, normalising, edits ----------------------------------------------------------------------------------
out.normalize = [
  { name: 's', width: 100, height: 100, root: { id: 'root', type: 'screen', rotation: 45, children: [] } },
  { name: 's', width: 100, height: 100, root: { id: 'root', type: 'screen', angle: 45, children: [{ id: 'a', type: 'panel', angle: 33 }] } },
  { root: { children: [{ id: 'a', type: 'panel', rot: '-370', rotation: null }, { id: 'b', type: 'icon', rotate: 0.04 }, { id: 'c', rotation: 'x', rot: 10 },
    { id: 'd', type: 'image', w: 10, rotation: 90, grow: 2, opacity: 0.5, angle: 5 }] } },
].map((raw) => [JSON.stringify(raw), JSON.stringify(model.normalizeDoc(raw))]);
const editDoc = uiml.parseUiml(DOCS[0]).doc;
out.edits = [
  '@update tile rotation=30\n@update card rotation=-',
  '@update tile rot=-',
  '@update ok angle=370 size=50x20',
  '@update root rotation=45',
  '@add card\n  badge new_badge anchor=top-right size=30x30 rotation=-20 "3"',
  '@replace tile\n  image tile2 anchor=center size=10x10 rotate=181',
].map((text) => {
  const e = uiml.parseEdits(text, editDoc);
  const after = ops.applyOps(editDoc, e.ops);
  return { text, ops: JSON.stringify(e.ops), errors: e.errors, doc: JSON.stringify(after.doc), uiml: uiml.docToUiml(after.doc) };
});

// --- the structure, back, the Unity prefab -------------------------------------------------------------------------------
out.structure = DOCS.map((src) => {
  const doc = uiml.parseUiml(src).doc;
  const st = structure.toUiStructure(doc, null);
  const back = fromStructure.fromUiStructure({ structure: JSON.parse(st.json), kit: null, assetNames: new Map(), dpi: 480 } as never);
  return { src, json: st.json, back: JSON.stringify(back.doc) };
});
out.unity = [];
for (const [i, src] of DOCS.entries()) {
  for (const canvasFit of [undefined, 'expand'] as const) {
    const doc = uiml.parseUiml(src).doc;
    const res = await unity.exportUnity({ name: doc.name, folder: 'Generated/UI', layout: structure.toUiStructure(doc, null).json, sprites: [], fonts: [], seed: `t${i}`, ...(canvasFit ? { canvasFit } : {}) });
    (out.unity as unknown[]).push({ src, canvasFit: canvasFit ?? '', prefab: new TextDecoder().decode(res.files.find((f: { path: string }) => f.path.endsWith('.prefab'))!.data), warnings: res.warnings });
  }
}

// --- the agent: keepTurns, the markup's rule ------------------------------------------------------------------------------
const draftDoc = uiml.parseUiml(['screen S 400x400', '  image tile_a anchor=top-left pos=40,40 size=40x40 kit=tile rotation=-30',
  '  image tile_b anchor=top-left pos=140,40 size=40x40 kit=tile rotation=15', '  panel box anchor=center size=100x100 rotation=5',
  '    text t anchor=center size=50x20 rotation=-8 "A"'].join('\n')).doc;
out.keepTurns = [
  'screen S 400x400\n  image tile_a anchor=top-left pos=110,110 size=110x110 kit=tile\n  image other size=10x10',
  'screen S 400x400\n  image tile_b size=10x10 rotation=40\n  panel box size=100x100\n    text t size=50x20 "A"\n  image tile_a size=1x1 opacity=0.5',
  'screen S 400x400\n  image nothing size=10x10',
].map((src) => {
  const doc = uiml.parseUiml(src).doc;
  return { src, draft: JSON.stringify(draftDoc), doc: JSON.stringify(agentRun.keepTurns(doc, draftDoc)) };
});
const markupSrc = fs.readFileSync(`${A}/backend/src/uiLayout/markup.ts`, 'utf8');
const flat = /const FLAT_RULE = (\[[\s\S]*?\])\.join\('\\n'\);/.exec(markupSrc);
if (!flat) throw new Error('FLAT_RULE not found in markup.ts');
out.flatRule = (new Function(`return ${flat[1]};`)() as string[]).join('\n');

fs.writeFileSync(process.argv[2], JSON.stringify(out));
console.log('ok', Object.keys(out).length, JSON.stringify(out).length);
