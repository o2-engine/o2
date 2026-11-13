// Golden references for the o2 port of the AI UI layout's Save to project: the uids AssetsLine's o2 export
// (shared/uiExport/o2.ts) derives for the files it writes. It reads the structure cases of golden.json (cases.mts) and imports
// the web's TypeScript as is (read-only):
//   AL=<AssetsLine checkout>; ASSETSLINE=$AL $AL/backend/node_modules/.bin/tsx export_refs.mts /tmp/golden.json /tmp/export.json
//   python3 gen_tests.py /tmp/export.json ../UiDocExportRefs.inc kUiDocExportRefParts
import fs from 'node:fs';
const A = process.env.ASSETSLINE ?? '/Users/andreizenkovich/work/zenkovich.space/AssetsLine';
const uiml = await import(`${A}/shared/uiDoc/uiml.ts`);
const structure = await import(`${A}/shared/uiDoc/structure.ts`);
const kitm = await import(`${A}/shared/uiKit.ts`);
const o2 = await import(`${A}/shared/uiExport/o2.ts`);

const golden = JSON.parse(fs.readFileSync(process.argv[2], 'utf8'));
const out: Record<string, unknown> = {};
const enc = (s: string) => new TextEncoder().encode(s);
const dec = new TextDecoder();

// The kits of the structure cases, as cases.mts made them
const kitA = JSON.parse(golden.structure.kitA);
const KIT = kitm.parseKit(golden.kit.json);
for (const e of KIT.elements) e.png = golden.structure.png;
const kitOf = (k: string | null) => (k === null ? null : k === 'A' ? kitA : k === 'KIT' ? KIT : JSON.parse(k));

// The fonts stand in as text
const ROBOTO = ['Roboto-Regular.ttf', 'Roboto-Medium.ttf', 'Roboto-Bold.ttf', 'LICENSE-Roboto.txt'];
const fakeFont = (file: string) => ({ file, data: enc('FONT ' + file) });

{
  const c = golden.structure.cases.find((x: any) => x.name === 'screen_kitA');
  const doc = uiml.parseUiml(c.src).doc;
  const st = structure.toUiStructure(doc, kitOf(c.kit));
  const res = await o2.exportO2({ name: doc.name, folder: 'Generated/UI', layout: st.json, sprites: st.sprites, seed: 'node_o2', fonts: ROBOTO.map(fakeFont) });
  out.o2 = { files: res.files.filter((f: any) => f.path.endsWith('.meta')).map((f: any) => [f.path, JSON.parse(dec.decode(f.data)).Value?.mId ?? null]), warnings: res.warnings };
}

fs.writeFileSync(process.argv[3], JSON.stringify(out));
console.log('ok', Object.keys(out).length, JSON.stringify(out).length);
