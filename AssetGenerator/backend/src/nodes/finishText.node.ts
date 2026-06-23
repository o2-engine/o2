import fs from 'node:fs/promises';
import type { NodeImpl } from './types.js';
import { ensureDirFor, safeJoin } from '../utils/files.js';

export const finishTextNode: NodeImpl = {
  schema: {
    type: 'finishText',
    label: 'Finish — text',
    category: 'output',
    description: 'Save the connected text input to a file. Press Play to run the pipeline that feeds this node.',
    inputs: [
      { id: 'in', name: 'in', type: 'text' },
    ],
    outputs: [],
    configSchema: [
      { key: 'targetRoot', label: 'Root', type: 'enum', options: ['assets', 'contentDb'], default: 'contentDb' },
      { key: 'path', label: 'Relative path', type: 'string', placeholder: 'NodeImageGen/out/result.txt' },
    ],
    hasPlay: true,
  },
  async run(ctx, inputs, node) {
    const value = inputs.get('in');
    if (!value) throw new Error('finishText: input "in" is not connected');
    if (value.type !== 'text') throw new Error('finishText: input must be text');

    const targetRoot = (node.config.targetRoot as 'assets' | 'contentDb') ?? 'contentDb';
    const relPath = String(node.config.path ?? '').trim();
    if (!relPath) throw new Error('finishText: relative path is empty');

    const abs = safeJoin(targetRoot, relPath, ctx.settings);
    await ensureDirFor(abs);
    await fs.writeFile(abs, value.data, 'utf8');
    ctx.log(`finishText: wrote ${value.data.length} chars to ${abs}`);
    return new Map();
  },
};
