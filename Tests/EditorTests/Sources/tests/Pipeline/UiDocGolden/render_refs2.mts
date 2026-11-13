// Reference renders, round 2: text styles with a gradient fill and layer effects (shared/textStyle.ts, render.ts textSvg)
// on the shared sample screen (shared/uiDoc/sample.ts), and a kit=none plain block; drawn by headless Chrome with the UI
// fonts loaded by @font-face, as the web card draws them.
//   ASSETSLINE=$AL $AL/backend/node_modules/.bin/tsx render_refs2.mts /tmp/refs2/refs2.json
//   python3 gen_tests.py /tmp/refs2/refs2.json ../UiDocRenderRefs2.inc kUiDocRenderRefs2Parts
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

const STYLES = [
  { name: 'gold', font: 'Lilita One', size: 96, color: '#FFF3A0',
    fill: { angle: 90, stops: [{ color: '#FFF3A0', at: 0 }, { color: '#FFD84A', at: 0.48 }, { color: '#F0A020', at: 0.52 }, { color: '#C06000', at: 1 }] },
    effects: [{ type: 'stroke', color: '#5A2A00', width: 6 }, { type: 'extrude', color: '#3A1800', x: 0, y: 8 }, { type: 'shadow', color: '#000000', x: 0, y: 6, blur: 10, opacity: 0.5 }] },
  { name: 'double', font: 'Russo One', size: 80, color: '#FFFFFF',
    effects: [{ type: 'stroke', color: '#FFFFFF', width: 3 }, { type: 'stroke', color: '#1A3A8A', width: 7 }, { type: 'glow', color: '#40A0FF', size: 14, opacity: 0.8 }] },
  { name: 'emboss', font: 'Fira Sans', weight: 900, size: 90, color: '#7FD34A',
    effects: [{ type: 'bevel', highlight: '#FFFFFF', shade: '#000000', depth: 4, soft: 2, opacity: 0.6 }, { type: 'innerShadow', color: '#003300', x: 0, y: -3, blur: 3, opacity: 0.5 }, { type: 'stroke', color: '#1E4A10', width: 4 }] },
  { name: 'sideways', font: 'Bangers', size: 88, color: '#FF4060', fill: { angle: 0, stops: ['#FF4060', '#FFD040'] }, effects: [{ type: 'shadow', color: '#200010', x: 5, y: 5, blur: 0, opacity: 1 }] },
];
const styles = STYLES.map((s) => ts.normalizeTextStyle(s, s.name));
const TEXT = 'Victory 3!';
const fontCss = ts.fontFaceCss((f: string) => `file://${A}/backend/src/uiLayout/fonts/${f}`);
const chrome = '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome';
const out: any = { text: TEXT, cases: [] };
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
  const pw = doc.width, ph = doc.height;
  const svg = render.renderLayoutSvg(doc, { mode: 'kit', kit, pixelSize: { w: pw, h: ph } });
  out.cases.push({ name: st.name, style: JSON.stringify(st), doc: JSON.stringify(doc), png: await shoot(st.name, svg, pw, ph) });
  console.log(st.name, pw, ph);
}
{
  const src = 'screen Plain 600x400\n  panel dim anchor=stretch kit=none color=#000000 opacity=0.6\n  button ok anchor=center size=300x100 kit=none color=#3060C0 "OK"\n  panel empty anchor=top-left pos=20,20 size=100x60 kit=none';
  const doc = uiml.parseUiml(src).doc;
  const kit = { version: 1, name: 'k', elements: [], textStyles: [ts.normalizeTextStyle({ name: 'button', role: 'button', font: 'Roboto', size: 40, color: '#FFFFFF' })] };
  const svg = render.renderLayoutSvg(doc, { mode: 'kit', kit, pixelSize: { w: 600, h: 400 } });
  out.plain = { src, kit: JSON.stringify(kit), png: await shoot('plain', svg, 600, 400) };
}
fs.writeFileSync(OUT, JSON.stringify(out));
