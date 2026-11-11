// The web's kit merge (frontend/src/nodeEditor/uiLayout/UiKitBody.tsx at 77d5b09), verbatim, over test inputs.
type Item = Record<string, any>;
function merge(cur: Record<string, Item>, items: Record<string, Item>) {
  const next = { ...cur };
  for (const [pid, it] of Object.entries(items)) {
    const { background: _b, ...clean } = it as Item & { background?: string };
    const hand = !!cur[pid]?.nineSlice && cur[pid]?.nineFrom !== 'ai';
    const nine = hand ? { nineSlice: cur[pid]!.nineSlice, nineFrom: undefined } : { nineSlice: clean.nineSlice ?? null, nineFrom: clean.nineSlice ? clean.nineFrom : undefined };
    const keepAi = cur[pid]?.bg === 'ai' && !!cur[pid]?.prepared && !clean.prepared;
    next[pid] = { ...cur[pid], ...clean, ...nine, ...(keepAi ? { bg: 'ai' as const, prepared: cur[pid]!.prepared } : {}) };
  }
  return next;
}
const cur = {
  a: { name: 'old', nineSlice: { left: 7, top: 7, right: 7, bottom: 7 } },
  b: { bg: 'ai', prepared: { uploadId: 'u.png', from: 'h' } },
  c: { nineSlice: { left: 1, top: 1, right: 1, bottom: 1 }, nineFrom: 'ai' },
  e: { nineSlice: { left: 1, top: 1, right: 1, bottom: 1 }, nineFrom: 'ai', name: 'e' },
  f: { name: 'f', nineFrom: 'ai', nineSlice: { left: 4, top: 4, right: 4, bottom: 4 } },
  g: { name: 'g', nineSlice: null, nineFrom: 'x' },
};
const items = {
  a: { name: 'new', bg: 'keep', ai: true, background: 'transparent', nineSlice: { left: 2, top: 2, right: 2, bottom: 2 }, nineFrom: 'ai' },
  b: { name: 'b', bg: 'keep', ai: true, background: 'complex' },
  c: { name: 'c', bg: 'key', ai: true, background: 'flat' },
  d: { name: 'd', bg: 'keep', ai: true, background: 'transparent', nineSlice: { left: 3, top: 3, right: 3, bottom: 3 }, nineFrom: 'ai' },
  e: { name: 'e2', nineSlice: { left: 5, top: 5, right: 5, bottom: 5 } },
  f: { name: 'f2', nineSlice: null, nineFrom: 'ai' },
  g: { name: 'g2', nineSlice: { left: 6, top: 6, right: 6, bottom: 6 }, nineFrom: 'ai', prepared: null, bg: 'ai' },
};
console.log(JSON.stringify({ cur, items, merged: JSON.parse(JSON.stringify(merge(cur, items))) }));
