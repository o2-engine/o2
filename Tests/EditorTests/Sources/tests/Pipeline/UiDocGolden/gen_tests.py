import sys, json
# Embeds golden.json (the web's outputs) into the o2 tests as chunks of a C++ string array
src = open(sys.argv[1], 'rb').read()
# Re-dump compactly with UTF-8 kept (json.load → dumps keeps member order)
data = json.loads(src)
text = json.dumps(data, ensure_ascii=False, separators=(',', ':')).encode('utf-8')
HEX = set(b'0123456789abcdefABCDEF')
def lit(chunk):
    out = '"'
    prev_hex = False
    for b in chunk:
        if b >= 0x80 or b < 0x20:
            out += '\\x%02X' % b
            prev_hex = True
            continue
        if prev_hex and b in HEX:
            out += '" "'
        prev_hex = False
        c = chr(b)
        if c == '\\': out += '\\\\'
        elif c == '"': out += '\\"'
        elif c == '?': out += '\\?'
        else: out += c
    return out + '"'
lines = []
step = 1500
for i in range(0, len(text), step):
    lines.append('    ' + lit(text[i:i + step]) + ',')
with open(sys.argv[2], 'w') as f:
    f.write('// Generated from the web (AssetsLine: shared/uiDoc, shared/uiExport, shared/uiKit.ts, shared/textStyle.ts, backend/src/uiKit/prep.ts)\n')
    f.write('// by a golden script: every case\'s inputs and the web\'s outputs as one JSON document, in chunks. Do not edit by hand.\n')
    f.write('static const char* const ' + (sys.argv[3] if len(sys.argv) > 3 else 'kUiDocGoldenParts') + '[] = {\n')
    f.write('\n'.join(lines) + '\n')
    f.write('};\n')
print(len(lines), 'lines', len(text), 'bytes')
