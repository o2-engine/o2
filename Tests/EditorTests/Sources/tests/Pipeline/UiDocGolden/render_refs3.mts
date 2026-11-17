// Reference renders, round 3 (AssetsLine ea79bbc): the effects as a layer stack - a shadow or a depth above an outline -
// and gradient outlines, on one line (the shared sample screen) and wrapped over two (a narrow block on a screen of its
// own), drawn by headless Chrome with the UI fonts loaded by @font-face, as the web card draws them.
//   ASSETSLINE=$AL $AL/backend/node_modules/.bin/tsx render_refs3.mts /tmp/refs3/refs3.json
//   python3 gen_tests.py /tmp/refs3/refs3.json ../UiDocRenderRefs3.inc kUiDocRenderRefs3Parts
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
const ts = await import(`${A}/shared/textStyle.ts`);
const sample = await import(`${A}/shared/uiDoc/sample.ts`);

const RAINBOW = { angle: 90, stops: [{ color: '#FFE14A', at: 0 }, { color: '#FF6A00', at: 0.5 }, { color: '#C0007A', at: 1 }] };
const STYLES = [
  // A hard shadow above the outline falls on it
  { name: 'shadow_over', font: 'Russo One', size: 88, color: '#FFFFFF', fxStack: true,
    effects: [{ type: 'shadow', color: '#FF2050', x: 7, y: 7, blur: 0, opacity: 1 }, { type: 'stroke', color: '#1A3A8A', width: 8 }] },
  // The depth above the outline covers it
  { name: 'depth_over', font: 'Lilita One', size: 96, color: '#FFD84A', fxStack: true,
    effects: [{ type: 'extrude', color: '#7A2E00', x: 0, y: 9 }, { type: 'stroke', color: '#2A1000', width: 6 }, { type: 'shadow', color: '#000000', x: 0, y: 6, blur: 10, opacity: 0.6 }] },
  // A gradient outline around a flat one, a soft glow under both
  { name: 'grad_outline', font: 'Fira Sans', weight: 900, size: 90, color: '#FFFFFF',
    effects: [{ type: 'stroke', color: '#202020', width: 3 }, { type: 'stroke', width: 9, fill: RAINBOW }, { type: 'glow', color: '#40A0FF', size: 12, opacity: 0.7 }] },
  // A sideways gradient outline, faded, over a gradient fill
  { name: 'grad_side', font: 'Bangers', size: 92, color: '#FFFFFF', fill: { angle: 90, stops: ['#FFFFFF', '#9AD0FF'] },
    effects: [{ type: 'stroke', width: 7, opacity: 0.6, fill: { angle: 0, stops: [{ color: '#00C0FF', at: 0 }, { color: '#8000FF', at: 1 }] } }] },
];
const styles = STYLES.map((s) => ts.normalizeTextStyle(s, s.name));
const TEXT = 'Victory 3!';
const fontCss = ts.fontFaceCss((f: string) => `file://${A}/backend/src/uiLayout/fonts/${f}`);
const chrome = '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome';
const out: any = { text: TEXT, cases: [], wrapped: [] };
const shoot = async (name: string, svg: string, pw: number, ph: number) => {
  const html = `<!doctype html><html><head><style>${fontCss}\nhtml,body{margin:0;padding:0;background:transparent;overflow:hidden}svg{display:block}</style></head><body>${svg}</body></html>`;
  const page = path.join(WORK, `${name}.html`), shot = path.join(WORK, `${name}.png`);
  fs.writeFileSync(page, html);
  execFileSync(chrome, ['--headless=new', '--disable-gpu', '--no-first-run', `--user-data-dir=${WORK}/profile`, '--force-device-scale-factor=1',
    '--hide-scrollbars', '--default-background-color=00000000', `--window-size=${pw},${ph}`, `--screenshot=${shot}`, '--virtual-time-budget=5000', `file://${page}`], { stdio: 'ignore' });
  const png = await sharp(shot).extract({ left: 0, top: 0, width: pw, height: ph }).png().toBuffer();
  fs.writeFileSync(shot, png);
  return png.toString('base64');
};
for (const st of styles) {
  const doc = sample.sampleDoc(st, TEXT, '#20232B');
  const kit = { version: 1, name: 'k', elements: [], textStyles: [st] };
  const svg = render.renderLayoutSvg(doc, { mode: 'kit', kit, pixelSize: { w: doc.width, h: doc.height } });
  out.cases.push({ name: st.name, style: JSON.stringify(st), doc: JSON.stringify(doc), png: await shoot(st.name, svg, doc.width, doc.height) });
  console.log(st.name, doc.width, doc.height);
}
// Two lines: each line has its own gradient over its own reach
for (const st of styles.slice(2)) {
  const src = `screen Wrapped 640x480 color=#20232B\n  text t anchor=center size=380x400 style=${st.name} "Victory Royale"`;
  const doc = uiml.parseUiml(src).doc;
  const kit = { version: 1, name: 'k', elements: [], textStyles: [st] };
  const svg = render.renderLayoutSvg(doc, { mode: 'kit', kit, pixelSize: { w: 640, h: 480 } });
  out.wrapped.push({ name: st.name, src, kit: JSON.stringify(kit), png: await shoot(`wrapped_${st.name}`, svg, 640, 480) });
}
fs.writeFileSync(OUT, JSON.stringify(out));
