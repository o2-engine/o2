import fs from 'node:fs/promises';
import type { NodeImpl } from './types.js';
import { ensureDirFor, safeJoin } from '../utils/files.js';

export const finishImageNode: NodeImpl = {
  schema: {
    type: 'finishImage',
    label: 'Finish — image',
    category: 'output',
    description: 'Save the connected image input to a PNG. Press Play to run the pipeline that feeds this node.',
    inputs: [
      { id: 'in', name: 'in', type: 'image' },
    ],
    outputs: [],
    configSchema: [
      { key: 'targetRoot', label: 'Root', type: 'enum', options: ['assets', 'contentDb'], default: 'assets' },
      { key: 'path', label: 'Relative path', type: 'string', placeholder: 'sprites/cat.png' },
    ],
    hasPlay: true,
  },
  async run(ctx, inputs, node) {
    const value = inputs.get('in');
    if (!value) throw new Error('finishImage: input "in" is not connected');
    if (value.type !== 'image') throw new Error('finishImage: input must be image');

    const targetRoot = (node.config.targetRoot as 'assets' | 'contentDb') ?? 'assets';
    const relPath = String(node.config.path ?? '').trim();
    if (!relPath) throw new Error('finishImage: relative path is empty');

    const abs = safeJoin(targetRoot, relPath, ctx.settings);
    await ensureDirFor(abs);
    await fs.writeFile(abs, value.data);
    ctx.log(`finishImage: wrote ${value.data.length} bytes to ${abs}`);
    return new Map();
  },
};
