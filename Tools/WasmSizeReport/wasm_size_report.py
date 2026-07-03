#!/usr/bin/env python3
"""Code size report for a WebAssembly binary.

Parses the .wasm sections directly, sizes every function body from the code section,
names them through the Emscripten symbol map (--emit-symbol-map), demangles C++ names
and groups them into a namespace/class/function tree. The result is a single
self-contained interactive HTML page (treemap + expandable tree + search).

Usage:
    python3 wasm_size_report.py Bin/WebAssembly/PetStory.wasm
        [--symbols path/to/PetStory.html.symbols]  # default: auto-discover near the wasm
        [--demangler path/to/llvm-cxxfilt]         # default: from $EMSDK or ~/emsdk
        [-o report.html]                           # default: <wasm>.size-report.html
"""

import argparse
import html
import json
import os
import re
import subprocess
import sys

# ---------------------------------------------------------------- wasm parsing

SECTION_NAMES = {
    0: "custom", 1: "type", 2: "import", 3: "function", 4: "table", 5: "memory",
    6: "global", 7: "export", 8: "start", 9: "elem", 10: "code", 11: "data", 12: "datacount",
}


class Reader:
    def __init__(self, data, pos=0):
        self.data = data
        self.pos = pos

    def byte(self):
        b = self.data[self.pos]
        self.pos += 1
        return b

    def uleb(self):
        result = 0
        shift = 0
        while True:
            b = self.byte()
            result |= (b & 0x7F) << shift
            if not b & 0x80:
                return result
            shift += 7

    def bytes(self, n):
        chunk = self.data[self.pos:self.pos + n]
        self.pos += n
        return chunk

    def name(self):
        return self.bytes(self.uleb()).decode("utf-8", "replace")


def uleb_len(value):
    n = 1
    while value >= 0x80:
        value >>= 7
        n += 1
    return n


def parse_wasm(path):
    """Returns (sections, functions): section sizes and per-function body sizes."""
    data = open(path, "rb").read()
    if data[:4] != b"\0asm":
        sys.exit(f"{path}: not a wasm binary")

    reader = Reader(data, 8)
    sections = []   # {name, size}
    functions = []  # {index, size}
    num_func_imports = 0

    while reader.pos < len(data):
        section_id = reader.byte()
        payload_size = reader.uleb()
        payload_start = reader.pos
        name = SECTION_NAMES.get(section_id, f"id{section_id}")

        if section_id == 0:
            sub = Reader(data, payload_start)
            name = f"custom:{sub.name()}"
        elif section_id == 2:  # import: count function imports to offset code indices
            sub = Reader(data, payload_start)
            for _ in range(sub.uleb()):
                sub.name()
                sub.name()
                kind = sub.byte()
                if kind == 0:
                    sub.uleb()
                    num_func_imports += 1
                elif kind == 1:
                    sub.byte()
                    flags = sub.uleb()
                    sub.uleb()
                    if flags & 1:
                        sub.uleb()
                elif kind == 2:
                    flags = sub.uleb()
                    sub.uleb()
                    if flags & 1:
                        sub.uleb()
                elif kind == 3:
                    sub.byte()
                    sub.byte()
        elif section_id == 10:  # code: per-function body sizes
            sub = Reader(data, payload_start)
            count = sub.uleb()
            for i in range(count):
                body_size = sub.uleb()
                functions.append({"index": num_func_imports + i,
                                  "size": body_size + uleb_len(body_size)})
                sub.pos += body_size

        sections.append({"name": name, "size": payload_size + uleb_len(payload_size) + 1})
        reader.pos = payload_start + payload_size

    return sections, functions


# ------------------------------------------------------------------- symbols

def load_symbols(path):
    symbols = {}
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or ":" not in line:
                continue
            idx, name = line.split(":", 1)
            symbols[int(idx)] = name
    return symbols


def find_demangler(explicit):
    candidates = [explicit] if explicit else []
    emsdk = os.environ.get("EMSDK", os.path.expanduser("~/emsdk"))
    candidates += [os.path.join(emsdk, "upstream/bin/llvm-cxxfilt"), "llvm-cxxfilt", "c++filt"]
    for c in candidates:
        if c and (os.path.exists(c) or not os.path.sep in c):
            try:
                subprocess.run([c, "_ZN2o26RenderD2Ev"], capture_output=True, check=True)
                return c
            except Exception:
                continue
    return None


def demangle_all(names, demangler):
    if not demangler:
        return dict(zip(names, names))
    out = subprocess.run([demangler], input="\n".join(names).encode(),
                         capture_output=True, check=True).stdout.decode("utf-8", "replace")
    return dict(zip(names, out.splitlines()))


# ------------------------------------------------------- name -> group path

# C libraries and runtime glue don't carry namespaces; map them by prefix
PREFIX_GROUPS = [
    (re.compile(r"^(FT_|TT_|T1_|ft_|tt_|t1_|cff_|cf2_|psh_|ps_|af_|sfnt_|gray_|pcf_|bdf_|woff|PS_)"), "FreeType"),
    (re.compile(r"^png_"), "libpng"),
    (re.compile(r"^(jerry_?|ecma_|vm_|lit_|parser_|lexer_|scanner_|opfunc_|jcontext_|jmem_|re_)"), "JerryScript"),
    (re.compile(r"^(inflate|deflate|adler32|crc32|zcalloc|zcfree|compress|uncompress|inftree|_tr_|fill_window|read_buf|updatewindow|fixedtables)"), "zlib"),
    (re.compile(r"^(dlmalloc|dlfree|dlrealloc|dlcalloc|dlposix|dlmemalign|internal_malloc|malloc|free|calloc|realloc)$"), "malloc"),
    (re.compile(r"^(invoke_|dynCall|legalstub|legalfunc|emscripten_|_emscripten|sbrk|stackSave|stackRestore|stackAlloc|setThrew|saveSetjmp|testSetjmp|getTempRet|setTempRet|fflush|__wasm|__cxa|__stdio|__towrite|__toread|__fwritex|__lockfile|__unlockfile|__ofl|__emscripten)"), "runtime/libc"),
    (re.compile(r"^(memcpy|memset|memmove|memcmp|strlen|strcpy|strcmp|strncmp|strchr|strstr|snprintf|vsnprintf|sprintf|sscanf|printf|puts|qsort|atoi|atof|strtod|strtol|pow|exp|log|sin|cos|tan|fmod|floor|ceil|sqrt|round|abs)"), "runtime/libc"),
    (re.compile(r"^b2"), "Box2D"),
    (re.compile(r"^stbi_|^stb_"), "stb"),
]


def split_qualified(name):
    """Splits a demangled C++ name into path segments at top-level '::'."""
    segments = []
    current = []
    depth_angle = depth_paren = 0
    i = 0
    while i < len(name):
        ch = name[i]
        if ch == "<":
            depth_angle += 1
        elif ch == ">":
            depth_angle = max(0, depth_angle - 1)
        elif ch == "(":
            depth_paren += 1
        elif ch == ")":
            depth_paren = max(0, depth_paren - 1)
        elif ch == ":" and depth_angle == 0 and depth_paren == 0 and name[i:i + 2] == "::":
            segments.append("".join(current))
            current = []
            i += 2
            continue
        current.append(ch)
        i += 1
    segments.append("".join(current))
    return segments


def strip_signature(demangled):
    """Removes the trailing argument list and any return type, keeping the qualified path."""
    s = demangled.strip()
    # cut the trailing (...) group with everything after it (const, &, ...)
    depth = 0
    cut = -1
    for i in range(len(s) - 1, -1, -1):
        ch = s[i]
        if ch == ")":
            depth += 1
        elif ch == "(":
            depth -= 1
            if depth == 0:
                cut = i
                break
    if cut > 0:
        # keep operator()() intact: don't cut if this paren belongs to "operator()"
        head = s[:cut]
        if head.endswith("operator"):
            pass
        else:
            s = head
    # drop the return type: last space-separated token at zero angle/paren depth
    depth_angle = depth_paren = 0
    last_space = -1
    for i, ch in enumerate(s):
        if ch == "<":
            depth_angle += 1
        elif ch == ">":
            depth_angle = max(0, depth_angle - 1)
        elif ch == "(":
            depth_paren += 1
        elif ch == ")":
            depth_paren = max(0, depth_paren - 1)
        elif ch == " " and depth_angle == 0 and depth_paren == 0:
            last_space = i
    if last_space >= 0 and not s[:last_space].endswith(("operator", "operator new", "operator delete")):
        tail = s[last_space + 1:]
        if "::" in tail or not tail.startswith(("const", "&", "*")):
            s = tail
    return s


def group_path(mangled, demangled):
    # Emscripten symbol maps usually carry already-demangled names, so don't require
    # the demangler to have changed anything — just look for C++ name structure
    if "::" in demangled or "(" in demangled:
        qualified = strip_signature(demangled)
        segments = [seg for seg in split_qualified(qualified) if seg]
        if len(segments) > 1:
            top = segments[0]
            if top.startswith("std"):
                return ["std"] + segments[1:]
            if top.startswith("b2") or top == "Box2D":
                return ["Box2D"] + segments
            if top == "spine":
                return ["spine"] + segments[1:]
            return segments
        qualified = segments[0] if segments else demangled
        for pattern, group in PREFIX_GROUPS:
            if pattern.search(qualified):
                return [group, qualified]
        return ["<global>", qualified]

    for pattern, group in PREFIX_GROUPS:
        if pattern.search(mangled):
            return [group, mangled]
    return ["<global>", mangled]


# --------------------------------------------------------------- tree build

def build_tree(functions, symbols, demangled_map):
    root = {"name": "code", "size": 0, "count": 0, "children": {}}
    for func in functions:
        mangled = symbols.get(func["index"], f"func[{func['index']}]")
        demangled = demangled_map.get(mangled, mangled)
        path = group_path(mangled, demangled)

        node = root
        node["size"] += func["size"]
        node["count"] += 1
        for segment in path:
            child = node["children"].setdefault(
                segment, {"name": segment, "size": 0, "count": 0, "children": {}})
            child["size"] += func["size"]
            child["count"] += 1
            node = child
    return root


def compact(node, min_size):
    """Converts children dicts to sorted lists, folding tiny leaves into '(other)'."""
    children = sorted(node["children"].values(), key=lambda c: -c["size"])
    out = []
    other_size = other_count = 0
    for child in children:
        if child["size"] < min_size and len(out) >= 30:
            other_size += child["size"]
            other_count += child["count"]
        else:
            out.append(compact(child, min_size))
    if other_size:
        out.append({"name": "(other)", "size": other_size, "count": other_count, "children": []})
    return {"name": node["name"], "size": node["size"], "count": node["count"], "children": out}


# ----------------------------------------------------------------- HTML GUI

HTML_TEMPLATE = """<!DOCTYPE html>
<html><head><meta charset="utf-8"><title>Wasm size: %TITLE%</title>
<style>
  :root { --bg:#14161c; --panel:#1d2027; --text:#e8eaf0; --dim:#8a90a0; --accent:#5aa9e6; }
  * { box-sizing: border-box; margin: 0; }
  body { background: var(--bg); color: var(--text); font: 13px/1.5 -apple-system, "Segoe UI", sans-serif; padding: 16px; }
  h1 { font-size: 17px; margin-bottom: 4px; }
  .summary { color: var(--dim); margin-bottom: 12px; }
  .sections { display: flex; gap: 8px; flex-wrap: wrap; margin-bottom: 14px; }
  .chip { background: var(--panel); border-radius: 6px; padding: 5px 10px; font-size: 12px; }
  .chip b { color: var(--accent); }
  #crumbs { margin: 8px 0; color: var(--dim); min-height: 20px; }
  #crumbs span { color: var(--accent); cursor: pointer; }
  #crumbs span:hover { text-decoration: underline; }
  #treemap { width: 100%; height: 380px; background: var(--panel); border-radius: 8px; display:block; cursor: pointer; }
  #tip { position: fixed; pointer-events: none; background: #000d; padding: 6px 9px; border-radius: 5px;
         font-size: 12px; display: none; z-index: 10; max-width: 480px; }
  #search { width: 100%; margin: 14px 0 8px; padding: 8px 10px; border-radius: 6px; border: 1px solid #333;
            background: var(--panel); color: var(--text); font-size: 13px; }
  table { width: 100%; border-collapse: collapse; }
  th, td { text-align: left; padding: 3px 8px; }
  th { color: var(--dim); font-weight: 500; border-bottom: 1px solid #333; position: sticky; top: 0; background: var(--bg); }
  td.num, th.num { text-align: right; font-variant-numeric: tabular-nums; white-space: nowrap; }
  tr.row:hover { background: #ffffff10; }
  .toggle { cursor: pointer; user-select: none; color: var(--accent); display: inline-block; width: 14px; }
  .bar { height: 4px; background: var(--accent); border-radius: 2px; min-width: 1px; }
  .name { word-break: break-all; }
  .count { color: var(--dim); }
</style></head><body>
<h1>%TITLE%</h1>
<div class="summary">%SUMMARY%</div>
<div class="sections">%SECTIONS%</div>
<div id="crumbs"></div>
<canvas id="treemap"></canvas>
<div id="tip"></div>
<input id="search" placeholder="Фильтр по имени (подстрока)..." />
<table><thead><tr><th style="width:55%">Имя</th><th class="num">Размер</th><th class="num">%</th>
<th class="num">Функций</th><th style="width:18%"></th></tr></thead><tbody id="rows"></tbody></table>
<script>
const DATA = %DATA%;
const TOTAL = DATA.size;
const fmt = n => n >= 1048576 ? (n/1048576).toFixed(2)+" MB" : n >= 1024 ? (n/1024).toFixed(1)+" KB" : n+" B";
const pct = n => (100*n/TOTAL).toFixed(2)+"%";

// ---- treemap (slice-and-dice by depth, simple and readable) ----
const canvas = document.getElementById("treemap"), tip = document.getElementById("tip");
const palette = ["#5aa9e6","#e6a65a","#7ed67e","#e65a7e","#a67ee6","#5ae6d0","#e6dd5a","#e67e5a","#8a9ce6","#6ec6a0"];
let zoomNode = DATA, zoomPath = [], rects = [];

// squarified treemap: rows of near-square tiles, greedy worst-aspect minimization
function squarify(items, x, y, w, h, out) {
  items = items.filter(i => i.size > 0);
  if (!items.length || w <= 1 || h <= 1) return;
  const total = items.reduce((s, i) => s + i.size, 0);
  const area = w * h;
  let row = [], rowSum = 0, rest = items.slice(), restSum = total;
  const worst = (sum, side) => {
    let max = 0;
    for (const i of row) {
      const tile = (i.size / total) * area, len = sum / total * area / side, breadth = tile / len;
      max = Math.max(max, len / breadth, breadth / len);
    }
    return max;
  };
  while (rest.length) {
    const side = Math.min(w, h);
    const item = rest[0];
    row.push(item);
    if (row.length > 1 && worst(rowSum + item.size, side) > worst(rowSum, side)) {
      row.pop();
      const len = (rowSum / total) * area / side;
      let offset = 0;
      for (const i of row) {
        const breadth = ((i.size / total) * area) / len;
        if (w <= h) out.push({item: i, x: x + offset, y, w: breadth, h: len});
        else out.push({item: i, x, y: y + offset, w: len, h: breadth});
        offset += breadth;
      }
      if (w <= h) { y += len; h -= len; } else { x += len; w -= len; }
      const placed = row.reduce((s, i) => s + i.size, 0);
      restSum -= placed;
      row = []; rowSum = 0;
      continue;
    }
    rowSum += item.size;
    rest.shift();
  }
  if (row.length) {
    const side = Math.min(w, h);
    const len = side > 0 ? Math.max(w, h) : 0;
    let offset = 0;
    for (const i of row) {
      const frac = i.size / rowSum;
      if (w <= h) out.push({item: i, x: x + offset, y, w: frac * w, h}); else out.push({item: i, x, y: y + offset, w, h: frac * h});
      offset += frac * (w <= h ? w : h);
    }
  }
}

function layout(node, x, y, w, h, horizontal, depth, out) {
  const kids = node.children || [];
  if (depth > 0) { out.push({node, x, y, w, h, depth}); if (!kids.length || depth >= 2 || w<46 || h<30) return; }
  const inner = depth > 0 ? 15 : 0;
  const ix = x+2, iy = y+2+inner, iw = w-4, ih = h-4-inner;
  if (iw <= 0 || ih <= 0) return;
  const tiles = [];
  squarify(kids.map(k => ({size: k.size, node: k})), ix, iy, iw, ih, tiles);
  for (const t of tiles)
    layout(t.item.node, t.x, t.y, t.w, t.h, !horizontal, depth+1, out);
}
function draw() {
  const dpr = devicePixelRatio || 1;
  canvas.width = canvas.clientWidth*dpr; canvas.height = 380*dpr;
  const g = canvas.getContext("2d"); g.scale(dpr,dpr);
  g.clearRect(0,0,canvas.clientWidth,380);
  rects = [];
  layout(zoomNode, 0, 0, canvas.clientWidth, 380, true, 0, rects);
  for (const r of rects) {
    if (r.depth === 0) continue;
    if (r.w < 1.5 || r.h < 1.5) continue;
    const top = topAncestorIndex(r.node);
    g.fillStyle = palette[top % palette.length] + (r.depth === 1 ? "55" : "2e");
    g.fillRect(r.x, r.y, r.w, r.h);
    g.strokeStyle = "#0006"; g.strokeRect(r.x+.5, r.y+.5, r.w-1, r.h-1);
    if (r.w > 46 && r.h > 15) {
      g.fillStyle = "#e8eaf0"; g.font = (r.depth===1 ? "12px" : "11px")+" sans-serif";
      const label = r.node.name+" "+fmt(r.node.size);
      g.save(); g.beginPath(); g.rect(r.x,r.y,r.w,r.h); g.clip();
      g.fillText(label, r.x+4, r.y+13); g.restore();
    }
  }
}
const topIndexCache = new Map();
function topAncestorIndex(node) {
  if (topIndexCache.has(node)) return topIndexCache.get(node);
  let idx = 0;
  (zoomNode.children||[]).forEach((kid, i) => { if (contains(kid, node)) idx = i; });
  topIndexCache.set(node, idx); return idx;
}
function contains(a, b) { if (a === b) return true; return (a.children||[]).some(k => contains(k, b)); }
function hit(e) {
  const box = canvas.getBoundingClientRect();
  const x = e.clientX-box.left, y = e.clientY-box.top;
  let best = null;
  for (const r of rects) if (r.depth>0 && x>=r.x && x<=r.x+r.w && y>=r.y && y<=r.y+r.h)
    if (!best || r.depth > best.depth) best = r;
  return best;
}
canvas.onmousemove = e => {
  const r = hit(e);
  if (r) { tip.style.display="block"; tip.style.left=(e.clientX+14)+"px"; tip.style.top=(e.clientY+14)+"px";
    tip.textContent = r.node.name+" — "+fmt(r.node.size)+" ("+pct(r.node.size)+", "+r.node.count+" функц.)"; }
  else tip.style.display = "none";
};
canvas.onmouseleave = () => tip.style.display = "none";
canvas.onclick = e => {
  const r = hit(e);
  if (!r) return;
  let target = r.node;
  for (const kid of (zoomNode.children||[])) if (contains(kid, r.node)) { target = kid; break; }
  if (target.children && target.children.length) { zoomPath.push(zoomNode); zoomNode = target; topIndexCache.clear(); crumbs(); draw(); }
};
function crumbs() {
  const el = document.getElementById("crumbs");
  el.innerHTML = "";
  const chain = [...zoomPath, zoomNode];
  chain.forEach((node, i) => {
    const s = document.createElement("span");
    s.textContent = node.name;
    s.onclick = () => { zoomPath = chain.slice(0, i); zoomNode = node; topIndexCache.clear(); crumbs(); draw(); };
    el.appendChild(s);
    if (i < chain.length-1) el.appendChild(document.createTextNode("  ›  "));
  });
  el.appendChild(document.createTextNode("   (клик — вглубь, по хлебным крошкам — назад)"));
}

// ---- tree table ----
const rowsEl = document.getElementById("rows");
let expanded = new Set([DATA]);
function renderTable(filter) {
  rowsEl.innerHTML = "";
  const frag = document.createDocumentFragment();
  function matches(node) {
    if (!filter) return true;
    if (node.name.toLowerCase().includes(filter)) return true;
    return (node.children||[]).some(matches);
  }
  function walk(node, depth) {
    if (filter && !matches(node)) return;
    const tr = document.createElement("tr"); tr.className = "row";
    const kids = (node.children||[]).filter(k => !filter || matches(k));
    const open = expanded.has(node) || !!filter;
    const td = document.createElement("td");
    td.style.paddingLeft = (depth*18+8)+"px";
    const tog = document.createElement("span"); tog.className = "toggle";
    tog.textContent = kids.length ? (open ? "▾" : "▸") : "·";
    tog.onclick = () => { if (!kids.length) return; open && !filter ? expanded.delete(node) : expanded.add(node); renderTable(filter); };
    td.appendChild(tog);
    const nm = document.createElement("span"); nm.className = "name"; nm.textContent = " "+node.name;
    td.appendChild(nm); tr.appendChild(td);
    const sizeTd = document.createElement("td"); sizeTd.className="num"; sizeTd.textContent = fmt(node.size); tr.appendChild(sizeTd);
    const pctTd = document.createElement("td"); pctTd.className="num"; pctTd.textContent = pct(node.size); tr.appendChild(pctTd);
    const cntTd = document.createElement("td"); cntTd.className="num count"; cntTd.textContent = node.count; tr.appendChild(cntTd);
    const barTd = document.createElement("td");
    const bar = document.createElement("div"); bar.className = "bar"; bar.style.width = (100*node.size/TOTAL)+"%";
    barTd.appendChild(bar); tr.appendChild(barTd);
    frag.appendChild(tr);
    if (open) for (const kid of kids) walk(kid, depth+1);
  }
  walk(DATA, 0);
  rowsEl.appendChild(frag);
}
document.getElementById("search").oninput = e => renderTable(e.target.value.trim().toLowerCase());
addEventListener("resize", draw);
crumbs(); draw(); renderTable("");
</script></body></html>
"""


def make_html(wasm_path, sections, tree, out_path):
    total_file = os.path.getsize(wasm_path)
    code_size = tree["size"]
    section_chips = "".join(
        f'<div class="chip">{html.escape(s["name"])}: <b>{s["size"]/1048576:.2f} MB</b></div>'
        if s["size"] >= 1048576 else
        f'<div class="chip">{html.escape(s["name"])}: <b>{s["size"]/1024:.0f} KB</b></div>'
        for s in sorted(sections, key=lambda s: -s["size"]) if s["size"] >= 1024)
    summary = (f"Файл: {html.escape(os.path.basename(wasm_path))} — "
               f"{total_file/1048576:.2f} MB, из них код функций {code_size/1048576:.2f} MB "
               f"({tree['count']} функций). Доли ниже считаются от размера кода.")
    page = (HTML_TEMPLATE
            .replace("%TITLE%", html.escape(os.path.basename(wasm_path)))
            .replace("%SUMMARY%", summary)
            .replace("%SECTIONS%", section_chips)
            .replace("%DATA%", json.dumps(tree, separators=(",", ":"))))
    with open(out_path, "w") as f:
        f.write(page)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("wasm")
    ap.add_argument("--symbols")
    ap.add_argument("--demangler")
    ap.add_argument("-o", "--output")
    ap.add_argument("--min-leaf", type=int, default=256,
                    help="fold leaves smaller than this into '(other)' (bytes)")
    args = ap.parse_args()

    symbols_path = args.symbols
    if not symbols_path:
        base = re.sub(r"\.wasm$", "", args.wasm)
        for candidate in (base + ".html.symbols", base + ".js.symbols", base + ".symbols"):
            if os.path.exists(candidate):
                symbols_path = candidate
                break
    if not symbols_path or not os.path.exists(symbols_path):
        sys.exit("Symbol map not found: build with --emit-symbol-map or pass --symbols")

    sections, functions = parse_wasm(args.wasm)
    symbols = load_symbols(symbols_path)

    demangler = find_demangler(args.demangler)
    if not demangler:
        print("warning: no demangler found, names stay mangled", file=sys.stderr)
    mangled_names = sorted({symbols.get(f["index"], "") for f in functions} - {""})
    demangled_map = demangle_all(mangled_names, demangler)

    tree = compact(build_tree(functions, symbols, demangled_map), args.min_leaf)

    out_path = args.output or re.sub(r"\.wasm$", "", args.wasm) + ".size-report.html"
    make_html(args.wasm, sections, tree, out_path)

    total = tree["size"]
    print(f"code total: {total/1048576:.2f} MB in {tree['count']} functions")
    for child in tree["children"][:15]:
        print(f"  {child['size']/1048576: 7.2f} MB {100*child['size']/total: 5.1f}%  {child['name']} ({child['count']})")
    print(f"report: {out_path}")


if __name__ == "__main__":
    main()
