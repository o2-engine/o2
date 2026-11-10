// Golden references for the o2 port of AssetsLine's UI layout (shared/uiDoc, uiKit, textStyle, backend/src/uiKit/prep):
// it imports the web's TypeScript as is (read-only) and writes golden.json, every case's inputs and the web's outputs;
// gen_tests.py embeds it into the o2 tests (../UiDocGolden.inc). To regenerate after the web changes:
//   AL=<AssetsLine checkout>; ASSETSLINE=$AL $AL/backend/node_modules/.bin/tsx cases.mts /tmp/golden.json
//   python3 gen_tests.py /tmp/golden.json ../UiDocGolden.inc
import fs from 'node:fs';
import crypto from 'node:crypto';
import { createRequire } from 'node:module';
const A = process.env.ASSETSLINE ?? '/Users/andreizenkovich/work/zenkovich.space/AssetsLine';
const sharp = createRequire(`${A}/backend/package.json`)('sharp');
const uiml = await import(`${A}/shared/uiDoc/uiml.ts`);
const ops = await import(`${A}/shared/uiDoc/ops.ts`);
const model = await import(`${A}/shared/uiDoc/model.ts`);
const solve = await import(`${A}/shared/uiDoc/solve.ts`);
const edit = await import(`${A}/shared/uiDoc/edit.ts`);
const render = await import(`${A}/shared/uiDoc/render.ts`);
const agent = await import(`${A}/shared/uiDoc/agent.ts`);
const structure = await import(`${A}/shared/uiDoc/structure.ts`);
const device = await import(`${A}/shared/uiDoc/device.ts`);
const kitm = await import(`${A}/shared/uiKit.ts`);
const ts = await import(`${A}/shared/textStyle.ts`);
const prep = await import(`${A}/backend/src/uiKit/prep.ts`);

const out: Record<string, unknown> = {};
const U = '__undef__';
const j = (v: unknown) => (v === undefined ? U : JSON.parse(JSON.stringify(v)));

// --- UIML ------------------------------------------------------------------------------------
const SCREEN = `screen Menu 1080x1920
  background bg anchor=stretch kit=bg_plain
  text title anchor=top-stretch pos=0,160 h=140 margin=0,60 style=title "FOREST QUEST"
  column menu anchor=center size=640x600 gap=30
    button play h=140 kit=btn_red "PLAY"
    button settings h=120 "SETTINGS"
    row coins h=80 gap=16 justify=center align=center
      icon coin size=64x64
      text amount w=220 "1 250"
  row bar anchor=bottom-stretch h=180 pad=20 gap=20
    button a grow=1 "A"
    button b grow=1 "B"
  button close anchor=top-right pos=36,36 size=96x96
  grid slots anchor=bottom-stretch pos=0,220 h=360 margin=0,40 cols=4 gap=16
    slot s1
    slot s2
    slot s3
    slot s4
    slot s5
`;
const SCREEN2 = `screen S 1000x2000
  panel box anchor=center size=600x400
    button corner anchor=bottom-right pos=20,30 size=100x50
    text bar anchor=top-stretch pos=0,10 h=40 margin=0,15,0,25
  row r anchor=bottom-stretch pos=0,100 h=120 margin=0,50 gap=10
    button a w=100
    button b w=100
    button c grow=1
`;
const DPI = 'screen Menu 1179x2556 dpi=460 layout=column\n  button play "Play"';
const TRICKY = [
  '# comment line',
  'screen Shop 720x1280 dpi=320 pad=10 layout=grid cols=3 gap=8,12 "root text"',
  '  card c1 anchor="top left" pos=10,20 size=200×100 kit=Panel_Blue! variant="  Primary  " color=f80 opacity=0.555 note="  a note  "',
  '    label l1 "Hello" “curly” «guillemets» textAlign=CENTER font-size=33.7 text-color=#abc',
  '    btn b1 hidden 300x80 margin=1,2,3 grow=-1',
  '  row r1 padding="4 8" spacing=6 justify=space-between align=middle',
  '    icon coin_icon size=64*64',
  '    text t1 "line1\\nline2 \\"quoted\\" back\\\\slash"',
  '    progressbar p value=0.12345 w=abc',
  '    checkbox t on=yes',
  '  hbox h2 cell=50x60 cols=2 gapY=5',
  '\tslot s_tab',
  '  frame f1 anchor=bottom fit=COVER placeholder="type here" fontSize=500',
  '  foo bar',
  '  "quoted first"',
  '  image img1 x=1.005 y=-2.345 w=0.001 h=10.004 margin=0.004 pad=1,2',
  '  grid g2 anchor=stretch margin=5,6,7,8 cellW=100 gap=10 align=end justify=center',
  '    slot q1 x=3',
  '    slot q2 w=40 h=30',
  '    slot q3 hidden',
  '    slot q4',
  '  column col2 anchor=middle-right size=300x500 pad=5,10,15,20 gap=4 justify=evenly align=end',
  '    button k1 h=40 "K1"',
  '    tab k2 w=50 h=60 y=4',
  '    badge k3 grow=2 "9"',
  '  row row3 anchor=top-center pos=0,40 size=900x100 gap=10 justify=around',
  '    toggle tg1 w=100',
  '    slider sl1 grow=1 value=1.5',
  '    input in1 w=200 placeholder="Name"',
  '    divider dv1',
  '  row row4 anchor=center size=600x80 justify=end align=start',
  '    icon ic1 size=40x40',
  '    icon ic2 size=40x40',
  '  row row5 anchor=center pos=0,300 size=600x80 justify=between',
  '    icon ic3 w=40',
  '  column col6 anchor=bottom-left pos=10,10 size=200x300 justify=center',
  '    text tx6 "x"',
  '    text tx7 "y" hidden',
].join('\n');
const NOSCREEN = '  button a "A"\n    icon i1\n  text b "B"';
const CYR = 'screen Экран 1080x1920\n  text заголовок "Привет, мир"\n  button кнопка_1 "ОК"';
const DUPS = 'screen S 100x100\n  button a\n  button a\n  icon a\n  group\n  group\n  row\n  button a_2\n  button 9x9\n  text t "a" "b"';

const uimlCases: { name: string; src: string; base?: Record<string, unknown> }[] = [
  { name: 'screen', src: SCREEN }, { name: 'screen2', src: SCREEN2 }, { name: 'dpi', src: DPI }, { name: 'tricky', src: TRICKY },
  { name: 'noscreen', src: NOSCREEN }, { name: 'noscreen_base', src: NOSCREEN, base: { name: 'Base', width: 640, height: 480, dpi: 240 } },
  { name: 'cyr', src: CYR }, { name: 'dups', src: DUPS }, { name: 'empty', src: '' }, { name: 'crlf', src: 'screen W 300x200\r\n  button x\r\n    icon y\r  text z "t"' },
];
out.uiml = uimlCases.map((c) => {
  const { doc, errors } = uiml.parseUiml(c.src, c.base as any);
  const text = uiml.docToUiml(doc);
  return { name: c.name, src: c.src, base: c.base ?? null, doc: JSON.stringify(doc), uiml: text, errors, again: uiml.docToUiml(uiml.parseUiml(text).doc) === text };
});
out.tokens = ['button a key=v k2="q \\" x" "text" “c” #x', '=v a=', 'w==3 «»'].map((line) => ({ line, toks: uiml.tokenize(line).map((t: any) => [t.key ?? null, t.value, t.quoted]) }));

// --- anchors, cleaning, nodes ----------------------------------------------------------------
out.anchors = ['top-left', 'stretch top', 'left-stretch', 'bottom', 'fill', 'centre', '', '  TOP_RIGHT ', 'middle', 'h-stretch', 'vstretch', 'center-left',
  'stretch-stretch', 'right center', 'center center', 'top bottom', 'left right stretch', 'stretch-fill', 'top-stretch-left', 'bottom fill', 'x', 'center-stretch']
  .map((s) => [s, model.anchorName(model.parseAnchor(s))]);
const props: [string, unknown][] = [
  ['anchor', 'Top Right'], ['anchor', ''], ['anchor', null], ['anchor', 5], ['x', '12.345'], ['x', ''], ['x', '  '], ['x', 'abc'], ['x', true], ['x', [7]], ['x', -0.004],
  ['w', -5], ['w', '1e3'], ['w', '0x10'], ['grow', 0], ['grow', '0.001'], ['grow', -1], ['gap', '-3.333'], ['margin', '1,2'], ['margin', [1, 2, 3]], ['margin', '0 0'],
  ['margin', { top: 1, x: 2 }], ['margin', { y: 3 }], ['margin', 'a,b'], ['margin', 4.567], ['margin', true], ['pad', [1, 2, 3, 4, 5]], ['layout', 'ROW'], ['layout', 'flex'],
  ['align', 'left'], ['align', 'fill'], ['align', 'middle'], ['justify', 'space-between'], ['justify', 'bottom'], ['justify', 'stretch'], ['cols', '2.5'], ['cols', 100],
  ['cols', 0], ['kit', ' my kit! '], ['kit', ''], ['style', 'Tést'], ['variant', '  ' + 'v'.repeat(50)], ['variant', ' '], ['color', '#abc'], ['color', 'ABCDEF'],
  ['color', 'red'], ['color', 123456], ['opacity', 1.5], ['opacity', '0.333'], ['value', '0.12345'], ['value', -1], ['visible', 'yes'], ['visible', 'false'],
  ['visible', 1], ['visible', 0], ['visible', null], ['on', true], ['fit', 'Cover'], ['textAlign', 'RIGHT'], ['fontSize', 2], ['fontSize', '33.5'], ['text', null],
  ['text', 42], ['text', 'x'.repeat(2010)], ['placeholder', ''], ['note', '  ' + 'n'.repeat(310)], ['note', ''], ['bogus', 1],
];
out.cleanProp = props.map(([k, v]) => ({ k, v, c: j(model.cleanProp(k, v)) }));
const rawNodes: unknown[] = [
  { type: 'row', id: 'r', children: [{ type: 'btn', name: 'b', size: '100 x 50', label: 'L' }, { type: 'image', size: { width: 30, h: 40 } }, { type: 'picture', width: 7, height: 8, padding: 3, spacing: 2, columns: 4 }] },
  { type: 'grid', cols: 3, cellW: 10, cellH: 12, gapY: 4, gap: 2, align: 'end', layout: 'grid', children: [{}, null, 5, { type: 'screen' }] },
  { type: 'column', layout: null, gap: 3, pad: 2 },
  { type: 'panel', layout: 'row', cols: 2, gapY: 3, w: '1e2', h: null, size: '5x6' },
  { type: 'text', text: 'T', label: 'L', id: '  my id  ' },
  { type: 'icon', id: 7 }, { type: 'icon', id: null, name: 'nm' }, { type: 'slot', size: [10, 20] }, { type: 'slot', size: '10\t20' }, { type: 'slot', size: '10 \t 20' },
  { type: 'slot', size: '-1.5,2' }, { type: 'slot', size: 'a,2' }, { type: 'slot', size: '1..2*3' }, 'string', [1, 2],
];
{
  const book = { used: new Set<string>(), counters: new Map<string, number>() };
  out.nodes = { raws: rawNodes, outs: rawNodes.map((r) => JSON.stringify(model.normalizeNode(r, book))) };
}
out.docs = [{ name: 'X Y', width: '100.4', height: 99999, dpi: '200.04', root: { type: 'button', id: 'zz', layout: 'row', anchor: 'center', x: 5, children: [{ type: 'button' }] } },
  { width: 0, height: -5 }, null, { root: null, dpi: 0 }].map((d) => ({ raw: d, doc: JSON.stringify(model.normalizeDoc(d)) }));

// --- edits ---------------------------------------------------------------------------------------
const EDITS1 = '@update play text="START" kit=btn_blue\n@add menu after=play\n  button shop h=120 "SHOP"\n@remove settings\n@move coin to=bar index=0\n@update nope text="x"\n@screen size=1080x2340';
const EDITS2 = '@add root\n  button play "again"';
const EDITS3 = [
  '@update menu layout=- gap=5 type=row id=menu2',
  '@update title anchor="bottom right" pos=- size=10x20 text="New" hidden',
  '@screen name=Shop 800x600 layout=column pad=4 dpi=320',
  '@move coin to=menu index=-1',
  '@move s1 to=slots before=s5',
  '@move bar to=bar',
  '@replace coins',
  '  row coins2 h=90',
  '    icon gem',
  '@remove close nope root',
  '@add nope',
  '  text x',
  '@frobnicate',
  'not a command',
  '@update',
  '@add menu index=1',
  '@replace bar',
  '  text a1',
  '  text a2',
  '@update close kit=- color=- text=- visible=-',
  '@update play type=nonsense text="zzz"',
  '@update root anchor=center layout=row kit=bg_x text="t" gap=3',
  '@update s2 Size=- W=- cell=- pos=none',
  '@add slots index=-2 at=x',
  '  slot n1',
  '  slot n2',
  '@move play to=menu2 after=shop',
  '@move a to=root before=close',
  '@move menu2 to=play',
  '@replace root',
  '  screen New layout=row',
  '    button only "Only"',
].join('\n');
const EDITS4 = '@screen Main 1080x2340\n@canvas size=10x10 dpi=-3 name=!!!\n@update amount text="2 000" textColor=#fff fontSize=50 textAlign=left\n@insert coins before=coin\n  badge new_badge "!"\n@delete coin';
const EDITS5 = '@set play hidden\n@edit play shown x=5 y=- pos=1,2 cell=3\n@update play rename=play_btn "Go" "Now"\n@update a w=10 h=20\n@update a type=icon\n@update s1 kit=Icon_Coin variant=gold opacity=0.5 color=#123 note="n"';
const editCases = [
  { name: 'test', base: SCREEN, src: EDITS1 }, { name: 'again', base: SCREEN, src: EDITS2 }, { name: 'big', base: SCREEN, src: EDITS3 },
  { name: 'screen', base: SCREEN, src: EDITS4 }, { name: 'misc', base: SCREEN, src: EDITS5 }, { name: 'tricky', base: TRICKY, src: '@update c1 anchor=stretch margin=1\n@move l1 to=r1 index=1\n@remove b1 t' },
];
out.edits = editCases.map((c) => {
  const doc = uiml.parseUiml(c.base).doc;
  const p = uiml.parseEdits(c.src, doc);
  const res = ops.applyOps(doc, p.ops);
  const d = ops.diffDocs(doc, res.doc);
  return { name: c.name, base: c.base, src: c.src, ops: JSON.stringify(p.ops), errors: p.errors, text: uiml.opsToText(p.ops), doc: JSON.stringify(res.doc), applyErrors: res.errors, diff: d, summary: ops.diffSummary(d), uiml: uiml.docToUiml(res.doc) };
});
// Raw stored ops (not from parseEdits): odd values the web tolerates
const rawOps: unknown[] = [
  { op: 'update', id: 'play', set: { w: '200', h: 'x', type: 'icon', anchor: 'Bottom', layout: 'grid', cols: '3' }, unset: ['kit', 'children'] },
  { op: 'add', parent: '', index: 1, nodes: [{ type: 'button', id: 'play' }, { type: 'text', text: 'hi' }] },
  { op: 'add', parent: 'menu', before: 'coins', after: 'nope', nodes: [{ id: 'z' }] },
  { op: 'move', id: 'coins', parent: '', index: 0 },
  { op: 'move', id: 'title', parent: 'menu', index: '1' },
  { op: 'screen', width: '700', height: 70, dpi: null, name: 'A b' },
  { op: 'screen', width: 10, height: 10000, dpi: '300' },
  { op: 'replace', id: 'settings', node: { type: 'icon', id: 'play' } },
  { op: 'remove', ids: ['a', 'b'] },
  { op: 'update', id: 'bar', set: { id: 'title' } },
  { op: 'update', id: 'menu', set: { type: 'screen' } },
  { op: 'whatever' },
  { op: 'update', id: 5 },
  { op: 'add', parent: 'menu' },
  { op: 'remove' },
  { op: 'doc', doc: { name: 'D', root: { children: [{ type: 'row', id: 'r', children: [{ type: 'button' }] }] } } },
  { op: 'update', id: 'r', set: { anchor: '', x: 3 } },
  { op: 'add', parent: 'r', index: -1, nodes: [] },
];
{
  const doc = uiml.parseUiml(SCREEN).doc;
  out.rawOps = rawOps.map((op, i) => {
    const before = ops.applyOps(doc, rawOps.slice(0, i) as any).doc;
    const res = ops.applyOps(before, [op as any]);
    let text: string | null = null; try { text = uiml.opsToText([op as any]); } catch { text = null; }
    return { op, doc: JSON.stringify(res.doc), errors: res.errors, text };
  });
}

// --- the solver ------------------------------------------------------------------------------------
const solveCases = [
  { name: 'screen', src: SCREEN, sizes: [[0, 0], [1920, 1080], [720, 1600]] },
  { name: 'screen2', src: SCREEN2, sizes: [[0, 0], [1400, 2000]] },
  { name: 'tricky', src: TRICKY, sizes: [[0, 0], [1000, 1000]] },
  { name: 'dpi', src: DPI, sizes: [[0, 0]] },
];
out.solve = solveCases.map((c) => {
  const doc = uiml.parseUiml(c.src).doc;
  return { name: c.name, src: c.src, sizes: c.sizes.map(([w, h]) => {
    const rects = solve.solveLayout(doc, w ? { w, h } : undefined);
    return { w, h, rects: [...rects.entries()].map(([id, r]) => [id, r.x, r.y, r.w, r.h]),
      hits: [[100, 100], [540, 700], [300, 1800], [5, 5], [700, 30]].map(([x, y]) => [x, y, solve.hitTest(doc, rects, x, y)]) };
  }) };
});
{
  const doc = uiml.parseUiml(TRICKY).doc;
  const g = model.findNode(doc, 'g2')!;
  out.grid = [[400, 3], [0, 0], [333, 7]].map(([w, n]) => ({ w, n, g: solve.gridGeometry(g, { x: 0, y: 0, w, h: 100 }, n) }));
}

// --- hand edits -----------------------------------------------------------------------------------
{
  const doc = uiml.parseUiml(SCREEN2).doc;
  const rects = solve.solveLayout(doc, { w: 1000, h: 2000 });
  const res: unknown[] = [];
  const anchors = [undefined, { h: 'left', v: 'top' }, { h: 'center', v: 'middle' }, { h: 'stretch', v: 'bottom' }, { h: 'stretch', v: 'stretch' }, { h: 'right', v: 'stretch' }];
  for (const id of ['corner', 'bar', 'box', 'a', 'c']) {
    const n = model.findNode(doc, id)!, p = rects.get(model.findParent(doc, id)!.id)!, r = rects.get(id)!;
    for (const a of anchors) for (const size of [false, true]) for (const [dx, dy, dw, dh] of [[0, 0, 0, 0], [-40.4, 10.6, 0, 0], [5, -3, -20, 7.5]]) {
      const rr = { x: r.x + dx, y: r.y + dy, w: r.w + dw, h: r.h + dh };
      res.push({ id, a: a ?? null, size, d: [dx, dy, dw, dh], patch: edit.placeAt(n, p, rr, { anchor: a as any, size }) });
    }
  }
  out.placeAt = res;
  const row = model.findNode(doc, 'r')!;
  out.slots = [[10, 10], [300, 1800], [2000, 1830], [0, 0]].flatMap(([x, y]) => ['a', 'b', 'c', 'zz'].map((d) => ({ d, x, y, s: edit.layoutSlot(row, rects, d, { x, y }) ?? null })));
  const tdoc = uiml.parseUiml(TRICKY).doc;
  const trects = solve.solveLayout(tdoc);
  out.gridSlots = [[0, 0], [150, 300], [500, 600], [700, 1200]].map(([x, y]) => ({ x, y, s: edit.layoutSlot(model.findNode(tdoc, 'g2')!, trects, 'q1', { x, y }) ?? null }));
  out.sizeInLayout = [['c', 'r', 200, 120], ['a', 'r', 100.4, 60], ['corner', 'box', 50, 50]].map(([id, pid, w, h]) => {
    const from = rects.get(id as string)!;
    return { id, pid, w, h, p: edit.sizeInLayout(model.findNode(doc, id as string)!, model.findNode(doc, pid as string)!, from, { ...from, w: w as number, h: h as number }) };
  });
  out.anchorAt = [[10, 390], [210, 180], [380, 5], [100, 100], [300, 300]].map(([x, y]) => [x, y, edit.anchorAt({ x: 0, y: 0, w: 400, h: 400 }, { x, y }), edit.anchorAt({ x: 0, y: 0, w: 0, h: 0 }, { x, y })]);
  out.anchorBox = ['corner', 'bar', 'box', 'r'].map((id) => [id, edit.anchorBox(model.findNode(doc, id)!, rects.get(model.findParent(doc, id)!.id)!)]);
  out.snap = ['corner', 'bar', 'a', 'box'].map((id) => {
    const t = edit.snapTargets(doc, rects, id);
    const r = rects.get(id)!;
    return { id, t, moves: [[3, 0], [-4, 5], [7.5, -6.2], [100, 100]].map(([dx, dy]) => edit.snapMove({ ...r, x: r.x + dx, y: r.y + dy }, t, 6)),
      resizes: [{ e: true }, { w: true, n: true }, { s: true }].map((e) => edit.snapResize({ ...r, w: r.w + 4, h: r.h - 3 }, e, t, 6)) };
  });
  out.nudge = [['corner', -1, 0], ['a', 0, 2], ['bar', 10, 10], ['box', 0, -1], ['c', -5, 0], ['root', 1, 1]].map(([id, dx, dy]) => ({ id, dx, dy, op: edit.nudgeOp(doc, rects, id as string, dx as number, dy as number) ?? null }));
  const n2 = uiml.parseUiml(SCREEN2 + '  text hid anchor=center hidden "h"\n').doc;
  const r2 = solve.solveLayout(n2, { w: 1000, h: 2000 });
  out.reparent = [['corner', 'root', '', ''], ['corner', 'r', 'b', ''], ['bar', 'root', '', 'r'], ['a', 'box', '', ''], ['corner', 'box', '', ''], ['hid', 'box', '', ''], ['nope', 'box', '', ''], ['box', 'r', '', '']]
    .map(([id, parent, before, after]) => ({ id, parent, before, after, ops: edit.reparentOps(n2, r2, id, { parent, ...(before ? { before } : {}), ...(after ? { after } : {}) }) }));
  out.noop = [{ op: 'move', id: 'b', parent: 'r', before: 'c' }, { op: 'move', id: 'b', parent: 'r', after: 'a' }, { op: 'move', id: 'b', parent: 'r', before: 'a' }, { op: 'move', id: 'a', parent: 'r', after: 'zz' }, { op: 'move', id: 'b', parent: 'box', before: 'c' }]
    .map((op) => [op, edit.moveIsNoop(doc, op as any)]);
}

// --- steps ------------------------------------------------------------------------------------------
{
  const t0 = Date.parse('2026-11-10T10:00:00Z');
  const agentStep = { id: 'ag', by: 'agent', prompt: 'make it', reply: 'done', ops: [{ op: 'add', parent: 'root', nodes: [{ id: 'x', type: 'button' }] }], at: new Date(t0).toISOString() };
  const upd = (set: Record<string, unknown>, id = 'x', unset?: string[]) => [{ op: 'update', id, set, ...(unset ? { unset } : {}) }];
  type Act = [string, unknown[], string, number, boolean?];
  const acts: Act[] = [
    ['push', upd({ w: 10 }), 'x: size', 1000], ['push', upd({ text: 'A' }), 'x: text', 10_000], ['push', upd({ text: 'AB', h: 3 }, 'x', ['kit']), 'x: text', 11_000],
    ['push', upd({ kit: 'k' }, 'x', ['h']), 'x: text', 12_000], ['push', [{ op: 'add', parent: 'root', nodes: [{ id: 'y', type: 'icon' }] }], '+ y', 20_000],
    ['push', upd({ x: 5 }, 'y'), 'y: move', 21_000, false], ['push', upd({ x: 9 }, 'y'), 'y: move', 21_500, false], ['push', upd({ id: 'yy' }, 'y'), 'y: rename', 22_000],
    ['push', upd({ id: 'z' }, 'yy'), 'y: rename', 22_500], ['undo'] as any, ['undo'] as any, ['redo'] as any, ['push', upd({ h: 5 }), 'x: size', 20_000 + 600_001],
    ['undo'] as any, ['undo'] as any, ['undo'] as any, ['undo'] as any, ['redo'] as any, ['redo'] as any, ['redo'] as any, ['redo'] as any, ['redo'] as any, ['push', [], 'none', 700_000],
  ];
  const ids = new Map<string, string>();
  const norm = (v: unknown): unknown => JSON.parse(JSON.stringify(v), (k, x) => {
    if ((k === 'id' || k === 'partOf') && typeof x === 'string' && /^s[0-9a-z]{6,}$/.test(x)) { if (!ids.has(x)) ids.set(x, `S${ids.size + 1}`); return ids.get(x); }
    return x;
  });
  let steps: any[] = [agentStep];
  let redo: any[] = [];
  const trace: unknown[] = [];
  for (const a of acts) {
    if (a[0] === 'push') steps = ops.pushUserAction(steps, a[1] as any, a[2], t0 + a[3], a[4] ?? true);
    else if (a[0] === 'undo') { const r = ops.undoAction(steps, redo); if (r) { steps = r.steps; redo = r.redo; } }
    else { const r = ops.redoAction(steps, redo); if (r) { steps = r.steps; redo = r.redo; } }
    trace.push({ steps: norm(steps), redo: norm(redo), parts: steps.map((s) => ops.stepParts(s)), text: steps.map((s) => (s.by === 'user' ? ops.userStepText(s) : '')),
      doc: JSON.stringify(ops.docAt({ steps }).doc) });
  }
  out.steps = { t0, agent: agentStep, acts: acts.map((a) => (a[0] === 'push' ? { kind: 'push', ops: a[1], label: a[2], dt: a[3], fold: a[4] ?? true } : { kind: a[0] })), trace };
  const old = { id: 'o', by: 'user', prompt: 'edit', ops: upd({ w: 1 }), at: agentStep.at };
  const bad = { id: 'p', by: 'user', prompt: 'bad', ops: upd({ w: 1 }).concat(upd({ h: 2 })), at: agentStep.at, parts: [{ label: 'a', n: 1.5, at: '' }, { label: 'b', n: 0.5 }] };
  out.stepParts = [old, bad, agentStep, { ...bad, parts: [{ label: '', n: 1, at: 'x' }, { label: 'c', n: 1 }] }].map((s) => ({ s, parts: ops.stepParts(s as any), text: ops.userStepText(s as any) }));
  out.docAt = { config: { screenName: 'My Screen!', steps: [agentStep, { junk: 1 }, null, old, { id: 'q', by: 'user', ops: [{ op: 'update', id: 'nope' }] }] }, counts: [null, 0, 1, 2, -1, 10].map((c) => {
    const r = ops.docAt({ screenName: 'My Screen!', steps: [agentStep, { junk: 1 }, null, old, { id: 'q', by: 'user', ops: [{ op: 'update', id: 'nope' }] }] }, c ?? undefined);
    return { c, doc: JSON.stringify(r.doc), errors: r.errors };
  }) };
}

// --- kits and styles ------------------------------------------------------------------------------
const KIT_JSON = JSON.stringify({ version: 1, name: 'My Kit!'.repeat(20), elements: [
  { name: 'Btn Red!', type: 'button', assetType: 'sliced', width: 120.6, height: 0, png: 'iVBORw0KGgo=', nineSlice: { left: 10.4, top: '12', right: -3, bottom: null }, variant: 'Primary' + 'x'.repeat(50), description: 'Main action', port: 'p1', dpi: '960', dpiFrom: 'input', source: { w: 10, h: 10, trim: { x: 0, y: 0, w: 10, h: 10 } } },
  { name: 'btn red', png: 'AAAA', type: 'nope', assetType: 'tiled', nineSlice: { left: 1 } },
  { name: '', png: 'AAAA', nineSlice: { left: 5 } },
  { name: 'icon_coin', png: 'AAAA', type: 'icon', width: 64, height: 64, dpi: 0, dpiFrom: 'other', description: '' },
  { name: 'x', png: '' }, null, { png: 5 },
  { name: 'panel_wood', png: 'AAAA', type: 'panel', width: 300, height: 200, nineSlice: { left: 40, top: 40, right: 40, bottom: 40 }, dpi: 240, variant: 'secondary' },
  { name: 'progress_track', png: 'AAAA', width: 400, height: 40, nineSlice: { left: 12, top: 0, right: 12, bottom: 0 } },
  { name: 'progress_fill', png: 'AAAA', width: 400, height: 40 },
  { name: 'tab_on', png: 'AAAA', type: 'tab', width: 200, height: 80, variant: 'selected' },
  { name: 'bg_forest', png: 'AAAA', width: 1080, height: 1920 },
  { name: 'icon_gem', png: 'AAAA', type: 'icon', width: 60, height: 60, description: 'premium currency' },
  { name: 'slot_frame', png: 'AAAA', width: 160, height: 160, nineSlice: { left: 20, top: 20, right: 20, bottom: 20 }, dpi: 960 },
  { name: 'фон_кнопка', png: 'AAAA' },
], textStyles: [
  { name: 'title', role: 'title', font: 'Russo One', size: 96, color: '#FFE066', gradient: '#FF8C00', stroke: { color: '#5A2A00', width: 6 } },
  { name: 'Title', role: 'heading', font: 'lobster', weight: 500, shadow: { opacity: 0.5 } },
  { name: 'body text', font: 'Fira Sans', weight: 800, uppercase: 1, description: ' body ' },
  { role: 'number', color: '#0f0', letterSpacing: 99, lineHeight: 0.1, align: 'right' },
  'junk',
] });
{
  const kit = kitm.parseKit(KIT_JSON);
  out.kit = { json: KIT_JSON, parsed: kit.elements.map((e: any) => [e.name, e.type, e.assetType, e.width, e.height, e.variant ?? U, e.description ?? U, e.nineSlice ?? U, e.dpi ?? U, e.dpiFrom ?? U, e.port ?? U, e.source ?? U, e.png]),
    styles: JSON.stringify(kit.textStyles), name: kit.name,
    summary: [kitm.kitSummary(kit), kitm.kitSummary(kit, 480), kitm.kitSummary(kit, 960), kitm.kitSummary(kit, 326), kitm.kitSummary({ version: 1, name: 'k', elements: [], textStyles: [] })],
    bad: ['', 'nope', '5', 'null', '[1]', '{"name": 12345}', '{"name": null, "elements": {}}'].map((s) => JSON.stringify(kitm.parseKit(s))) };
  out.names = ['map_icon', 'coins_icon', 'currency_icon', 'settings_icon', 'icon', 'premium_gem', 'gems', 'Coin', 'big_red_button', 'кнопка_фон', 'frame_slot', 'x', 'progress_bar_hp', 'tab_selected_1', 'icon_coin', '123_coin', 'forest']
    .map((id) => [id, kit.elements.map((e: any) => kitm.nameScore(id, e))]);
  const queries: [string[], string | undefined, string | undefined][] = [[['icon'], undefined, 'map_icon'], [['icon'], undefined, 'coins_icon'], [['icon'], undefined, 'currency_icon'], [['icon'], undefined, 'zzz'],
    [['tab', 'button'], undefined, 'shop_tab'], [['button'], 'primary', undefined], [['button'], 'red', undefined], [['button'], 'nope', 'btn'], [['panel', 'frame'], 'SECONDARY', undefined],
    [['slot', 'frame', 'panel'], undefined, 's1'], [['progressFill'], undefined, 'hp'], [['header'], undefined, undefined], [['other'], undefined, undefined], [['background'], undefined, 'bg']];
  out.ofType = queries.map(([types, v, h]) => [types, v ?? null, h ?? null, kitm.kitElementOfType(kit, types as any, v, h)?.name ?? null]);
  out.guess = ['bg_main', 'Background', 'фон', 'big_btn', 'КНОПКА', 'hp_fill', 'progress', 'xp_bar', 'sidebar', 'panel', 'Окно', 'border', 'Рамка', 'coin', 'Иконка', 'toggle', 'knob', 'slider', 'text_field', 'tab', 'badge', 'line', 'banner', 'tile', 'zzz', 'header_tab']
    .map((n) => [n, kitm.guessElementType(n)]);
  out.kitNames = ['  my  kit  ', '__a-b__', '!!!', 'Кит', 5, null, '-x-', 'a b\tc'].map((v) => [v, kitm.cleanKitName(v, 'fb')]);
  out.nine = [null, 5, { left: 1.5, top: -2, right: 'x', bottom: '3' }, { left: 0 }, [], { left: 0.4 }].map((v) => [v, kitm.normalizeNine(v) ?? U]);
  const kdoc = uiml.parseUiml('screen S 1080x1920 kit=bg_forest\n  icon settings_icon size=80x80\n  icon map_icon size=80x80\n  text heart "♥"\n  badge b1 "!"\n  button play "PLAY"\n  tab shop_tab "Shop"\n  progress hp value=0.3 "30"\n  slider vol\n  slot s1\n  panel p1 variant=secondary\n  image coin_pic\n  text star " ★★ "\n  text emoji "😀"\n  text num "12"\n  button kitless kit=nope\n  input name_field placeholder="Name"').doc;
  out.report = [kit, null, { version: 1, name: 'k', elements: [], textStyles: [] }].map((k: any) => ({ report: render.kitReport(kdoc, k), coverage: render.kitCoverage(kdoc, k) }));
  out.reportDoc = uiml.docToUiml(kdoc);
  out.skins = [kit, null].map((k: any) => { const r: unknown[] = []; model.walk(kdoc.root, (n: any) => { const s = render.skinOf(n, k); r.push([n.id, s.el?.name ?? null, s.fill?.name ?? null, s.handle?.name ?? null, s.explicit, s.missing, render.roleOf(n), JSON.stringify(render.styleOf(n, k)), JSON.stringify(render.styleOf(n, k, n))]); }); return r; });
}
const STYLE_RAWS: unknown[] = [
  { name: 'big title', font: 'nope', weight: 650, size: 999, color: 'fff', stroke: { width: 3 } },
  { name: '', font: ' russo one ', weight: 100, size: 5.5, color: '#abc', gradient: '#123456', stroke: { width: 0 }, shadow: { color: 'red', x: 100, y: -100, blur: 99, opacity: 2 }, letterSpacing: '3', lineHeight: '2', uppercase: 'yes', align: 'left', description: 'd'.repeat(400) },
  { shadow: { opacity: 0 }, stroke: [], gradient: '', role: 'number' }, { shadow: [], stroke: { width: '7', color: '#0F0' }, size: null, weight: null, lineHeight: null }, null, [1], 'x',
  { name: 'Ä b-c_d', font: 'Press Start 2P', weight: 900, role: 'nope', align: 'justify', gradient: 'nothex' },
];
out.styles = STYLE_RAWS.map((r) => ({ raw: r ?? null, style: JSON.stringify(ts.normalizeTextStyle(r, 'fallback')) }));
out.styleConfig = JSON.stringify(ts.textStyleFromConfig({ name: '  hello world ', font: 'Bangers', size: 50, uiThing: 1 }));
const WIDTHS = ['Hello', 'PLAY', 'iiii llll', 'mmmm WWWW', 'Привет МИР', 'щшжю ЩШЖЮ Ё ё', '1 250', '', 'ß straße', '😀 emoji', 'A.B,C:D;E\'F!G|', 'Ünïcödé'];
const WSTYLES = [ts.TEXT_STYLE_DEFAULT, { ...ts.TEXT_STYLE_DEFAULT, font: 'Russo One', weight: 400, size: 96, uppercase: true, letterSpacing: 2.5 }, { ...ts.TEXT_STYLE_DEFAULT, font: 'nope', weight: 500, size: 33.3, letterSpacing: -1 }, { ...ts.TEXT_STYLE_DEFAULT, font: 'Amatic SC', weight: 700, size: 10 }];
out.widths = WSTYLES.map((s) => ({ style: s, w: WIDTHS.map((t) => ts.estimateTextWidth(t, s)) }));
out.widthTexts = WIDTHS;
{
  const cur = ts.normalizeTextStyle(STYLE_RAWS[1], 'cur');
  out.stylePrompt = { cur: JSON.stringify(cur), prompts: [ts.buildStylePrompt(cur, '  make it bold — please  ', 0), ts.buildStylePrompt(ts.TEXT_STYLE_DEFAULT, 'x', 2)] };
  const answers = ['Here:\n```json\n{"role":"title","font":"Russo One","weight":400,"size":110,"color":"#FFE45C","gradient":"#FF8A00","stroke":{"color":"#3A1800","width":8},"uppercase":true,"description":"main title (1 ref)"}\n```',
    'no json', '{bad json}', '{"stroke": null, "shadow": {"x": 3}, "name": "renamed", "description": ""}', '```JSON\n{"size": "12"}```', '[1,2]', '{"a":1} and {"b":2}'];
  out.styleAnswers = answers.map((a) => ({ a, s: JSON.stringify(ts.parseStyleAnswer(a, cur)) }));
  out.fontFiles = ts.fontFilesOf([cur, ts.TEXT_STYLE_DEFAULT, { ...ts.TEXT_STYLE_DEFAULT, weight: 700 }, cur]);
}
{
  const fit: unknown[] = [];
  const styles = [ts.TEXT_STYLE_DEFAULT, ts.normalizeTextStyle({ font: 'Russo One', size: 96, uppercase: true, letterSpacing: 2, lineHeight: 1 }), ts.normalizeTextStyle({ font: 'Lobster', size: 40, lineHeight: 1.5 })];
  for (const s of styles) for (const [t, w, h, size, pad] of [['FOREST QUEST', 960, 140, undefined, 0], ['Hello world this is a long label', 300, 200, undefined, 10], ['Line1\nLine two is longer', 200, 40, 50, 0], ['', 100, 10, undefined, 0], ['A', 1, 1, undefined, 30], ['Привет мир как дела', 150, 300, undefined, 5]] as const)
    fit.push({ style: JSON.stringify(s), t, w, h, size: size ?? null, pad, r: render.layoutText({ text: t, rect: { x: 0, y: 0, w, h }, style: s, align: 'center', size, padX: pad }) });
  out.layoutText = fit;
}

// --- device ------------------------------------------------------------------------------------------
out.device = { cleanDpi: [0, -1, 'abc', '480', 35, 96001, 100.04, 100.05, 460.25, null, true].map((v) => [v, device.cleanDpi(v) ?? U]),
  names: [480, 160, 326, 460, 100].map((d) => [d, device.densityName(d)]),
  frames: device.UI_DEVICES.map((s: any) => [s.id, device.previewFrame({ width: 1080, height: 1920, dpi: 480 }, s, 'dpi'), device.previewFrame({ width: 1080, height: 1920 }, s, 'fit'), device.previewFrame({ width: 750, height: 1334, dpi: 326 }, s, 'dpi')]),
  scale: [[{}, {}], [{ dpi: 480 }, { dpi: 960 }], [{ dpi: 326 }, { dpi: 0 }], [{ dpi: 'x' }, { dpi: 1000 }]].map(([d, e]) => [d, e, device.kitScale(d as any, e as any), device.designSize(d as any, { width: 401, height: 121, ...(e as any) })]) };

// --- agent ------------------------------------------------------------------------------------------------
{
  const kit = kitm.parseKit(KIT_JSON);
  const doc = uiml.parseUiml(SCREEN).doc;
  const steps = [
    { id: 's1', by: 'agent', prompt: 'menu', reply: 'ok', at: '', ops: [{ op: 'doc', doc }] },
    { id: 's2', by: 'user', at: '', ops: [{ op: 'update', id: 'play', set: { text: 'GO' } }] },
    { id: 's3', by: 'user', at: '', prompt: '', ops: [{ op: 'remove', ids: ['close'] }, { op: 'update', id: 'a', set: { w: 5 } }] },
    { id: 's4', by: 'user', at: '', ops: [], parts: [{ label: 'x: size', n: 0, at: '' }] },
    { id: 's5', by: 'user', at: '', prompt: 'p'.repeat(250), ops: [{ op: 'add', parent: 'root', nodes: [] }] },
    { id: 's6', by: 'agent', at: '', ops: [] },
  ];
  const many = Array.from({ length: 12 }, (_, i) => ({ id: `m${i}`, by: i % 2 ? 'agent' : 'user', prompt: `p${i}`, reply: `r${i}`, at: '', ops: [{ op: 'update', id: 'x', set: { w: i } }] }));
  const contexts = [
    { doc, kit, steps, prompt: '  добавь кнопку магазина  ', references: [{ name: 'shot', kind: 'image' }, { name: 'brief', kind: 'text', text: 'Make it green.\n' + 'z'.repeat(6100) }, { name: 'ref2', kind: 'image' }], selected: 'play', withScreenshot: true, withKitSheet: true },
    { doc: uiml.parseUiml('').doc, kit: null, steps: [], prompt: 'build', references: [], withScreenshot: true },
    { doc: uiml.parseUiml(DPI).doc, kit, steps: many, prompt: 'x', references: [{ name: 'r', kind: 'image' }], withKitSheet: true },
    { doc: uiml.parseUiml(TRICKY).doc, kit: { version: 1, name: 'k', elements: [], textStyles: [] }, steps: [], prompt: '', references: [{ name: 'q', kind: 'text' }] },
  ];
  out.agent = { kit: KIT_JSON, contexts: contexts.map((c: any) => ({ ...c, kit: c.kit === null ? null : c.kit === kit ? 'KIT' : 'EMPTY', doc: c.doc === doc ? 'SCREEN' : uiml.docToUiml(c.doc), prompt_text: agent.buildAgentPrompt(c), steps: c.steps })) };
  const rep = render.kitReport(doc, kit);
  out.review = { prompt: agent.buildReviewPrompt({ doc, kit, request: ' меню ', missing: rep.missing, unused: rep.unused, glyphs: ['heart', 'star'] }),
    empty: agent.buildReviewPrompt({ doc: uiml.parseUiml(DPI).doc, kit: { version: 1, name: 'k', elements: [], textStyles: [] }, request: 'r', missing: [], unused: [], glyphs: [] }) };
  const answers = [
    'Переименовал кнопку.\n```uiml-edit\n@update play "GO"\n```',
    'Here it is:\n```uiml\nscreen Shop 1080x1920\n  panel p anchor=center size=800x900\n```',
    'Done\n@remove close',
    'It has 3 buttons.',
    'A\n\n\n\n```\n@update play text=X\n```\n\n\n\nB ```uiml\n  button b2 "x"\n``` tail',
    '```text\nscreen T 100x100\n  button z\n```',
    'Before\nscreen Big 500x500\n  icon q',
    'two blocks\n```json\n{"a":1}\n```\n```uiml-edit\n@move coin to=bar\n@update nope x=1\n```',
    '```uiml-edit no newline',
    'x ````uiml-edit\n@remove a\n',
    '```uiml\n',
    '  \n```foo\n  @remove bar\n```',
    'Note: @mention\n  @update play kit=btn_red\n',
    'screen\n  x',
    'a\r@remove close',
    '```UIML-Edit extra words\n@remove b\n```',
  ];
  out.answers = answers.map((a) => { const r = agent.parseAgentAnswer(a, doc); return { a, reply: r.reply, ops: JSON.stringify(r.ops), errors: r.errors }; });
}

// --- structure ----------------------------------------------------------------------------------------------
{
  const pngB64 = (await sharp({ create: { width: 40, height: 20, channels: 4, background: '#ff0000' } }).png().toBuffer()).toString('base64');
  const kitA = { version: 1, name: 'k', textStyles: [], elements: [
    { name: 'btn_red', type: 'button', assetType: 'sliced', width: 60, height: 40, nineSlice: { left: 10, top: 10, right: 10, bottom: 10 }, png: pngB64 },
    { name: 'icon_coin', type: 'icon', assetType: 'simple', width: 64, height: 64, png: pngB64 }] };
  const kit = kitm.parseKit(KIT_JSON);
  for (const e of kit.elements) e.png = pngB64;
  const STRUCT = [
    'screen Struct 1080x1920 kit=bg_forest layout=column pad=10 gap=4',
    '  text title h=150 style=title "FOREST QUEST quest quest"',
    '  button play h=140 variant=red "PLAY" color=#FF0000',
    '  tab shop_tab h=80 "Shop" textColor=#00FF00 textAlign=left',
    '  badge b1 w=50 h=50 "9"',
    '  progress hp h=40 value=0.25 "25%"',
    '  progress xp h=40',
    '  input name_field h=60 placeholder="Your name" fontSize=20',
    '  input empty_field h=60',
    '  grid slots h=400 cols=3 gap=8',
    '    slot s1',
    '    slot s2',
    '    slot s3 hidden',
    '  grid slots2 h=400 cols=3 gap=8 pad=4,8',
    '    slot t1 w=10',
    '  row r h=100 align=center justify=between pad=6',
    '    icon icon_gem size=60x60',
    '    text count w=200 h=60 "1 250"',
    '  row r2 h=100',
    '    button g1 grow=1 "G"',
    '    text tt w=100 "T"',
    '  column c3 h=300 align=stretch',
    '    text c3a h=40 "Long label that will surely need to wrap across several lines here"',
    '    panel c3b w=300 h=100 opacity=0.5',
    '      text inner "x"',
    '  text emptytext h=20',
    '    icon child_of_text',
    '  divider d1 h=4 hidden',
    '  image coin_pic h=100 opacity=1',
  ].join('\n');
  const cases: [string, string, any][] = [['screen_kitA', SCREEN, kitA], ['screen_nokit', SCREEN, null], ['dpi_kit', 'screen M 600x400 dpi=480\n  button b anchor=center size=300x100 kit=btn "Go"', { version: 1, name: 'k', textStyles: [], elements: [{ name: 'btn', type: 'button', assetType: 'sliced', width: 200, height: 120, nineSlice: { left: 40, top: 40, right: 40, bottom: 40 }, dpi: 960, png: pngB64 }] }],
    ['struct_kit', STRUCT, kit], ['struct_nokit', STRUCT, null], ['tricky_kit', TRICKY, kit], ['nobg_layout', STRUCT.replace(' kit=bg_forest', ''), kit]];
  out.structure = { png: pngB64, kitA: JSON.stringify(kitA), cases: cases.map(([name, src, k]) => {
    const doc = uiml.parseUiml(src).doc;
    const r = structure.toUiStructure(doc, k);
    return { name, src, kit: k === null ? null : k === kitA ? 'A' : k === kit ? 'KIT' : JSON.stringify(k), json: JSON.stringify(JSON.parse(r.json)), pretty: name === 'dpi_kit' || name === 'struct_kit' ? r.json : null, sprites: r.sprites.map((s: any) => [s.file, s.width, s.height, s.nineSlice ?? U, s.density ?? U, s.dpi ?? U, s.png.length]), fonts: r.fonts, warnings: r.warnings };
  }) };
}

// --- kit element preparation ----------------------------------------------------------------------------
{
  const svgPng = (svg: string) => sharp(Buffer.from(svg)).png().toBuffer();
  const green = await svgPng('<svg xmlns="http://www.w3.org/2000/svg" width="360" height="200"><rect width="360" height="200" fill="#00C040"/>\n    <rect x="40" y="40" width="280" height="120" rx="24" fill="#D83A3A" stroke="#401010" stroke-width="6"/></svg>');
  const striped = await svgPng('<svg xmlns="http://www.w3.org/2000/svg" width="300" height="120"><defs><pattern id="g" width="8" height="8" patternUnits="userSpaceOnUse"><rect width="4" height="8" fill="#2050E0"/><rect x="4" width="4" height="8" fill="#E05020"/></pattern></defs>\n      <rect x="4" y="4" width="292" height="112" rx="20" fill="url(#g)" stroke="#101010" stroke-width="6"/></svg>');
  // A noisy panel on transparency: flat middle with a pixel of noise, an off-centre flat run
  const W = 90, H = 60;
  const raw = Buffer.alloc(W * H * 4);
  for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) {
    const o = (y * W + x) * 4;
    const inside = x >= 3 && x < W - 3 && y >= 2 && y < H - 2;
    const edge = inside && (x < 9 || x >= W - 9 || y < 8 || y >= H - 8);
    const c = !inside ? [0, 0, 0, 0] : edge ? [90 + ((x * 7 + y * 3) % 40), 60, 30, 255] : [200, 180, 120, 255];
    if (x === 45 && y === 30) c.splice(0, 4, 205, 182, 118, 255);
    if (x >= 20 && x < 24 && inside && !edge) c.splice(0, 4, 150, 100, 50, 255);
    raw.set(c, o);
  }
  const noisy = await sharp(raw, { raw: { width: W, height: H, channels: 4 } }).png().toBuffer();
  const tiny = await sharp({ create: { width: 3, height: 1, channels: 4, background: '#00000000' } }).png().toBuffer();
  const pics = { green, striped, noisy, tiny };
  const runs: [keyof typeof pics, Record<string, unknown>][] = [
    ['green', { assetType: 'sliced', key: true, trim: true, optimize: true, nineSlice: null }],
    ['green', { assetType: 'sliced', key: true, trim: true, optimize: false, nineSlice: { left: 80, top: 70, right: 80, bottom: 70 } }],
    ['green', { assetType: 'simple', key: true, trim: true, optimize: true, nineSlice: null }],
    ['green', { assetType: 'sliced', key: false, trim: false, optimize: true, nineSlice: null }],
    ['green', { assetType: 'sliced', key: true, trim: true, optimize: true, nineSlice: { left: 300, top: 5, right: 300, bottom: 190 } }],
    ['striped', { assetType: 'sliced', key: false, trim: true, optimize: true, nineSlice: null }],
    ['striped', { assetType: 'tiled', key: true, trim: true, optimize: true, nineSlice: null }],
    ['noisy', { assetType: 'sliced', key: false, trim: true, optimize: true, nineSlice: null }],
    ['noisy', { assetType: 'sliced', key: true, trim: false, optimize: true, nineSlice: null }],
    ['tiny', { assetType: 'sliced', key: true, trim: true, optimize: true, nineSlice: null }],
  ];
  const rawSha = async (png: Buffer) => crypto.createHash('sha1').update((await sharp(png).ensureAlpha().raw().toBuffer())).digest('hex');
  const results: unknown[] = [];
  for (const [pic, p] of runs) {
    const r = await prep.prepareElement({ data: pics[pic], ...(p as any) });
    results.push({ pic, p, width: r.width, height: r.height, nineSlice: r.nineSlice ?? U, source: r.source, detected: r.detected ?? U, sha: await rawSha(r.png) });
  }
  const detect: unknown[] = [];
  for (const pic of Object.keys(pics) as (keyof typeof pics)[]) for (const key of [false, true]) detect.push({ pic, key, r: await prep.detectInputNine(pics[pic], key) });
  out.prep = { pics: Object.fromEntries(Object.entries(pics).map(([k, v]) => [k, v.toString('base64')])), hashes: Object.fromEntries(Object.entries(pics).map(([k, v]) => [k, prep.inputHash(v)])),
    inputShas: Object.fromEntries(await Promise.all(Object.entries(pics).map(async ([k, v]) => [k, await rawSha(v)]))), results, detect };
}

out.misc = { stepId: /^s[0-9a-z]+$/.test(ops.newStepId()), uiOnly: ops.UI_LAYOUT_UI_ONLY, presets: model.ANCHOR_PRESETS, types: model.UI_NODE_TYPES, keys: model.NODE_KEYS,
  base36: [0, 35, 36, 1762768800000].map((n) => [n, n.toString(36)]), iso: [0, 1762768800123, -1, 951782400000, 4102444800000].map((n) => [n, new Date(n).toISOString()]),
  parse: ['2026-11-10T10:00:00Z', '2026-11-10T10:00:00.5Z', '2026-11-10T10:00:00.123+03:00', '2026-11-10', '', 'nope', '2026-11-10T10:00Z', '2026-02-30T00:00:00Z'].map((s) => [s, Number.isNaN(Date.parse(s)) ? null : Date.parse(s)]) };

fs.writeFileSync(process.argv[2], JSON.stringify(out));
console.log('ok', Object.keys(out).length, JSON.stringify(out).length);
