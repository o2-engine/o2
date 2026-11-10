// Reference renders of the web's layout renderer for the o2 port: renderLayoutSvg's SVG drawn by headless Chrome with the
// UI fonts loaded by @font-face (as the web card draws it). Writes refs.json: the kit, and per case the inputs and the PNG;
// gen_tests.py embeds it as ../UiDocRenderRefs.inc (array kUiDocRenderRefParts). Needs Google Chrome and golden.json
// (cases.mts's output):
//   ASSETSLINE=$AL $AL/backend/node_modules/.bin/tsx render_refs.mts /tmp/refs/refs.json /tmp/golden.json
//   python3 gen_tests.py /tmp/refs/refs.json ../UiDocRenderRefs.inc kUiDocRenderRefParts
import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { createRequire } from 'node:module';
const A = process.env.ASSETSLINE ?? '/Users/andreizenkovich/work/zenkovich.space/AssetsLine';
const OUT = process.argv[2];
const WORK = path.dirname(OUT);
const sharp = createRequire(`${A}/backend/package.json`)('sharp');
const uiml = await import(`${A}/shared/uiDoc/uiml.ts`);
const render = await import(`${A}/shared/uiDoc/render.ts`);
const kitm = await import(`${A}/shared/uiKit.ts`);
const ts = await import(`${A}/shared/textStyle.ts`);
const prep = await import(`${A}/backend/src/uiKit/prep.ts`);
const model = await import(`${A}/shared/uiDoc/model.ts`);

const svgPng = async (svg: string) => (await sharp(Buffer.from(svg)).png().toBuffer()).toString('base64');
const green = await sharp(Buffer.from('<svg xmlns="http://www.w3.org/2000/svg" width="360" height="200"><rect width="360" height="200" fill="#00C040"/><rect x="40" y="40" width="280" height="120" rx="24" fill="#D83A3A" stroke="#401010" stroke-width="6"/></svg>')).png().toBuffer();
const btn = await prep.prepareElement({ data: green, assetType: 'sliced', key: true, trim: true, optimize: true, nineSlice: null });
const elements = [
  { name: 'btn_red', type: 'button', assetType: 'sliced', width: btn.width, height: btn.height, nineSlice: btn.nineSlice, png: btn.png.toString('base64') },
  { name: 'panel_wood', type: 'panel', assetType: 'sliced', width: 160, height: 120, nineSlice: { left: 32, top: 32, right: 32, bottom: 32 },
    png: await svgPng('<svg xmlns="http://www.w3.org/2000/svg" width="160" height="120"><rect x="4" y="4" width="152" height="112" rx="20" fill="#8B5A2B" stroke="#4A2A10" stroke-width="8"/><rect x="20" y="20" width="120" height="80" rx="8" fill="#A0703C"/></svg>') },
  { name: 'icon_coin', type: 'icon', assetType: 'simple', width: 64, height: 64,
    png: await svgPng('<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64"><circle cx="32" cy="32" r="28" fill="#F2C230" stroke="#8A6000" stroke-width="6"/><circle cx="32" cy="32" r="14" fill="#FFE27A"/></svg>') },
  { name: 'bg_forest', type: 'background', assetType: 'simple', width: 108, height: 192,
    png: await svgPng('<svg xmlns="http://www.w3.org/2000/svg" width="108" height="192"><defs><linearGradient id="g" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#2E7D32"/><stop offset="1" stop-color="#1B3A1C"/></linearGradient></defs><rect width="108" height="192" fill="url(#g)"/><circle cx="20" cy="150" r="30" fill="#3F9A45"/><circle cx="90" cy="170" r="26" fill="#357F3A"/></svg>') },
  { name: 'progress_track', type: 'progressBar', assetType: 'sliced', width: 200, height: 32, nineSlice: { left: 16, top: 16, right: 16, bottom: 15 },
    png: await svgPng('<svg xmlns="http://www.w3.org/2000/svg" width="200" height="32"><rect x="1.5" y="1.5" width="197" height="29" rx="14.5" fill="#333A44" stroke="#111111" stroke-width="3"/></svg>') },
  { name: 'progress_fill', type: 'progressFill', assetType: 'sliced', width: 200, height: 32, nineSlice: { left: 16, top: 16, right: 16, bottom: 15 },
    png: await svgPng('<svg xmlns="http://www.w3.org/2000/svg" width="200" height="32"><rect x="3" y="3" width="194" height="26" rx="13" fill="#59C84A"/></svg>') },
  { name: 'tile_grass', type: 'slot', assetType: 'tiled', width: 32, height: 32,
    png: await svgPng('<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32"><rect width="32" height="32" fill="#4C8F3A"/><rect width="16" height="16" fill="#5FA548"/><rect x="16" y="16" width="16" height="16" fill="#5FA548"/></svg>') },
  { name: 'icon_gear', type: 'icon', assetType: 'simple', width: 96, height: 96, dpi: 960,
    png: await svgPng('<svg xmlns="http://www.w3.org/2000/svg" width="96" height="96"><polygon points="48,4 60,36 92,36 66,56 76,90 48,70 20,90 30,56 4,36 36,36" fill="#C0C6D0" stroke="#5A606C" stroke-width="5"/></svg>') },
];
const textStyles = [
  { name: 'title', role: 'title', font: 'Russo One', size: 96, color: '#FFE066', gradient: '#FF8C00', stroke: { color: '#5A2A00', width: 6 }, shadow: { color: '#000000', x: 0, y: 6, blur: 8, opacity: 0.5 }, uppercase: true },
  { name: 'button', role: 'button', font: 'Lilita One', size: 48, color: '#FFFFFF', stroke: { color: '#6A1010', width: 4 } },
  { name: 'number', role: 'number', font: 'Fira Sans', weight: 700, size: 40, color: '#FFE066', letterSpacing: 2 },
  { name: 'body', role: 'body', font: 'Roboto', size: 32, color: '#FFFFFF' },
];
const KIT_JSON = JSON.stringify({ version: 1, name: 'ref', elements, textStyles });
const kit = kitm.parseKit(KIT_JSON);

const GAME = `screen Game 1080x1920 kit=bg_forest
  text title anchor=top-stretch pos=0,120 h=160 margin=0,60 style=title "Forest Quest"
  panel card anchor=center size=800x900 kit=panel_wood
    column menu anchor=stretch margin=60 gap=30
      button play h=150 kit=btn_red "PLAY"
      button shop h=150 kit=btn_red "SHOP" color=#80C0FF
      progress hp h=60 value=0.35 "35%"
      row coins h=90 gap=20 justify=center align=center
        icon coin_icon size=80x80
        text amount w=260 h=80 style=number "1 250"
      row tiles h=160 gap=10
        slot t1 grow=1 kit=tile_grass
        slot t2 grow=1 kit=tile_grass opacity=0.5
  icon gear_icon anchor=top-right pos=40,40 size=96x96
  button missing anchor=bottom-center pos=0,80 size=500x120 kit=nope "MISSING"
  input name anchor=bottom-stretch pos=0,260 h=100 margin=0,140 placeholder="Your name"
`;
const goldenPath = process.argv[3] ?? path.join(WORK, 'golden.json');
const golden = JSON.parse(fs.readFileSync(goldenPath, 'utf8'));
const SCREEN = golden.uiml[0].src, TRICKY = golden.uiml[3].src;
const sampleStyle = kit.textStyles[0];
const sampleText = 'Forest Quest';

// The Text style card's sample doc (frontend TextStyleBody.tsx sampleDoc)
function sampleDoc(style: any, text: string, dark: boolean) {
  const stroke = style.stroke ? style.stroke.width * 2 : 0;
  const w = Math.max(680, Math.ceil(ts.estimateTextWidth(text, style) * 1.03 + 36 + stroke * 2 + 8));
  const d = model.emptyDoc('Sample', w, 224);
  d.root = { id: 'root', type: 'screen', color: dark ? '#20232B' : '#F3F4F7', children: [{ id: 'sample', type: 'text', anchor: 'stretch', margin: [12, 18, 12, 18], style: style.name, text }] };
  return d;
}

const cases = [
  { name: 'lines', src: SCREEN, mode: 'lines', kit: false, long: 480 },
  { name: 'game_kit', src: GAME, mode: 'kit', kit: true, long: 600 },
  { name: 'game_lines_wide', src: GAME, mode: 'lines', kit: false, long: 480, width: 1920, height: 1080 },
  { name: 'tricky_ids', src: TRICKY, mode: 'lines', kit: false, long: 480, showIds: true },
  { name: 'screen_kit', src: SCREEN, mode: 'kit', kit: true, long: 480 },
  { name: 'sample', src: null, mode: 'kit', kit: false, long: 680 },
];

const fontCss = ts.fontFaceCss((f: string) => `file://${A}/backend/src/uiLayout/fonts/${f}`);
const chrome = '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome';
const out: any = { kit: KIT_JSON, sampleText, cases: [] };
for (const c of cases) {
  const doc = c.src === null ? sampleDoc(sampleStyle, sampleText, true) : uiml.parseUiml(c.src).doc;
  const useKit = c.src === null ? { version: 1, name: 'k', elements: [], textStyles: [sampleStyle] } : c.kit ? kit : null;
  const W = c.width ?? doc.width, H = c.height ?? doc.height;
  const pw = W >= H ? c.long : Math.max(1, Math.round(W * c.long / H));
  const ph = W >= H ? Math.max(1, Math.round(H * c.long / W)) : c.long;
  const svg = render.renderLayoutSvg(doc, { mode: c.mode, kit: useKit, pixelSize: { w: pw, h: ph }, showIds: !!c.showIds, ...(c.width ? { size: { w: c.width, h: c.height } } : {}) });
  const html = `<!doctype html><html><head><style>${fontCss}\nhtml,body{margin:0;padding:0;background:transparent;overflow:hidden}svg{display:block}</style></head><body>${svg}</body></html>`;
  const page = path.join(WORK, `${c.name}.html`), shot = path.join(WORK, `${c.name}.png`);
  fs.writeFileSync(page, html);
  execFileSync(chrome, ['--headless=new', '--disable-gpu', '--no-first-run', `--user-data-dir=${WORK}/profile`, '--force-device-scale-factor=1',
    '--hide-scrollbars', '--default-background-color=00000000', `--window-size=${pw},${ph}`, `--screenshot=${shot}`, '--virtual-time-budget=5000', `file://${page}`], { stdio: 'ignore' });
  const png = await sharp(shot).extract({ left: 0, top: 0, width: pw, height: ph }).png().toBuffer();
  fs.writeFileSync(shot, png);
  out.cases.push({ ...c, src: c.src ?? '', png: png.toString('base64') });
  console.log(c.name, pw, ph, png.length);
}
fs.writeFileSync(OUT, JSON.stringify(out));
