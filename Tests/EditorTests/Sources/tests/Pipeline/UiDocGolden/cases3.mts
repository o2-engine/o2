// Golden references, round 3 (AssetsLine after 77d5b09): ui_structure.json -> layout (shared/uiDoc/fromStructure.ts) and the
// markup's analysis -> layout (backend/src/uiLayout/draft.ts), with parse.ts's kit / opacity / colour / rotation fields and its
// holds rule. Real structures are read from the AssetsLine checkout's data/ when they are there (skipped otherwise).
//   AL=<AssetsLine checkout>; ASSETSLINE=$AL $AL/backend/node_modules/.bin/tsx cases3.mts /tmp/golden3.json
//   python3 gen_tests.py /tmp/golden3.json ../UiDocGolden3.inc kUiDocGolden3Parts
import fs from 'node:fs';
const A = process.env.ASSETSLINE ?? '/Users/andreizenkovich/work/zenkovich.space/AssetsLine';
const fromStructure = await import(`${A}/shared/uiDoc/fromStructure.ts`);
const draft = await import(`${A}/backend/src/uiLayout/draft.ts`);
const parse = await import(`${A}/backend/src/uiLayout/parse.ts`);
const kitm = await import(`${A}/shared/uiKit.ts`);
const ts = await import(`${A}/shared/textStyle.ts`);

const out: Record<string, unknown> = {};
type J = Record<string, unknown>;

// --- the kit -------------------------------------------------------------------------------------------------------
const el = (name: string, type: string, extra: J = {}) => ({ name, type, assetType: kitm.defaultAssetType(type), width: 200, height: 100, png: 'AAAA', ...extra });
const KIT = { version: 1, name: 'golden3', elements: [
  el('btn_green', 'button', { width: 300, height: 100 }), el('btn_blue_round', 'button', { assetType: 'simple', width: 120, height: 120 }),
  el('panel_wood', 'panel', { width: 600, height: 400 }), el('tab_main', 'tab'), el('badge_red', 'badge', { width: 60, height: 60 }),
  el('icon_coin', 'icon', { width: 64, height: 64 }), el('icon_close', 'icon', { width: 64, height: 64 }),
  el('bar_track', 'progressBar', { width: 400, height: 40 }), el('bar_fill', 'progressFill', { width: 400, height: 40 }),
  el('bg_menu', 'background', { width: 1080, height: 1920 }), el('frame_gold', 'frame'), el('slot_wood', 'slot', { width: 200, height: 200 }),
  el('header_ribbon', 'header', { width: 600, height: 120 }), el('divider_line', 'divider', { width: 400, height: 6 }),
  el('sticker_star', 'image', { assetType: 'simple', width: 120, height: 80 }), el('Coin_Big', 'other', { width: 128, height: 128 }),
  el('input_field', 'input'), el('toggle_on', 'toggle'), el('slider_bar', 'slider'), el('slider_knob', 'sliderHandle'),
], textStyles: [
  { name: 'title', role: 'title', font: 'Lilita One', size: 96, color: '#FFE45C', stroke: { color: '#5A2A00', width: 6 } },
  { name: 'button', role: 'button', font: 'Fira Sans', weight: 900, size: 48, color: '#FFFFFF', stroke: { color: '#1A4A00', width: 4 } },
  { name: 'body', role: 'body', font: 'Roboto', size: 36, color: '#FFFFFF' },
  { name: 'number', role: 'number', font: 'Russo One', size: 40, color: '#FFD700' },
  { name: 'Caption', role: 'caption', font: 'Roboto', weight: 500, size: 24, color: '#C0C0C0', align: 'left' },
] };
const KIT_JSON = JSON.stringify(KIT);
out.kit = KIT_JSON;
const kit = kitm.parseKit(KIT_JSON);

// --- structures ------------------------------------------------------------------------------------------------------
const pos = (x: number, y: number, w: number, h: number) => ({ position: { x, y }, size: { width: w, height: h } });
const img = (name: string, asset: unknown, x: number, y: number, w: number, h: number, extra: J = {}) => ({ type: 'image', name, asset, ...pos(x, y, w, h), ...extra });
const txt = (name: string, text: unknown, x: number, y: number, w: number, h: number, extra: J = {}) => ({ type: 'text', name, text, ...pos(x, y, w, h), ...extra });
const blk = (type: string, name: unknown, x: number, y: number, w: number, h: number, extra: J = {}) => ({ type, name, ...pos(x, y, w, h), ...extra });
const kids3 = (name: string, xs: number[], ys: number[], w: number | number[], h: number | number[]) =>
  xs.map((x, i) => img(name, 'slot_wood.png', x, ys[i], Array.isArray(w) ? w[i] : w, Array.isArray(h) ? h[i] : h));

const POPUP = { canvas: { width: 1080, height: 1920, name: 'Reward Popup' }, root: { type: 'container', name: 'root', ...pos(0, 0, 1080, 1920), children: [
  img('backdrop', 'assets/bg_menu.png', 0, 0, 1080, 1920, { anchor: { horizontal: 'center', vertical: 'middle' }, responsive: { min: { x: 0, y: 0 }, max: { x: 1, y: 1 } } }),
  blk('rect', 'dim', 0, 0, 1080, 1920, { color: '#000000', opacity: 0.55 }),
  blk('container', 'popup', 90, 480, 900, 960, { anchor: { horizontal: 'center', vertical: 'middle' }, children: [
    img('popup_bg', './panel_wood.png', 0, 0, 900, 960, { color: '#ffeedd' }),
    txt('title', 'VICTORY!', 50, 40, 800, 120, { fontFamily: 'fonts/LilitaOne-Regular.ttf', fontSize: 96, color: '#ffe45c', strokeColor: '#5a2a00', strokeWidth: 6, alignment: 'center', anchor: { horizontal: 'center', vertical: 'top' } }),
    txt('subtitle', 'Level 5 complete', 60, 170, 500, 50, { fontFamily: 'Roboto-Medium.ttf', fontSize: 40, color: '#ffffff', alignment: 'left' }),
    blk('container', 'reward_row', 50, 260, 800, 200, { layout: { type: 'row', spacing: 100 }, children: [
      img('slot', 'slot_wood.png', 0, 0, 200, 200, { children: [
        img('icon', 'ICON_COIN', 50, 30, 100, 100),
        txt('count', '×25', 0, 140, 200, 50, { fontSize: 36, color: '#FFD700', fontFamily: 'RussoOne-Regular.ttf' }),
      ] }),
      img('slot', 'slot_wood.png', 300, 0, 200, 200), img('slot', 'slot_wood.png', 600, 0, 200, 200),
    ] }),
    blk('button', 'play_btn', 250, 780, 400, 130, { anchor: { horizontal: 'center', vertical: 'bottom' }, children: [
      img('play_bg', 'btn_green.png', 0, 0, 400, 130),
      txt('play_label', 'PLAY', 0, 30, 400, 70, { fontFamily: 'FiraSans-Black.ttf', fontSize: 52, color: '#ffffff', strokeColor: '#1a4a00', strokeWidth: 4, alignment: 'center' }),
    ] }),
    blk('button', 'close', 820, 10, 70, 70, { asset: 'icon_close.png', anchor: { horizontal: 'right', vertical: 'top' } }),
    blk('container', 'hp_bar', 100, 520, 700, 50, { progress: { value: 30, min: 0, max: 100, fill: 'hp_fill' }, children: [
      img('hp_fill', 'bar_fill.png', 0, 0, 210, 50), img('hp_track', 'bar_track.png', 0, 0, 700, 50),
      txt('hp_text', '30/100', 0, 5, 700, 40, { fontSize: 30, color: '#fff' }),
    ] }),
    blk('container', 'shop_tab', 50, 600, 250, 80, { children: [img('tab_img', 'tab_main.png', 1, 0, 249, 81), txt('tab_text', 'Shop', 0, 20, 250, 40, { fontSize: 32, color: '#333333' })] }),
    blk('container', 'news', 320, 600, 80, 80, { children: [img('news_bg', 'badge_red.png', 0, 0, 80, 80), txt('news_count', '3', 0, 20, 80, 40, { fontSize: 30 })] }),
    blk('container', 'ribbon', 420, 600, 300, 80, { children: [
      img('header_img', 'ribbon_header.png', 0, 0, 300, 80, { color: '#ff8800' }), txt('ribbon_text', 'Daily', 0, 20, 300, 40, { fontSize: 30 }), txt('ribbon_text2', 'bonus', 0, 50, 300, 20, { fontSize: 20 }),
    ] }),
    blk('container', 'buy_btn', 740, 600, 120, 80, { children: [img('buy_bg', 'frame_gold.png', 0, 0, 120, 80), txt('buy_text', 'Buy', 0, 20, 120, 40, { fontSize: 28 })] }),
    blk('container', 'plain_btn', 50, 700, 200, 60, { children: [img(undefined as unknown as string, undefined, 0, 0, 200, 60), txt('plain_text', 'Go', 0, 10, 200, 40, { fontSize: 28 })] }),
  ] }),
  blk('container', 'tiles', 220, 1480, 640, 430, { layout: { type: 'grid', columns: 3, cellSize: { width: 200, height: 200 }, spacing: { x: 20, y: 30 } },
    children: kids3('tile', [0, 220, 440, 0, 220], [0, 0, 0, 230, 230], 200, 200) }),
  blk('container', 'top_bar', 0, 0, 1080, 120, { color: '#203040', children: [
    img('coin', 'sprites/coin_large.png', 20, 20, 80, 80), txt('coins', '1,250', 110, 30, 200, 60, { fontSize: 48, color: '#ffd700', fontFamily: 'fonts\\RussoOne-Regular.ttf' }),
    blk('button', 'settings', 980, 20, 80, 80, { asset: 'btn_blue_round.png', anchor: { horizontal: 'right', vertical: 'top' } }),
  ] }),
] } };

const LAYOUTS = { canvas: { width: 1000, height: 2400 }, root: { children: [
  blk('container', 'row_center', 0, 0, 1000, 200, { layout: { type: 'row' }, children: kids3('a', [150, 400, 650], [0, 0, 0], 200, 200) }),
  blk('container', 'row_irregular', 0, 220, 1000, 200, { layout: { type: 'row', spacing: 50 }, children: kids3('b', [0, 250, 600], [0, 0, 0], 200, 200) }),
  blk('container', 'row_overlap', 0, 440, 1000, 200, { layout: { type: 'row' }, children: kids3('c', [0, 190, 380], [0, 0, 0], 200, 200) }),
  blk('container', 'col_between', 0, 660, 300, 500, { layout: { type: 'column', align: 'space-between' }, children: kids3('d', [0, 0, 0], [0, 200, 400], 300, 100) }),
  blk('container', 'col_centred', 320, 660, 400, 400, { layout: { type: 'column' }, children: kids3('e', [100, 50], [50, 200], [200, 300], 100) }),
  blk('container', 'row_offsets', 0, 1180, 600, 200, { layout: { type: 'row', spacing: 10 }, children: kids3('f', [0, 110, 220], [0, 20, 50], 100, 100) }),
  blk('container', 'row_end', 620, 1180, 380, 200, { layout: { type: 'row' }, children: kids3('g', [0, 110], [100, 50], 100, [100, 150]) }),
  blk('container', 'row_hidden', 0, 1400, 600, 100, { layout: { type: 'row' }, children: [img('h1', 'slot_wood.png', 0, 0, 100, 100), img('h2', 'slot_wood.png', 110, 0, 100, 100, { visible: false })] }),
  blk('container', 'grid_number', 0, 1520, 410, 410, { layout: { type: 'grid', columns: '2', spacing: 10 }, children: kids3('i', [0, 210, 0], [0, 0, 210], 200, 200) }),
  blk('container', 'grid_padded', 420, 1520, 580, 500, { layout: { type: 'grid', columns: 2, cellSize: { width: 200, height: 200 }, spacing: { x: 20, y: 20 } },
    children: kids3('j', [30, 250, 30], [20, 20, 240], 200, 200) }),
  blk('container', 'grid_fail', 0, 2040, 600, 300, { layout: { type: 'grid', columns: 3, cellSize: { width: 100, height: 100 } }, children: kids3('k', [0, 150, 420], [0, 0, 10], 100, 100) }),
  blk('container', 'row_around', 620, 2040, 380, 100, { layout: { type: 'row', align: 'space-around' }, children: kids3('l', [20, 200], [0, 0], 100, 100) }),
  blk('container', 'row_evenly', 620, 2150, 380, 100, { layout: { type: 'row', align: 'space-evenly' }, children: kids3('m', [60, 220], [0, 0], 100, 100) }),
  blk('container', 'row_one', 620, 2260, 380, 100, { layout: { type: 'row', align: 'center' }, children: kids3('n', [140], [0], 100, 100) }),
  blk('container', 'row_absorbed', 0, 2350, 400, 50, { layout: { type: 'row' }, children: [img('o_bg', 'panel_wood.png', 0, 0, 400, 50), img('o1', 'icon_coin.png', 10, 0, 50, 50)] }),
] } };

const ROOT_ROW = { root: { size: { width: 750, height: 1334 }, layout: { type: 'row' }, children: kids3('card', [25, 275, 525], [467, 467, 467], 200, 400) } };
const ROOT_GRID = { canvas: { width: 750, height: 1334, name: 'Grid Screen' }, root: { layout: { type: 'grid', columns: 2, cellSize: { width: 100, height: 100 } }, children: [
  img('g1', 'slot_wood.png', 0, 0, 100, 100, { anchor: { horizontal: 'right', vertical: 'bottom' } }),
  img('g2', 'slot_wood.png', 333, 50, 100, 100, { responsive: { min: { x: 0, y: 0.5 }, max: { x: 1, y: 0.5 } } }),
  img('g3', 'slot_wood.png', 0, 600, 750, 100), txt('g4', 'centred', 300, 600, 150, 50, { anchor: { horizontal: 'center', vertical: 'middle' }, fontSize: 30 }),
] } };

const missing = Array.from({ length: 14 }, (_, i) => img(`m${i}`, `gone/missing_${i}.png`, i * 10, 0, 10, 10));
const ODDS = { canvas: { width: 800, height: 1200, name: 'odd   names!' }, root: { children: [
  blk('container', 'item', 0, 0, 800, 100, { state: 'normal', scroll: { vertical: true }, children: [blk('container', 'item', 0, 0, 100, 100, { role: null }), blk('container', 'x'.repeat(70), 0, 0, 100, 100, { children: [blk('container', 'item', 0, 0, 10, 10)] })] }),
  blk('container', 'item', 0, 100, 100, 100, { hueShift: 20, textScaleX: 0.9 }),
  blk('container', 'My Button', 0, 200, 100, 100), blk('container', 'Кнопка', 0, 300, 100, 100), blk('container', '', 0, 400, 100, 100), blk('container', undefined, 0, 500, 100, 100),
  blk('container', 'my button', 0, 600, 100, 100), blk('container', 'block', 0, 700, 100, 100),
  img('faded', 'icon_coin.png', 100, 0, 50, 50, { opacity: 0.5 }), img('gone_op', 'icon_coin.png', 150, 0, 50, 50, { opacity: -1 }),
  img('text_op', 'icon_coin.png', 200, 0, 50, 50, { opacity: '0.3' }), img('full_op', 'icon_coin.png', 250, 0, 50, 50, { opacity: 1.5 }),
  img('tilt1', 'sticker_star.png', 300, 0, 120, 80, { rotation: 15 }), img('tilt2', 'sticker_star.png', 300, 100, 120, 80, { rotation: 370 }),
  img('tilt3', 'sticker_star.png', 300, 200, 120, 80, { rotation: -190 }), img('tilt4', 'sticker_star.png', 300, 300, 120, 80, { rotation: '45' }),
  img('tilt5', 'sticker_star.png', 300, 400, 120, 80, { rotation: 0.04 }), img('hidden_one', 'icon_coin.png', 400, 0, 50, 50, { visible: false }),
  blk('rect', 'line', 0, 800, 800, 4, { color: '#ffffff' }), blk('rect', 'shade', 0, 810, 800, 40), blk('overlay', 'veil', 0, 860, 800, 40),
  blk('container', 'strip', 0, 900, 800, 30, { color: '#123' }), blk('button', 'bare_btn', 0, 940, 200, 60), blk('button', 'color_btn', 220, 940, 200, 60, { color: '#00ff00' }),
  blk('button', 'multi_btn', 440, 940, 300, 80, { children: [txt('l1', 'Buy', 0, 0, 150, 80, { fontSize: 30 }), txt('l2', '99', 150, 0, 150, 80, { fontSize: 30 })] }),
  img('by_name', 5, 0, 1000, 50, 50), img('hero_icon', undefined, 60, 1000, 50, 50), img('progress_bar_deco', undefined, 120, 1000, 100, 20),
  img('big', 'sprites/coin_large.png', 240, 1000, 50, 50), img('upper', 'SLOT_WOOD.PNG', 300, 1000, 50, 50),
  blk('container', 'resp', 0, 1050, 400, 50, { responsive: { min: { x: 0, y: 0 }, max: { x: 1, y: 0 } } }),
  blk('container', 'anch', 600, 1100, 200, 100, { anchor: { horizontal: 'right', vertical: 'bottom' } }),
  txt('t_bold', 'Bold one', 0, 1150, 200, 40, { fontFamily: 'C:\\fonts\\MyFont-Bold.ttf', fontSize: 28 }),
  txt('t_med', 'Medium one', 200, 1150, 200, 40, { fontFamily: 'X-Medium.otf', fontSize: 28, alignment: 'right' }),
  txt('t_heavy', 'Heavy', 400, 1150, 200, 40, { fontFamily: 'Font-HEAVY.ttf', fontSize: 60 }),
  txt('t_num', ' 12 x 3 % ', 600, 1150, 200, 40, { fontSize: 20 }), txt('t_obj', { a: 1 }, 600, 1150, 200, 40, { fontSize: 10, alignment: 'justify' }),
  blk('container', 'prog_default', 0, 1190, 300, 10, { progress: {}, asset: 'bar_track.png', children: [img('pd_fill', 'bar_fill.png', 0, 0, 100, 10)] }),
  blk('container', 'prog_untracked', 300, 1190, 300, 10, { progress: { value: 2, max: 1, fill: 'pf' }, children: [img('pf', 'bar_fill.png', 0, 0, 100, 10), img('pu', 'nope.png', 0, 0, 300, 10)] }),
  blk('container', 'missing_all', 0, 0, 800, 20, { children: missing }),
] } };

const LEGEND = { targets: [
  { path: 'root/popup', abs_bbox: [100, 470, 880, 980] }, { path: 'root/popup/title', abs_bbox: [140, 500, 800, 110] },
  { path: 'root/popup/shop_tab/tab_img', abs_bbox: [140, 1070, 250, 80] }, { path: 'root/popup/plain_btn/undefined', abs_bbox: [140, 1170, 200, 60] },
  { path: 'root/top_bar', abs_bbox: [0, 0, 1080, 120] }, { path: 5, abs_bbox: [1, 2, 3, 4] }, { path: 'root/dim', abs_bbox: [0, 0, 1080] },
  { path: 'root/tiles', abs_bbox: [220, 1480, 'x', 430] }, null, { path: 'root/backdrop', abs_bbox: ['0', null, '1080', [1920]] },
] };

const S_CASES: { name: string; structure: unknown; legend?: unknown; kit?: string | null; assetNames?: [string, string][]; dpi?: number }[] = [
  { name: 'popup', structure: POPUP, kit: KIT_JSON, assetNames: [['coin_large.png', 'Coin_Big'], ['ghost.png', 'not_in_kit']], dpi: 460 },
  { name: 'popup with legend', structure: POPUP, legend: LEGEND, kit: KIT_JSON, assetNames: [['coin_large.png', 'COIN_BIG']] },
  { name: 'popup without a kit', structure: POPUP, kit: null, dpi: 0 },
  { name: 'layouts', structure: LAYOUTS, kit: KIT_JSON },
  { name: 'root row', structure: ROOT_ROW, kit: KIT_JSON, dpi: 30 },
  { name: 'root grid', structure: ROOT_GRID, kit: KIT_JSON },
  { name: 'odds', structure: ODDS, kit: KIT_JSON, assetNames: [['coin_large.png', 'coin_big']] },
  { name: 'null', structure: null, kit: KIT_JSON }, { name: 'number', structure: 5, kit: null }, { name: 'empty', structure: {}, kit: KIT_JSON },
  { name: 'string children', structure: { root: { children: 'abc' } }, kit: KIT_JSON },
  { name: 'loose', structure: { canvas: { width: '800', height: null }, root: { children: ['str', 5, { type: 'text' }, [1]] } }, kit: KIT_JSON },
  { name: 'array', structure: [1, 2], kit: KIT_JSON },
];

// Real structures the skill wrote, with their legends; a kit made from their sprite names (some by assetNames, some missing)
const REAL = [
  ['data/node-work/7b2cfdd1-f86d-4e11-aae8-8fe10cdbed06/11f5282c-dbdf-4e39-a01e-34872d81479d/task', true],
  ['data/node-work/7b2cfdd1-f86d-4e11-aae8-8fe10cdbed06/aa78edd9-7ec3-43de-9757-10c7061282c4/task', true],
  ['data/node-work/7b2cfdd1-f86d-4e11-aae8-8fe10cdbed06/af6fef03-ad84-409e-b397-494011dd49ff/task', true],
  ['data/_uidev/benchE2', false],
] as const;
for (const [dir, withLegend] of REAL) {
  const file = `${A}/${dir}/ui_structure.json`;
  if (!fs.existsSync(file)) continue;
  const structure = JSON.parse(fs.readFileSync(file, 'utf8'));
  const legend = withLegend ? JSON.parse(fs.readFileSync(`${A}/${dir}/all_elements_legend.json`, 'utf8')) : undefined;
  const assets: string[] = [];
  const walk = (n: any) => { if (typeof n?.asset === 'string' && !assets.includes(n.asset)) assets.push(n.asset); for (const c of n?.children ?? []) walk(c); };
  walk(structure.root);
  const elements: unknown[] = [], assetNames: [string, string][] = [];
  assets.forEach((a, i) => {
    const file = a.split('/').pop()!.toLowerCase(), stem = file.replace(/\.png$/, '');
    if (i % 5 === 4) return;
    const name = i % 3 === 1 ? `kit_${i}` : stem;
    if (i % 3 === 1) assetNames.push([file, name.toUpperCase()]);
    elements.push(el(name, kitm.guessElementType(stem)));
  });
  const realKit = JSON.stringify({ ...KIT, name: 'real', elements });
  S_CASES.push({ name: `real ${dir.split('/').slice(-2).join('/')}`, structure, legend, kit: realKit, assetNames, dpi: 460 });
}

out.structures = S_CASES.map((c) => {
  const k = c.kit ? kitm.parseKit(c.kit) : null;
  const r = fromStructure.fromUiStructure({ structure: c.structure, boxes: c.legend !== undefined ? fromStructure.legendBoxes(c.legend) : undefined, kit: k,
    assetNames: c.assetNames ? new Map(c.assetNames) : undefined, dpi: c.dpi });
  return { name: c.name, structure: c.structure ?? null, legend: c.legend ?? null, kit: c.kit ?? null, assetNames: c.assetNames ?? [], dpi: c.dpi ?? null,
    doc: JSON.stringify(r.doc), styles: r.styles.map((s: unknown) => JSON.stringify(s)), warnings: r.warnings, stats: r.stats };
});

const LEGENDS: unknown[] = [null, 5, {}, { targets: 5 }, LEGEND, { targets: [{ path: 'a', abs_bbox: [1, 2, 3, 4] }, { path: 'a', abs_bbox: [9, 9, 9.5, 9] }, { path: 'b', abs_bbox: ['1', null, '', [5]] }, { path: 'c', abs_bbox: [1, 2, 3, 'Infinity'] }, { path: 'd', abs_bbox: [true, false, '0x10', ' 7 '] }, [1]] }];
out.legends = LEGENDS.map((l) => ({ legend: l === undefined ? null : l, boxes: [...fromStructure.legendBoxes(l).entries()] }));
out.elementNodes = [...['background', 'panel', 'frame', 'button', 'icon', 'image', 'progressBar', 'progressFill', 'toggle', 'slider', 'sliderHandle', 'input', 'tab', 'badge', 'divider', 'header', 'slot', 'other'].map((t) => [t, fromStructure.ELEMENT_NODE[t]]), ['nope', '']];

// --- the markup's analysis -> a draft ------------------------------------------------------------------------------
const POPUP_ANSWER = {
  background: { kind: 'image', describe: 'night sky' },
  looks: { popup_panel: 'wooden popup panel', btn_green: '' },
  elements: [
    { id: 'dim', kind: 'panel', box_2d: [0, 0, 1000, 1000], color: '#000000', opacity: 0.6 },
    { id: 'popup', kind: 'panel', box_2d: [250, 80, 750, 920], look: 'popup_panel' },
    { id: 'title', kind: 'text', parent: 'popup', box_2d: [270, 200, 320, 800], text: 'VICTORY', color: '#FFE45C', weight: 'bold', stroke: '#5A2A00' },
    { id: 'close', kind: 'button', parent: 'popup', box_2d: [255, 850, 290, 910], kit: ' icon_close ' },
    { id: 'play', kind: 'button', parent: 'popup', box_2d: [640, 300, 700, 700], look: 'btn_green' },
    { id: 'play_label', kind: 'text', parent: 'play', box_2d: [650, 400, 690, 600], text: 'PLAY', color: '#ffffff', weight: 'bold' },
    { id: 'hp', kind: 'progress', parent: 'popup', box_2d: [560, 150, 590, 850], value: 0.35, kit: 'bar_track' },
    { id: 'hp_fill', kind: 'image', parent: 'hp', box_2d: [562, 152, 588, 400] },
    { id: 'hp_text', kind: 'text', parent: 'hp', box_2d: [562, 400, 588, 600], text: '35/100', color: '#ffffff' },
    { id: 'rewards', kind: 'grid', parent: 'popup', box_2d: [350, 150, 530, 850], rows: 1, cols: 3, cells: [{ look: 'slot', texts: ['x25', 'coins'], color: '#FFD700' }, { look: 'slot', texts: ['x3'] }, null] },
    { id: 'star', kind: 'image', parent: 'popup', box_2d: [300, 100, 340, 180], kit: 'sticker_star', rotation: -12 },
    { id: 'coin', kind: 'image', box_2d: [20, 30, 50, 80], kit: 'icon_coin' },
    { id: 'coins_text', kind: 'text', box_2d: [25, 90, 45, 250], text: '1,250', color: '#ffffff', align: 'left' },
    { id: 'Name Field', kind: 'input', box_2d: [800, 100, 850, 900], look: 'input_field' },
    { id: 'tabs', kind: 'group', box_2d: [900, 0, 1000, 1000] },
    { id: 'tab_a', kind: 'button', parent: 'tabs', box_2d: [910, 20, 990, 320], kit: 'tab_main' },
    { id: 'tab_a_t', kind: 'text', parent: 'tab_a', box_2d: [930, 50, 970, 290], text: 'Shop', color: '#333333' },
    { id: 'tab_b', kind: 'button', parent: 'tabs', box_2d: [910, 350, 990, 650], kit: 'badge_red' },
    { id: 'frame', kind: 'panel', parent: 'tabs', box_2d: [910, 680, 990, 980], kit: 'slot_wood' },
    { id: 'pic', kind: 'panel', box_2d: [60, 400, 120, 600], kit: 'icon_coin' },
    { id: 'deco', kind: 'image', box_2d: [60, 700, 120, 900], kit: 'bar_track' },
    { id: 'bgpic', kind: 'image', box_2d: [130, 700, 200, 900], kit: 'bg_menu' },
    { id: 'nokit', kind: 'image', box_2d: [130, 100, 200, 300], kit: 'does_not_exist' },
    { id: 'big_img', kind: 'image', box_2d: [130, 350, 230, 650] },
    { id: 'strip', kind: 'image', box_2d: [230, 0, 240, 1000], color: '#FF0000', opacity: '0.5' },
    { id: 'tilted', kind: 'image', box_2d: [600, 50, 700, 250], rotation: 30, look: 'gem' },
    { id: 'tilted2', kind: 'panel', box_2d: [600, 750, 700, 950], angle: 60, color: '#00FF00' },
    { id: 'wobble', kind: 'image', box_2d: [710, 50, 760, 150], rotation: 1.5 },
    { id: 'flip', kind: 'image', box_2d: [710, 200, 760, 300], rotation: 200 },
    { id: 'sticker', kind: 'image', box_2d: [710, 400, 790, 600], kit: 'Sticker_Star', rotation: 45 },
    { id: 'toggle', kind: 'panel', box_2d: [710, 700, 760, 800], kit: 'toggle_on' },
    { id: 'knob', kind: 'image', box_2d: [770, 700, 790, 720], kit: 'slider_knob' },
    { id: 'tinted', kind: 'panel', box_2d: [770, 750, 790, 800], look: 'tint', color: '#123456' },
    { id: 'clear', kind: 'panel', box_2d: [770, 820, 790, 900], color: '#abc', opacity: null },
    { id: 'more', kind: 'group', box_2d: [400, 900, 500, 1000], color: '#ff00ff' },
    { id: 'note', kind: 'text', parent: 'more', box_2d: [410, 910, 440, 990], text: 'Note', color: '#c0c0c0', opacity: 0.2 },
    { id: 'small', kind: 'text', box_2d: [960, 400, 980, 600], text: 'v1.0', color: 'nope' },
  ],
};
const PIXEL_ANSWER = '```json\n' + JSON.stringify({ background: { kind: 'color', color: '#203040' }, elements: [
  { id: 'Play', kind: 'button', box: [60, 60, 240, 120], look: 'Btn Green!' },
  { id: 'play', kind: 'button', box: [60, 150, 240, 210] },
  { kind: 'text', parent: 'play', box: [70, 155, 230, 205], text: 'Again', color: '#000' },
  { id: 'two', kind: 'button', box: [300, 60, 540, 120] },
  { id: 't1', kind: 'text', parent: 'two', box: [306, 66, 414, 114], text: 'Buy', color: '#fff' },
  { id: 't2', kind: 'text', parent: 'two', box: [420, 66, 534, 114], text: '99', color: '#fff', stroke: '#000000' },
  { id: 'outer', kind: 'group', box: [0, 240, 723, 840] },
  { id: 'mid', kind: 'panel', parent: 'outer', box: [30, 270, 693, 810], look: 'panel' },
  { id: 'inner', kind: 'group', parent: 'mid', box: [60, 300, 663, 780] },
  { id: 'deep', kind: 'image', parent: 'inner', box: [90, 330, 180, 420], look: 'gem' },
  { id: 'deep_t', kind: 'text', parent: 'inner', box: [90, 432, 600, 480], text: 'Collect gems', color: '#ffffff', weight: 'medium', align: 'right' },
  { id: 'bar', kind: 'progress', box: [60, 900, 660, 930] },
  { id: 'bar_img', kind: 'image', parent: 'bar', box: [60, 900, 300, 930] },
  { id: 'blank', kind: 'text', box: [10, 10, 50, 50], text: '   ' },
  { id: 'odd', kind: 'blob', box: [10, 10, 50, 50] },
  { id: 'faded', kind: 'panel', box: [6, 960, 180, 1020], look: 'panel', opacity: null, kit: '   ' },
  { id: 'kitted', kind: 'image', box: [240, 960, 300, 1020], kit: ' icon_coin ', opacity: 7 },
  { id: 'pale', kind: 'text', box: [320, 960, 600, 1000], text: 'almost white', color: '#FFFFC1' },
  { id: 'pale2', kind: 'text', box: [320, 1010, 600, 1050], text: 'a bit further', color: '#FFFFBA' },
] }) + '\n```';
const ARRAY_ANSWER = JSON.stringify([
  { id: 'full', kind: 'panel', box_2d: [0, 0, 1000, 1000], look: 'bg' },
  { id: 'left', kind: 'image', parent: 'full', box_2d: [100, 0, 200, 100] },
  { id: 'right', kind: 'image', parent: 'full', box_2d: [100, 900, 200, 1000] },
  { id: 'centre', kind: 'image', parent: 'full', box_2d: [450, 400, 550, 600] },
  { id: 'near_centre', kind: 'image', parent: 'full', box_2d: [440, 380, 540, 600] },
  { id: 'bottom', kind: 'panel', parent: 'full', box_2d: [900, 0, 1000, 1000], look: 'bar' },
  { id: 'bottom_t', kind: 'text', parent: 'bottom', box_2d: [920, 100, 980, 300], text: 'Score 12', color: '#ffff00' },
  { id: 'wide', kind: 'panel', parent: 'full', box_2d: [300, 30, 400, 960], look: 'bar' },
]);

const extraStyles = [
  { name: 'Hero', role: 'heading', font: 'Bangers', size: 60, color: '#FF4040', gradient: '#FF8800' },
  { name: 'small_grey', role: 'caption', size: 20, color: '#888888' },
];
const D_CASES = [
  { name: 'popup', answer: JSON.stringify(POPUP_ANSWER), w: 1080, h: 1920, model: 'gemini-2.5-flash', fontSizes: { title: 96, coins_text: 40, tab_a_t: 30, note: 24, rewards_r0c0_t0: 44, hp_text: 28 },
    options: { assetOf: [['popup_panel', 'panel_wood'], ['btn_green', 'btn_green'], ['gem', 'gem_cut'], ['slot', 'slot_wood'], ['input_field', 'input_field'], ['tint', 'frame_gold']],
      styleOf: [['title', 'title'], ['play_label', 'NoSuchStyle'], ['hp_text', 'BUTTON'], ['note', 'small_grey']], extra: extraStyles, dpi: 460, name: 'Reward Popup', background: { kit: 'bg_menu' } } },
  { name: 'pixels', answer: PIXEL_ANSWER, w: 1179, h: 2556, model: 'gpt-5', fontSizes: { t1: 40, t2: 40, deep_t: 30, pale: 20, pale2: 20 },
    options: { assetOf: [['btn_green', 'btn_green'], ['panel', 'panel_wood']], styleOf: [['deep_t', 'hero']], extra: extraStyles, name: '', background: { color: '#203040' } } },
  { name: 'array', answer: ARRAY_ANSWER, w: 800, h: 600, model: '', fontSizes: {},
    options: { assetOf: [['bar', 'bar_track']], styleOf: [], extra: [], dpi: 0, name: 'My Screen!', background: { color: 'nope' } } },
  { name: 'no kit', answer: JSON.stringify(POPUP_ANSWER), w: 1080, h: 1920, model: 'gemini', fontSizes: { title: 96 }, noKit: true,
    options: { assetOf: [], styleOf: [['title', 'title']], extra: [], name: 'x', background: {} } },
];
out.drafts = D_CASES.map((c) => {
  const frame = parse.analysisFrame(c.model, c.w, c.h);
  const an = parse.parseAnalysis(c.answer, c.w, c.h, frame);
  for (const e of an.els) { const f = (c.fontSizes as Record<string, number>)[e.id]; if (f !== undefined) e.fontSize = f; }
  const k = c.noKit ? null : kit;
  const styles = [...(k?.textStyles ?? []), ...c.options.extra.map((s) => ts.normalizeTextStyle(s, 'x'))];
  const r = draft.draftFromAnalysis(an, { kit: k, assetOf: new Map(c.options.assetOf as [string, string][]), styleOf: new Map(c.options.styleOf as [string, string][]), styles,
    dpi: (c.options as { dpi?: number }).dpi, name: c.options.name, background: c.options.background });
  return { name: c.name, answer: c.answer, w: c.w, h: c.h, model: c.model, fontSizes: c.fontSizes, noKit: !!c.noKit,
    assetOf: c.options.assetOf, styleOf: c.options.styleOf, extra: c.options.extra, dpi: (c.options as { dpi?: number }).dpi ?? null, docName: c.options.name,
    backgroundKit: (c.options.background as { kit?: string }).kit ?? '', backgroundColor: (c.options.background as { color?: string }).color ?? '',
    analysis: JSON.parse(JSON.stringify(an)), doc: JSON.stringify(r.doc), styles: r.styles.map((s: unknown) => JSON.stringify(s)), boxes: [...r.boxes.entries()], blocks: r.blocks };
});

const R = (x: number, y: number, w: number, h: number) => ({ x, y, w, h });
const P = R(0, 0, 1000, 600);
out.anchors = [
  [R(0, 0, 1000, 600), P], [R(10, 10, 980, 580), P], [R(0, 0, 100, 100), P], [R(900, 500, 100, 100), P], [R(450, 250, 100, 100), P],
  [R(445, 260, 100, 100), P], [R(440, 240, 120, 100), P], [R(0, 250, 1000, 100), P], [R(100, 0, 100, 600), P], [R(30, 30, 920, 560), P],
  [R(30, 30, 880, 540), P], [R(500, 300, 0, 0), P], [R(-50, -50, 1100, 700), P], [R(110, 110, 30, 30), R(100, 100, 50, 50)], [R(10, 10, 1, 1), R(0, 0, 1, 1)],
  [R(12.4, 7.6, 975.2, 584.8), P], [R(0, 0, 10, 10), R(0, 0, 0, 0)],
  [R(0, 0, 965, 600), P], [R(0, 0, 960, 600), P], [R(0, 0, 959.9, 600), P], [R(0, 0, 1000, 576), P], [R(0, 0, 1000, 575), P], [R(0, 0, 899, 600), P],
  [R(400, 0, 200, 100), P], [R(380, 0, 200, 100), P], [R(388, 0, 200, 100), P], [R(387.9, 0, 200, 100), P], [R(0, 230, 100, 100), P], [R(0, 238, 100, 100), P],
  [R(0, 241, 100, 100), P], [R(990, 590, 10, 10), P], [R(5, 595, 10, 5), P], [R(1, 1, 3, 3), R(0, 0, 4, 4)],
].map(([r, p]) => [r, p, draft.anchorFor(r, p)]);

fs.writeFileSync(process.argv[2], JSON.stringify(out));
console.log('ok', Object.keys(out).length, JSON.stringify(out).length, 'structures', S_CASES.length, 'drafts', D_CASES.length);
