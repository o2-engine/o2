// Golden references for the o2 port of the AI UI layout's Download (AssetsLine shared/uiExport/unity.ts, the JSON branch
// of frontend/src/nodeEditor/uiLayout/uiLayoutState.ts downloadLayout, shared/uiExport/o2.ts for the seeded uids). It
// reads the structure cases of golden.json (cases.mts) and imports the web's TypeScript as is (read-only):
//   AL=<AssetsLine checkout>; ASSETSLINE=$AL $AL/backend/node_modules/.bin/tsx export_refs.mts /tmp/golden.json /tmp/export.json
//   python3 gen_tests.py /tmp/export.json ../UiDocExportRefs.inc kUiDocExportRefParts
import fs from 'node:fs';
import zlib from 'node:zlib';
import { createRequire } from 'node:module';
const A = process.env.ASSETSLINE ?? '/Users/andreizenkovich/work/zenkovich.space/AssetsLine';
const sharp = createRequire(`${A}/backend/package.json`)('sharp');
const uiml = await import(`${A}/shared/uiDoc/uiml.ts`);
const structure = await import(`${A}/shared/uiDoc/structure.ts`);
const device = await import(`${A}/shared/uiDoc/device.ts`);
const kitm = await import(`${A}/shared/uiKit.ts`);
const ts = await import(`${A}/shared/textStyle.ts`);
const pngMeta = await import(`${A}/shared/pngMeta.ts`);
const unity = await import(`${A}/shared/uiExport/unity.ts`);
const o2 = await import(`${A}/shared/uiExport/o2.ts`);

const golden = JSON.parse(fs.readFileSync(process.argv[2], 'utf8'));
const out: Record<string, unknown> = {};
const enc = (s: string) => new TextEncoder().encode(s);
const dec = new TextDecoder();
const b64 = (u: Uint8Array) => Buffer.from(u).toString('base64');
const isText = (path: string) => !/\.(png|ttf|otf)$/i.test(path);
const fileOut = (f: { path: string; data: Uint8Array }) => [f.path, isText(f.path) ? dec.decode(f.data) : b64(f.data)];

// The kits of the structure cases, as cases.mts made them
const png = Buffer.from(golden.structure.png, 'base64');
const kitA = JSON.parse(golden.structure.kitA);
const KIT = kitm.parseKit(golden.kit.json);
for (const e of KIT.elements) e.png = golden.structure.png;
const kitOf = (k: string | null) => (k === null ? null : k === 'A' ? kitA : k === 'KIT' ? KIT : JSON.parse(k));

// The Unity export's fonts as downloadLayout picks them; their bytes stand in as text
const ROBOTO = ['Roboto-Regular.ttf', 'Roboto-Medium.ttf', 'Roboto-Bold.ttf', 'LICENSE-Roboto.txt'];
function unityFonts(st: any): string[] {
  const used = st.fonts.filter((f: string) => !ROBOTO.includes(f));
  const licenses = ts.UI_FONTS.filter((f: any) => f.family !== 'Roboto' && f.faces.some((x: any) => used.includes(x.file))).map((f: any) => f.license);
  return [...ROBOTO, ...used, ...licenses];
}
const fakeFont = (file: string) => ({ file, data: enc('FONT ' + file) });

// --- Unity: the structure cases ---------------------------------------------------------------------------------------
out.unity = [];
for (const c of golden.structure.cases.filter((x: any) => !['struct_nokit', 'nobg_layout'].includes(x.name))) {
  const doc = uiml.parseUiml(c.src).doc;
  const st = structure.toUiStructure(doc, kitOf(c.kit));
  const fonts = unityFonts(st);
  const folder = c.name === 'dpi_kit' ? ' /My/Folder/ ' : '';
  const input = { name: doc.name, folder: folder.trim().replace(/^\/+|\/+$/g, '') || 'Generated/UI', layout: st.json, sprites: st.sprites, fonts: fonts.map(fakeFont), seed: 'node_' + c.name };
  let entry: Record<string, unknown>;
  try {
    const res = await unity.exportUnity(input);
    entry = { files: res.files.map(fileOut), warnings: res.warnings };
    if (c.name === 'screen_kitA') entry.package = tarEntries(zlib.gunzipSync(await unity.unityPackage(res.files)));
  } catch (e: any) {
    entry = { error: String(e?.message ?? e) };
  }
  (out.unity as unknown[]).push({ name: c.name, assetPath: folder, fonts, ...entry });
}

function tarEntries(tar: Uint8Array): unknown[] {
  const entries: unknown[] = [];
  for (let o = 0; o + 512 <= tar.length;) {
    const header = tar.subarray(o, o + 512);
    if (header.every((b) => b === 0)) break;
    const name = dec.decode(header.subarray(0, 100)).replace(/\0.*$/s, '');
    const size = parseInt(dec.decode(header.subarray(124, 135)), 8);
    const data = tar.subarray(o + 512, o + 512 + size);
    entries.push([name, b64(header), size, name.endsWith('/asset') && !name.startsWith('packagemanager') ? null : dec.decode(data)]);
    o += 512 + Math.ceil(size / 512) * 512;
  }
  return entries;
}

// --- Unity: structures written by hand (states, scroll, hue, progress labels, layouts the layout node does not write) --
{
  const pal = b64(await sharp(Buffer.from([255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 128, 250, 200, 10, 255, 30, 30, 30, 0, 120, 60, 200, 255]),
    { raw: { width: 3, height: 2, channels: 4 } }).png().toBuffer());
  const frame = b64(await sharp({ create: { width: 64, height: 48, channels: 4, background: '#336699' } }).png().toBuffer());
  const node = (o: Record<string, unknown>) => o;
  const handLayout = {
    canvas: { name: 'Hand', width: 800, height: 600, safeArea: { left: 10, top: 20, right: 30, bottom: 40 } },
    root: node({ name: 'Hand', type: 'container', position: { x: 0, y: 0 }, size: { width: 800, height: 600 }, children: [
      node({ name: 'bg', type: 'image', asset: 'pal.png', position: { x: 0, y: 0 }, size: { width: 800, height: 600 }, hueShift: 90,
        responsive: { min: { x: 0, y: 0 }, max: { x: 1, y: 1 }, offsetMin: { x: 0, y: 0 }, offsetMax: { x: 0, y: 0 }, safeArea: true } }),
      node({ name: 'tint', type: 'overlay', color: '#00000080', opacity: 0.5, position: { x: 0, y: 0 }, size: { width: 800, height: 600 } }),
      node({ name: 'panel', type: 'image', asset: 'frame.png', nineSlice: true, position: { x: 100, y: 50 }, size: { width: 300, height: 200 }, anchor: { horizontal: 'center', vertical: 'middle' }, children: [
        node({ name: 'title', type: 'text', text: 'Héllo: "world" #1', fontFamily: 'fonts/Roboto-Bold.ttf', fontSize: 31.5, color: 'yellow', alignment: 'center', textVAlign: 'middle', lineHeight: 40, strokeColor: '#102030', strokeWidth: 2.5, textScaleX: 0.8, position: { x: 10, y: 10 }, size: { width: 280, height: 40 } }),
        node({ name: 'note', type: 'text', text: '- a note', fontFamily: 'Unknown.ttf', lineHeight: 20, position: { x: 10, y: 60 }, size: { width: 280, height: 30 }, align: 'center', vAlign: 'bottom' }),
        node({ name: 'ok', type: 'button', asset: 'frame.png', nineSlice: { left: 8, top: 6, right: 8, bottom: 6 }, position: { x: 20, y: 120 }, size: { width: 120, height: 50 }, children: [
          node({ name: 'ok_label', type: 'text', text: 'OK', position: { x: 0, y: 0 }, size: { width: 120, height: 50 } }),
        ] }),
        node({ name: 'cancel', type: 'button', position: { x: 160, y: 120 }, size: { width: 120, height: 50 } }),
      ] }),
      node({ name: 'bar', type: 'container', position: { x: 450, y: 50 }, size: { width: 300, height: 30 },
        progress: { min: 0, max: 200, value: 50, fill: 'bar_fill', direction: 'right-to-left', label: 'bar_label', format: '{value:.1f}/{max} ({percent:,.2f}%) {{x}}' }, children: [
          node({ name: 'bar_track', type: 'rect', color: '#222', position: { x: 0, y: 0 }, size: { width: 300, height: 30 } }),
          node({ name: 'bar_fill', type: 'image', asset: 'frame.png', nineSlice: 4, position: { x: 0, y: 0 }, size: { width: 300, height: 30 }, children: [
            node({ name: 'shine', type: 'rect', color: '#fff', opacity: 0.2, position: { x: 0, y: 0 }, size: { width: 300, height: 10 } }),
          ] }),
          node({ name: 'bar_label', type: 'text', text: '', position: { x: 0, y: 0 }, size: { width: 300, height: 30 } }),
      ] }),
      node({ name: 'empty_bar', type: 'container', position: { x: 450, y: 100 }, size: { width: 300, height: 30 },
        progress: { min: 0, max: 1, value: 0, fill: 'empty_fill', direction: 'bottom-to-top' }, children: [
          node({ name: 'empty_fill', type: 'rect', color: 'lime', position: { x: 0, y: 0 }, size: { width: 300, height: 30 } }),
      ] }),
      node({ name: 'list', type: 'container', position: { x: 450, y: 150 }, size: { width: 300, height: 200 }, scroll: { content: 'list_content', direction: 'vertical', offset: { x: 0, y: 500 } }, children: [
        node({ name: 'list_content', type: 'container', position: { x: 0, y: 0 }, size: { width: 300, height: 400 },
          layout: { type: 'column', spacing: 'even', padding: { x: 4, y: 6 } }, children: [
            node({ name: 'i1', type: 'rect', color: '#a00', size: { width: 200, height: 50 } }),
            node({ name: 'i2', type: 'rect', color: '#0a0', size: { width: 250, height: 50 }, offset: { x: 5, y: 3 }, align: 'end' }),
            node({ name: 'i3', type: 'rect', color: '#00a', size: { width: 100, height: 50 }, visible: false }),
        ] }),
      ] }),
      node({ name: 'row', type: 'container', position: { x: 20, y: 400 }, size: { width: 500, height: 80 }, color: '#333333',
        layout: { type: 'row', align: 'space-around', vAlign: 'center', padding: 10 }, children: [
          node({ name: 'r1', type: 'image', asset: 'pal.png', size: { width: 60, height: 40 } }),
          node({ name: 'r2', type: 'text', text: 'two', size: { width: 100, height: 40 }, vAlign: 'top' }),
          node({ name: 'r3', type: 'image', asset: 'pal.png', size: { width: 60, height: 60 } }),
      ] }),
      node({ name: 'grid', type: 'container', position: { x: 20, y: 500 }, size: { width: 400, height: 90 },
        layout: { type: 'grid', columns: 3, cellSize: { width: 60, height: 40 }, spacing: { x: 5, y: 6 }, align: 'center', vAlign: 'end' }, children: [
          node({ name: 'g1', type: 'rect', color: '#fff', size: { width: 60, height: 40 } }),
          node({ name: 'g2', type: 'rect', color: '#fff', size: { width: 60, height: 40 }, offset: { x: 2, y: 2 } }),
          node({ name: 'g3', type: 'rect', color: '#fff', size: { width: 60, height: 40 } }),
          node({ name: 'g4', type: 'rect', color: '#fff', size: { width: 60, height: 40 } }),
      ] }),
      node({ name: 'toggle', type: 'container', position: { x: 600, y: 400 }, size: { width: 120, height: 60 },
        state: { current: 'on', variants: { on: { knob: { position: { x: 60, y: 0 }, color: '#0f0' } }, off: { '.': { opacity: 0.5 }, knob: { color: '#f00' } }, 'a/b': {} } }, children: [
          node({ name: 'knob', type: 'rect', color: '#888', position: { x: 0, y: 0 }, size: { width: 60, height: 60 } }),
      ] }),
      node({ name: 'hidden', type: 'rect', color: '#123456', position: { x: 0, y: 0 }, size: { width: 10, height: 10 }, visible: false, vAlign: 'middle' }),
    ] }),
  };
  const sprites = [
    { file: 'pal.png', b64: pal, width: 3, height: 2 },
    { file: 'frame.png', b64: frame, width: 64, height: 48, nineSlice: { left: 10, top: 12, right: 10, bottom: 12 }, density: 2, dpi: 960 },
  ];
  const cases: { name: string; layout: string; sprites: any[]; fonts: string[]; seed: string; folder: string }[] = [
    { name: 'hand', layout: JSON.stringify(handLayout), sprites, fonts: [...ROBOTO, 'Lobster-Regular.ttf'], seed: 'seed-1', folder: 'UI\\..\\Screens/./x' },
    { name: 'hand_nofonts', layout: JSON.stringify(handLayout), sprites, fonts: [], seed: '', folder: '' },
    // The layout's asset paths may start with ./ (uiAssetRefs drops it), the sprite lookup does not: the web fails there
    { name: 'dot_asset', layout: JSON.stringify(handLayout).replace('"asset":"frame.png","nineSlice":true', '"asset":"./frame.png","nineSlice":true'), sprites, fonts: [], seed: 's', folder: '' },
    { name: 'missing_sprite', layout: JSON.stringify(handLayout), sprites: sprites.slice(0, 1), fonts: [], seed: 's', folder: '' },
    { name: 'bad_color', layout: JSON.stringify({ canvas: { width: 10, height: 10 }, root: { name: 'R', type: 'rect', color: 'nope', size: { width: 10, height: 10 } } }), sprites: [], fonts: [], seed: 's', folder: '' },
    { name: 'bad_size', layout: JSON.stringify({ canvas: { width: 10, height: 10 }, root: { name: 'R', type: 'container', size: { width: 10, height: 10 }, children: [{ name: 'c', type: 'rect' }] } }), sprites: [], fonts: [], seed: 's', folder: '' },
  ];
  out.unityHand = [];
  for (const c of cases) {
    const input = { name: c.name === 'hand' ? 'Hand Screen!' : '', folder: c.folder, layout: c.layout, seed: c.seed, fonts: c.fonts.map(fakeFont),
      sprites: c.sprites.map((s) => ({ file: s.file, png: Buffer.from(s.b64, 'base64'), width: s.width, height: s.height, nineSlice: s.nineSlice, density: s.density, dpi: s.dpi })) };
    let entry: Record<string, unknown>;
    try {
      const res = await unity.exportUnity(input);
      entry = { files: res.files.map(fileOut), warnings: res.warnings,
        pixels: Object.fromEntries(await Promise.all(res.files.filter((f: any) => /hue_.*\.png$/.test(f.path)).map(async (f: any) =>
          [f.path, b64(await sharp(Buffer.from(f.data)).ensureAlpha().raw().toBuffer())]))) };
    } catch (e: any) {
      entry = { error: String(e?.message ?? e) };
    }
    (out.unityHand as unknown[]).push({ ...c, inputName: input.name, ...entry });
  }
  out.floats = [0, -0, 1, 0.1, 1 / 3, 1e-7, 123456.789, 3.4e38, -2.5e-5, 0.5019608, 1e15, 16777217, 1e21].map((x) => [x, unity.fmt(x)]);
  out.guids = ['', 'a', 'folder|Assets/Generated'].map((k) => [k, unity.unityGuid(k)]);
}

// --- JSON: the zip's entries as downloadLayout writes them --------------------------------------------------------------
out.json = [];
for (const c of golden.structure.cases.filter((x: any) => ['screen_kitA', 'dpi_kit', 'struct_kit', 'screen_nokit'].includes(x.name))) {
  const doc = uiml.parseUiml(c.src).doc;
  const st = structure.toUiStructure(doc, kitOf(c.kit));
  const dir = doc.name;
  const entries: [string, Uint8Array][] = [
    [`${dir}/ui_structure.json`, enc(st.json)],
    [`${dir}/layout.uiml`, enc(uiml.docToUiml(doc))],
    [`${dir}/layout.json`, enc(JSON.stringify(doc, null, 2))],
  ];
  for (const s of st.sprites) entries.push([`${dir}/sprites/${s.file}`, pngMeta.withPngDpi(s.png, s.dpi ?? device.docDpi(doc))]);
  const manifest = {
    layoutDpi: device.docDpi(doc),
    sprites: st.sprites.map((s: any) => ({ file: s.file, width: s.width, height: s.height, dpi: s.dpi ?? device.docDpi(doc), pixelsPerLayoutPixel: s.density ?? 1, ...(s.nineSlice ? { nineSlice: s.nineSlice } : {}) })),
  };
  entries.push([`${dir}/kit.json`, enc(JSON.stringify(manifest, null, 2))]);
  const fontFiles = new Set<string>(st.fonts);
  const licenses = new Set(ts.UI_FONTS.filter((f: any) => f.faces.some((x: any) => fontFiles.has(x.file))).map((f: any) => f.license));
  for (const f of [...fontFiles, ...licenses]) entries.push([`${dir}/fonts/${f}`, new Uint8Array(fs.readFileSync(`${A}/backend/src/uiLayout/fonts/${f}`))]);
  entries.push([`${dir}/preview.png`, enc('preview-bytes')]);
  (out.json as unknown[]).push({ name: c.name, entries: entries.map(([n, d]) => [n, /\/(sprites|fonts)\//.test(n) ? (/\.png$/.test(n) ? b64(d) : d.length) : dec.decode(d)]) });
}

// --- o2: the uids the web's o2 export derives for the files it writes ---------------------------------------------------
{
  const c = golden.structure.cases.find((x: any) => x.name === 'screen_kitA');
  const doc = uiml.parseUiml(c.src).doc;
  const st = structure.toUiStructure(doc, kitOf(c.kit));
  const res = await o2.exportO2({ name: doc.name, folder: 'Generated/UI', layout: st.json, sprites: st.sprites, seed: 'node_o2', fonts: ROBOTO.map(fakeFont) });
  out.o2 = { files: res.files.filter((f: any) => f.path.endsWith('.meta')).map((f: any) => [f.path, JSON.parse(dec.decode(f.data)).Value?.mId ?? null]), warnings: res.warnings };
}

fs.writeFileSync(process.argv[3], JSON.stringify(out));
console.log('ok', Object.keys(out).length, JSON.stringify(out).length);
