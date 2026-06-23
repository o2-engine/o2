import type { NodeImpl, PortValue } from './types.js';

export const sourceTextFileNode: NodeImpl = {
  schema: {
    type: 'sourceTextFile',
    label: 'Text source (from file)',
    category: 'source',
    description: 'Text snapshot loaded from a file. Pick a file via the system dialog; preview is read-only.',
    inputs: [],
    outputs: [
      { id: 'out', name: 'out', type: 'text' },
    ],
    configSchema: [
      { key: 'sourceName', label: 'Source name', type: 'string' },
      { key: 'text', label: 'Text', type: 'multiline' },
    ],
  },
  async run(_ctx, _inputs, node) {
    const text = String(node.config.text ?? '');
    const value: PortValue = { type: 'text', data: text };
    return new Map([['out', value]]);
  },
};
