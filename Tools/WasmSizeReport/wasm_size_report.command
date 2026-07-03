#!/bin/bash
# Builds the wasm size report and opens it in the browser (macOS/Linux).
# Double-clickable from Finder; an optional argument overrides the .wasm path.
# Without an argument it looks for Bin/WebAssembly/*.wasm in the o2 root and,
# when o2 is a submodule, in the parent project root.
set -e
TOOL_DIR="$(cd "$(dirname "$0")" && pwd)"
O2_ROOT="$TOOL_DIR/../.."

WASM="$1"
if [ -z "$WASM" ]; then
    for root in "$O2_ROOT" "$O2_ROOT/.."; do
        candidate=$(ls "$root"/Bin/WebAssembly/*.wasm 2> /dev/null | head -1)
        if [ -n "$candidate" ]; then
            WASM="$candidate"
            break
        fi
    done
fi

if [ -z "$WASM" ] || [ ! -f "$WASM" ]; then
    echo "No .wasm found (looked for Bin/WebAssembly/*.wasm near o2)."
    echo "Build the wasm target first or pass a path: $0 path/to/App.wasm"
    read -n 1 -s -r -p "Press any key to close..."
    exit 1
fi

REPORT="${WASM%.wasm}.size-report.html"
python3 "$TOOL_DIR/wasm_size_report.py" "$WASM" -o "$REPORT"

if command -v open > /dev/null; then open "$REPORT"; else xdg-open "$REPORT"; fi
