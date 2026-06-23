import fs from 'node:fs/promises';
import path from 'node:path';
import type { NodeImpl, PortValue } from './types.js';
import { safeJoin } from '../utils/files.js';

function mimeForExt(ext: string): string {
  switch (ext.toLowerCase()) {
    case '.png': return 'image/png';
    case '.jpg':
    case '.jpeg': return 'image/jpeg';
    case '.webp': return 'image/webp';
    case '.gif': return 'image/gif';
    case '.bmp': return 'image/bmp';
    case '.tga': return 'image/x-tga';
    case '.tiff':
    case '.tif': return 'image/tiff';
    default: return 'application/octet-stream';
  }
}

export const sourceImageNode: NodeImpl = {
  schema: {
    type: 'sourceImage',
    label: 'Image source',
    category: 'source',
    description: 'Read an image file from disk.',
    inputs: [],
    outputs: [
      { id: 'out', name: 'out', type: 'image' },
    ],
    configSchema: [
      { key: 'sourceRoot', label: 'Root', type: 'enum', options: ['assets', 'contentDb'], default: 'contentDb' },
      { key: 'filePath', label: 'Path', type: 'string', placeholder: 'NodeImageGen/refs/sample.png' },
    ],
  },
  async run(ctx, _inputs, node) {
    const root = (node.config.sourceRoot as 'assets' | 'contentDb') ?? 'contentDb';
    const rel = String(node.config.filePath ?? '').trim();
    if (!rel) throw new Error('sourceImage: path is empty');
    const abs = safeJoin(root, rel, ctx.settings);
    const buf = await fs.readFile(abs);
    const mime = mimeForExt(path.extname(abs));
    const value: PortValue = { type: 'image', data: buf, mimeType: mime };
    return new Map([['out', value]]);
  },
};
