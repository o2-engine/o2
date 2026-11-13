// Golden references, round 2 (AssetsLine after 77d5b09): text styles with a gradient fill and layer effects, the AI text
// style's prompt and answer, the target device, the kit's new element fields and purposes, points named in a request,
// the screen op with a scale, the layout node's own kit and graphic outputs, the agent's kit additions and turns.
//   AL=<AssetsLine checkout>; ASSETSLINE=$AL $AL/backend/node_modules/.bin/tsx cases2.mts /tmp/golden2.json
//   python3 gen_tests.py /tmp/golden2.json ../UiDocGolden2.inc kUiDocGolden2Parts
import fs from 'node:fs';
const A = process.env.ASSETSLINE ?? '/Users/andreizenkovich/work/zenkovich.space/AssetsLine';
const ts = await import(`${A}/shared/textStyle.ts`);
const device = await import(`${A}/shared/uiDoc/device.ts`);
const kitm = await import(`${A}/shared/uiKit.ts`);
const ops = await import(`${A}/shared/uiDoc/ops.ts`);
const own = await import(`${A}/shared/uiDoc/own.ts`);
const uiml = await import(`${A}/shared/uiDoc/uiml.ts`);
const agent = await import(`${A}/shared/uiDoc/agent.ts`);
const ports = await import(`${A}/shared/nodePorts.ts`);
const uploads = await import(`${A}/shared/uploads.ts`);
const sample = await import(`${A}/shared/uiDoc/sample.ts`);
const png = await import(`${A}/shared/pngMeta.ts`);

const out: Record<string, unknown> = {};
const U = '__undef__';
const j = (v: unknown) => (v === undefined ? U : JSON.parse(JSON.stringify(v)));

// --- text styles: fill and effects ------------------------------------------------------------------------
const STYLE_RAWS: unknown[] = [
  { name: 'gold title', font: 'Lilita One', size: 96, color: '#ffd84a',
    fill: { angle: 90, stops: [{ color: '#fff3a0', at: 0 }, { color: '#ffd84a', at: 0.48 }, { color: '#f0a020', at: 0.52 }, { color: '#c06000', at: 1 }] },
    effects: [{ type: 'bevel', depth: 3, soft: 1 }, { type: 'stroke', color: '#5a2a00', width: 7 }, { type: 'stroke', color: '#fff', width: 3.3 },
      { type: 'extrude', color: '#3a1800', y: 9 }, { type: 'dropShadow', y: 6, blur: 10, opacity: 0.5 }, { type: 'glow', color: '#ff0', size: 12, off: true }] },
  { name: 'aliases', effects: [{ kind: 'Outline', size: 2.26 }, { type: 'inner-shadow', y: 3 }, { type: 'emboss' }, { type: 'Outer Glow', blur: 7 },
    { type: '3D', depth: 5 }, { type: 'constructor' }, { type: 'nope' }, null, 5, [], { type: 'shadow', opacity: 0 }, { type: 'extrude', x: 0, y: 0 },
    { type: 'stroke', width: 0 }, { type: 'glow', size: 0 }, { type: 'bevel', depth: 0 }, { type: 'stroke', enabled: false, width: 4 }] },
  { name: 'many', effects: Array.from({ length: 12 }, (_, i) => ({ type: 'stroke', width: i + 1 })) },
  { name: 'clamps', effects: [{ type: 'stroke', width: 999, opacity: 7 }, { type: 'extrude', x: -200, y: '12.26' }, { type: 'shadow', x: 500, y: -500, blur: 900, spread: 99 },
    { type: 'innerShadow', x: 99, y: -99, size: 80 }, { type: 'bevel', depth: 99, soft: 99, highlight: 'abc', shade: 'nope' }] },
  { name: 'grad colors', color: '#123456', fill: { colors: ['#f00', 'bad', '#00f', '#0f0'], angle: -45 } },
  { name: 'grad one', fill: { stops: [{ color: '#f00' }] } },
  { name: 'grad order', fill: { stops: [{ color: '#f00', at: 0.8 }, { color: '#0f0', at: 0.2 }, { color: '#00f', pos: 0.5 }, { color: '#fff', offset: 2 }], angle: 725 } },
  { name: 'grad null', fill: null, gradient: '#ff0000' },
  { name: 'legacy', color: '#fff', gradient: '#ff8800', stroke: { color: '#000', width: 5 }, shadow: { color: '#111', x: 2, y: 3, blur: 4, opacity: 0.7 } },
  { name: 'legacy glow', effects: [{ type: 'glow', color: '#0ff', size: 9, opacity: 0.4 }, { type: 'shadow', y: 2 }] },
  { name: 'legacy null shadow', shadow: { opacity: null }, stroke: { width: '3' } },
  { name: 'off only', effects: [{ type: 'stroke', width: 3, off: true }, { type: 'shadow', off: true }] },
  { name: 'array', effects: {}, fill: [] },
];
const styles = STYLE_RAWS.map((r) => ts.normalizeTextStyle(r, 'fb'));
out.styles = STYLE_RAWS.map((r, i) => ({ raw: r, style: JSON.stringify(styles[i]) }));
out.reach = styles.map((s) => ts.effectsReach(s));
out.looks = styles.map((s) => ts.styleLook(s));
out.scaled = styles.flatMap((s) => [0.5, 1.0004, 2.731, 0, -1].map((k) => JSON.stringify(ts.scaleTextStyle(s, k))));
const MERGES: unknown[] = [
  { color: '#ff0000' }, { gradient: '#00ff00' }, { stroke: { color: '#fff', width: 4 } }, { stroke: null }, { shadow: { x: 1, y: 2 } }, { shadow: null },
  { effects: [{ type: 'glow', size: 5 }] }, { fill: null }, { fill: { stops: ['#000', '#fff'] } }, { name: 'renamed', size: 50, font: 'Bangers' },
  { stroke: { width: 0 }, color: '#00f' }, { stroke: { type: 'glow', width: 3 } },
];
out.merges = [styles[0], styles[8], ts.TEXT_STYLE_DEFAULT].flatMap((cur) => MERGES.map((m) => JSON.stringify(ts.mergeTextStyle(cur, m as Record<string, unknown>))));
out.defaults = [ts.TEXT_STYLE_DEFAULT, styles[0], { ...ts.TEXT_STYLE_DEFAULT, size: 10 }].flatMap((s) => ts.TEXT_EFFECT_TYPES.map((t: string) => ts.defaultEffect(t, s)));
out.lookSpec = ts.styleLookSpec('px at that size');
out.stylePrompt = [ts.buildStylePrompt(styles[0], ' gold please ', 0), ts.buildStylePrompt(styles[8], 'x', 3)];
const EXTRACT_REQS = [{ width: 1024, height: 576 }, { width: 300, height: 120, hint: '  the title  ', matte: '#808080' }, { width: 400, height: 100, fullWidth: 1600 }, { width: 400, height: 100, fullWidth: 405 }];
out.extractPrompts = EXTRACT_REQS.map((r) => ts.buildStyleExtractPrompt(r));
const EXTRACT_ANSWERS = [
  '```json\n{"text":"  VICTORY\\n  now ","name":"title gold","role":"title","font":"Lilita One","weight":400,"capHeight":71,"screenWidth":540,"color":"#FFE45C","fill":{"angle":90,"stops":[{"color":"#fff3a0","at":0},{"color":"#c06000","at":1}]},"effects":[{"type":"outline","color":"#5a2a00","width":0.3},{"type":"extrude","y":7},{"type":"shadow","y":5,"blur":"8","spread":null}],"letterSpacing":1.3,"uppercase":true,"description":"big title"}\n```',
  '{"text":"12","name":"num","role":"nope","font":"Russo One","capHeight":20,"color":"#fff","gradient":"#888","stroke":{"color":"#000","width":2},"shadow":{"color":"#000","y":3,"blur":-2},"letterSpacing":"x"}',
  '{"none": true}', 'no json', '{"font":"Roboto"}', '{"size":40,"color":"#f00"}', '{"capHeight":10}', '{"capHeight":"15","font":"Play","screenWidth":10,"shadow":{}}',
  '{"capHeight":30,"color":"#00f","stroke":{"width":0.2},"shadow":{"x":1,"y":1,"blur":1,"opacity":null}}',
];
out.extractAnswers = EXTRACT_ANSWERS.flatMap((a) => [[600, {}], [600, { name: '  my name ', role: 'button' }], [2000, { name: '', role: 'nope' }]].map(([w, over]) => {
  const r = ts.parseStyleExtractAnswer(a, { width: w as number }, over as Record<string, unknown>);
  return { a, w, over, r: r === null ? null : r === 'none' ? 'none' : JSON.stringify(r) };
}));
{
  const ex = ts.parseStyleExtractAnswer(EXTRACT_ANSWERS[0], { width: 600 }) as any;
  out.overrides = [[{}, ''], [{ name: 'other one', role: 'caption' }, ''], [{ name: '!!!' }, ''], [{ role: 'title' }, 'plain']].map(([over, kind]) => {
    const base = kind === 'plain' ? { ...styles[0] } : ex;
    return JSON.stringify(ts.applyStyleOverrides(base, over as Record<string, unknown>));
  });
}
out.answerObjects = ['x ```JSON\n{"a":1}\n``` y', '{"a": {"b": 2}} tail }', '[1]', '{bad}', 'none'].map((a) => j(ts.answerObject(a)));
out.sample = [styles[0], styles[3], ts.TEXT_STYLE_DEFAULT, ts.normalizeTextStyle({ size: 300, effects: [{ type: 'glow', size: 40 }] })]
  .map((s) => JSON.stringify(sample.sampleDoc(s, 'Hello world', '#202020')));

// --- the target device --------------------------------------------------------------------------------------
out.devices = device.UI_DEVICES;
out.targets = [undefined, null, 5, {}, { id: 'ipad' }, { id: 'nope', w: 1000, h: 2000, dpi: 300, name: ' My phone ' }, { w: 63, h: 100, dpi: 300 }, { w: 100.4, h: 9000, dpi: 300 },
  { w: '800', h: '1200', dpi: '0' }, { w: 800, h: 1200, dpi: 30 }, { w: 800, h: 1200, dpi: 200, name: 'x'.repeat(60) }, [1, 2]].map((r) => [j(r), j(device.cleanTarget(r))]);
out.targetConfigs = [{}, { kitDevice: 'pixel-8' }, { kitDevice: 'custom', kitDeviceCustom: { w: 1000, h: 2000, dpi: 400 } }, { kitDevice: 'custom' }, { kitDevice: 'custom', kitDeviceCustom: null },
  { kitDevice: 'nope' }, { kitDevice: 5 }, { kitDevice: 'custom', kitDeviceCustom: { w: 10, h: 10, dpi: 400 } }].map((c) => device.targetOfConfig(c));
const SCREENS = [{ w: 1179, h: 2556, dpi: 460 }, { w: 2064, h: 2752, dpi: 264 }, { w: 1920, h: 1080, dpi: 96 }];
const SOURCES = [[768, 1376], [1080, 1920], [1920, 1080], [500, 500], [0, 100], [33, 7]];
out.fits = SCREENS.flatMap((t) => SOURCES.map(([w, h]) => [device.sourceFit(t, w, h), device.sourceOnTarget(t, w, h)]));
out.units = SCREENS.flatMap((t) => [{ width: 768, height: 1376 }, { width: 1080, height: 1920, dpi: 480 }, { width: 1179, height: 2556, dpi: 460 }, { width: 750, height: 1334, dpi: 326 }]
  .map((d) => [device.inTargetUnits(d, t), device.toTargetUnits(d, t)]));
out.aspects = [[1179, 2556], [1920, 1080], [1080, 1920], [2064, 2752], [1000, 1000], [2560, 1664], [3440, 1440], [1000, 2000], [750, 1334], [1170, 2532]].map(([w, h]) => device.aspectName(w, h));
out.frames = [{ width: 1080, height: 1920 }, { width: 1179, height: 2112, dpi: 460 }, { width: 2000, height: 1000 }].flatMap((d) => device.UI_DEVICES.map((s: any) => device.previewFrame(d, s)));

// --- kit: purposes, device fields, merge ---------------------------------------------------------------------
const KIT_JSON = JSON.stringify({ version: 1, name: 'Purposeful', device: { id: 'iphone-16', name: 'ignored', w: 1, h: 1, dpi: 1 }, elements: [
  { name: 'btn_main', type: 'button', assetType: 'sliced', width: 300, height: 100, png: 'AAAA', nineSlice: { left: 20, top: 20, right: 20, bottom: 20 }, purpose: '  backing of the\nmain   action button  ',
    description: 'green glossy', dpi: 460, dpiFrom: 'kit', inches: { w: 0.652, h: '0.217' }, sizeFrom: 'source', share: 0.254, soft: true, texture: { w: 600, h: 200 }, own: true },
  { name: 'panel_popup', type: 'panel', png: 'AAAA', width: 400, height: 300, purpose: 'p'.repeat(250), dpiFrom: 'kit', inches: { w: 0, h: 1 }, sizeFrom: 'nope', share: -1, soft: 'yes', texture: { w: 5 } },
  { name: 'icon_coin', type: 'icon', png: 'AAAA', width: 64, height: 64, purpose: 5, dpiFrom: 'item', sizeFrom: 'tag', share: '0.1' },
  { name: 'bg_menu', type: 'background', png: 'AAAA', width: 1179, height: 2556, purpose: 'background of the main menu', sizeFrom: 'pixels' },
], textStyles: [{ name: 'title', font: 'Lilita One', size: 90, fill: { stops: ['#fff', '#fa0'] }, effects: [{ type: 'stroke', width: 6 }, { type: 'extrude', y: 6 }] }] });
{
  const kit = kitm.parseKit(KIT_JSON);
  out.kit = { json: KIT_JSON, parsed: JSON.stringify(kit), summary: [kitm.kitSummary(kit), kitm.kitSummary(kit, 460)],
    customDevice: JSON.stringify(kitm.parseKit(JSON.stringify({ name: 'k', device: { w: 1000, h: 1500, dpi: 300 }, elements: [], textStyles: [] }))),
    badDevice: JSON.stringify(kitm.parseKit(JSON.stringify({ name: 'k', device: { w: 10, h: 10, dpi: 300 }, elements: [], textStyles: [] }))) };
  out.names = ['main_button', 'popup_bg', 'action_btn', 'currency', 'menu_background', 'coin'].map((id) => [id, kit.elements.map((e: any) => kitm.nameScore(id, e))]);
  out.purposes = ['  a\n b  ', '', null, 5, 'x'.repeat(300)].map((v) => j(kitm.cleanPurpose(v)));
}
{
  const cur = {
    a: { name: 'a', nineSlice: { left: 1, top: 1, right: 1, bottom: 1 }, purpose: 'typed by hand' },
    b: { name: 'b', nineSlice: { left: 2, top: 2, right: 2, bottom: 2 }, nineFrom: 'ai', purpose: 'old ai', purposeFrom: 'ai' },
    c: { name: 'c', bg: 'ai', prepared: { uploadId: 'u.png', from: 'h' }, purpose: '   ' },
    d: { name: 'd', purposeFrom: 'ai', purpose: 'kept' },
  };
  const answer = {
    a: { name: 'A', purpose: 'ai purpose', nineSlice: { left: 9, top: 9, right: 9, bottom: 9 }, nineFrom: 'ai', background: 'flat', bg: 'key' },
    b: { name: 'B', purpose: 'new ai', nineSlice: null, background: 'transparent' },
    c: { name: 'C', purpose: 'filled', nineSlice: { left: 3, top: 3, right: 3, bottom: 3 }, nineFrom: 'ai', bg: 'keep' },
    d: { name: 'D' },
    e: { name: 'E', purpose: 'fresh' },
  };
  out.mergeKit = { cur, answer, merged: JSON.stringify(kitm.mergeKitAnalysis(cur as any, answer as any)) };
}

// --- ops: the screen with a scale, the root by name, points in a request -----------------------------------------
{
  const doc = uiml.parseUiml('screen Menu 768x1376\n  panel box anchor=center size=600x400 margin=10,20 pad=5\n    row r gap=10 h=120\n      button a w=100 "A"\n    text t fontSize=33.3 "Hi"\n  grid g cols=2 cell=100x50 gap=4,6').doc;
  const opsList = [
    [{ op: 'screen', width: 1179, height: 2112, dpi: 460, scale: 1.53516, fromDpi: 480 }],
    [{ op: 'screen', scale: 1 }], [{ op: 'screen', scale: '2' }], [{ op: 'screen', scale: -2 }], [{ op: 'screen', scale: 'x' }],
    [{ op: 'update', id: 'menu', set: { color: '#ff0000' } }], [{ op: 'update', id: 'SCREEN', set: { color: '#00ff00' } }],
    [{ op: 'add', parent: 'Menu', nodes: [{ id: 'x', type: 'icon' }] }], [{ op: 'move', id: 'a', parent: 'screen' }],
    [{ op: 'update', id: 5, set: {} }], [{ op: 'add', parent: 7, nodes: [] }], [{ op: 'move', id: 'a', parent: 8 }], [{ op: 'update', id: 'nope', set: {} }],
  ];
  out.opsScale = { src: uiml.docToUiml(doc), cases: opsList.map((list) => { const r = ops.applyOps(doc, list as any); return { ops: list, doc: JSON.stringify(r.doc), errors: r.errors }; }) };
}
{
  const pts = [{ x: 10.4, y: 20.6, text: '' }, { x: 5000, y: -3, text: '  note two ' }, { x: 'x', y: 1 }, { x: '7', y: '8', text: null }, null, 5, { x: 1, y: 2, text: 'n'.repeat(2100) }];
  out.points = { raw: pts, clean: [ops.cleanPoints(pts), ops.cleanPoints(pts, 1000, 1500), ops.cleanPoints('x'), ops.cleanPoints(Array.from({ length: 30 }, (_, i) => ({ x: i, y: i })))] };
  const P = [{ x: 1, y: 2, text: '' }, { x: 3, y: 4, text: 'old note' }, { x: 5, y: 6, text: '' }, { x: 7, y: 8, text: '  ' }];
  const DRAFTS = ['make [2] larger and move it under [1]', '[3] [3] [1] [9] [0] [12] [[1]] [123] [ 1]', '', '   ', 'no pins', 'line\n[4] tail  ', '[2]'];
  out.pins = DRAFTS.map((d) => ({ d, named: ops.pinsNamed(d, P.length), notes: ops.draftWithNotes(d, P), req: ops.requestWithPins(d, P) }));
  out.pinsPoints = P;
}

// --- the node's own kit and its outputs ---------------------------------------------------------------------------
{
  const asset = (name: string, extra: Record<string, unknown> = {}) => ({ name, uploadId: `${name}.png`, type: 'panel', assetType: 'sliced', width: 200, height: 100, dpi: 480, ...extra });
  const steps = [
    { id: 's1', by: 'agent', at: '', ops: [], assets: [asset('Plate'), asset('icon_star', { type: 'icon', assetType: 'simple', nineSlice: { left: 3 }, dpi: undefined }), { name: 'bad', uploadId: 'a/b.png' }, { uploadId: 'x.png' }, null],
      styles: [{ name: 'title', size: 60, effects: [{ type: 'stroke', width: 4 }] }, { size: 20 }] },
    { id: 's2', by: 'user', at: '', ops: [{ op: 'screen', width: 1179, height: 2112, dpi: 460, scale: 1.53516, fromDpi: 480 }] },
    { id: 's3', by: 'agent', at: '', ops: [], assets: [asset('plate', { purpose: 'plate behind', from: 'cut', variant: 'v', description: 'd' }), asset('Image', { from: 'drawn' }), asset('layout'), asset('image_2')] },
    { id: 's4', by: 'agent', at: '', ops: [{ op: 'screen', scale: 2 }], styles: [{ name: 'body', size: 30 }] },
  ];
  const config = { steps, uiRedo: [{ id: 'r', by: 'agent', at: '', ops: [], assets: [asset('redo_one')] }], uploadId: ' up.png ', items: { p1: { prepared: { uploadId: 'prep.png' } }, p2: null, p3: { prepared: { uploadId: 'a\\b' } } } };
  out.own = { config, kit: JSON.stringify(own.ownKitOf(steps as any)), assets: JSON.stringify(own.ownAssetsOf(steps as any)), uploads: own.ownUploadIds(steps as any),
    configUploads: uploads.configUploadIds(config), element: [JSON.stringify(own.ownElement(own.ownKitOf(steps as any).assets[0], 'PNG')), JSON.stringify(own.ownElement({ name: 'n', type: 'icon', assetType: 'simple', uploadId: 'u', width: 1, height: 1 }))],
    ports: JSON.stringify(ports.layoutOwnPorts(config)), ids: ['Plate', 'My Graphic!', 'Ünï', 'a-b_c'].map((n) => ports.layoutOwnPortId(n)),
    outputs: JSON.stringify(ports.layoutOutputs({ id: 'n', type: 'uiLayout', config, inputs: [], outputs: [{ id: 'image', name: 'image', type: 'image' }, { id: 'layout', name: 'layout', type: 'text' }, { id: 'keep-me', name: 'plate', type: 'image' }, { id: 'gone', name: 'gone', type: 'image' }] } as any)),
    byName: ['plate', 'PLATE', 'image', 'image_3', 'nope'].map((n) => j(ports.layoutOwnPortByName(config, n))),
    clean: [{ name: 'x y', uploadId: ' id.png ', type: 'nope', assetType: 'nope', width: 0, height: 'x', variant: 'v'.repeat(50), purpose: 'p'.repeat(400), dpi: 0, from: 'other' }, { name: '!!!', uploadId: 'i' }].map((r) => j(own.cleanOwnAsset(r))) };
  const k1 = kitm.parseKit(KIT_JSON);
  const k2 = kitm.parseKit(JSON.stringify({ name: 'kit', device: { w: 800, h: 1200, dpi: 200 }, elements: [{ name: 'btn_main', png: 'BBBB' }, { name: 'BTN_MAIN', png: 'CCCC' }], textStyles: [{ name: 'Title' }] }));
  const k3 = kitm.parseKit(JSON.stringify({ name: 'kit', elements: [{ name: 'btn_main_2', png: 'DDDD' }], textStyles: [] }));
  out.mergeKits = [[k1, k2, k3], [null, k2], [k3, k1], [], [null]].map((ks) => JSON.stringify(own.mergeKits(ks as any)));
  out.freeNames = [['btn_main', []], ['  new graphic ', []], ['icon_coin', ['icon_coin_2']], ['!!!', []]].map(([w, also]) => own.freeKitName(k1, w as string, also as string[]));
}

// --- the agent: kit additions, flow words, the turns' prompt ----------------------------------------------------------
const ANSWERS = [
  'Here it is.\n```uiml-edit\n@style title role=title font="Lilita One" weight=400 size=96 color=#FFE45C gradient=#FF8A00,#C06000,0deg stroke=#5A2A00,8 stroke=#FFFFFF,3,0.8 extrude=#3A1800,0,7 shadow=#000000,0,6,10,0.5 glow=#FFFF00,12,0.6 inner=#FFFFFF,0,2,2,0.4 bevel=#FFFFFF,#000000,3,0.5 spacing=2 uppercase=yes align=left "big titles"\n@style title size=50\n  @STYLE body\n@asset plate_wood type=panel asset=sliced size=600x400 box=10,20,300,200 purpose="wooden plate" "a wooden plate"\n@asset Coin! type=icon size=64\n@asset - \n@asset\n@style\n@update play text="GO"\n```\n@done',
  'Nothing to change.\n```\n@done\n```',
  '`@markup`',
  '@DONE.\nall good',
  'Thanks @done for that',
  '@asset bg type=background size=1179x2556 "night sky"\n@asset x_y-z size=10×20 box=1,2,3\n@asset q size=*\nscreen S 100x100\n  button b "B"',
  '```uiml\n```\nA reply\n```uiml-edit\n@update play text="X"\n```',
];
{
  const doc = uiml.parseUiml('screen Menu 1080x1920\n  button play "PLAY"').doc;
  out.agentAnswers = ANSWERS.map((a) => {
    const r = agent.parseAgentAnswer(a, doc);
    return { a, reply: r.reply, ops: JSON.stringify(r.ops), errors: r.errors, styles: JSON.stringify(r.styles), assets: JSON.stringify(r.assets), done: r.done, markup: r.markup };
  });
  out.kitAdditions = ANSWERS.map((a) => JSON.stringify(agent.parseKitAdditions(a)));
  out.additionsSpec = [agent.UI_KIT_ADDITIONS_SPEC(false), agent.UI_KIT_ADDITIONS_SPEC(true)];
}
{
  const kit = kitm.parseKit(KIT_JSON);
  const doc = uiml.parseUiml('screen Menu 1080x1920 dpi=460\n  panel box anchor=center size=600x400\n    button play anchor=center size=200x100 "PLAY"\n  text title anchor=top-center pos=0,100 size=600x120 "Hi"').doc;
  const empty = uiml.parseUiml('screen Menu 1179x2556 dpi=460').doc;
  const base = { doc, kit, steps: [], prompt: ' make [1] bigger ', references: [{ name: 'ref', kind: 'image', w: 768, h: 1376 }, { name: 'notes', kind: 'text', text: 'hello' }, { name: 'ref2', kind: 'image' }] };
  const run = { turn: 2, maxTurns: 6, log: ['turn 1: moved play', 'drafted'], built: true,
    compare: { score: 0.734, gridStep: 100, regions: [{ box: { x: 10.4, y: 20.6, w: 300.2, h: 99.5 }, share: 0.3, blocks: ['play', 'box'] }, { box: { x: 0, y: 0, w: 5, h: 5 }, share: 0.1, blocks: [] }], zoom: true },
    findings: ['icon_coin unused while coin has none'], canMarkup: true };
  const ctxs = [
    { ...base, canDraw: true, withScreenshot: true, withKitSheet: true, points: [{ x: 540, y: 960, text: '' }, { x: 5, y: 5, text: 'corner note' }, { x: 540, y: 160, text: '' }], pinned: true, lang: 'Russian' },
    { ...base, withScreenshot: true, run },
    { ...base, run: { turn: 6, maxTurns: 6, log: [], built: false, findings: [] }, points: [{ x: 1, y: 1, text: '' }] },
    { ...base, doc: empty, withKitSheet: true, run: { ...run, turn: 1, compare: { ...run.compare, regions: [], zoom: false } } },
  ];
  out.turnPrompts = ctxs.map((c) => agent.buildAgentPrompt(c as any));
  out.blocksAt = [[540, 960], [5, 5], [540, 160], [2000, 2000]].map(([x, y]) => agent.blocksAt(doc, x, y));
}

// --- PNG source tag ---------------------------------------------------------------------------------------------------
{
  const tiny = Buffer.from('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==', 'base64');
  const tagged = png.withPngDpi(tiny, 460, 1.23456, { w: 768.4, h: 1376 });
  out.png = { tiny: tiny.toString('base64'), tagged: Buffer.from(tagged).toString('base64'), source: j(png.readPngSource(tagged)),
    noSource: Buffer.from(png.withPngDpi(tiny, 460, undefined, { w: 0, h: 5 })).toString('base64'),
    bad: ['', '12x', 'x12', '0x5', '12x34x5', '1.5x2', ' 12x34'].map((v) => j(png.readPngSource(png.writePngText(tiny, { 'AssetsLine:source': v })))) };
}

fs.writeFileSync(process.argv[2], JSON.stringify(out));
console.log('ok', Object.keys(out).length, JSON.stringify(out).length);
