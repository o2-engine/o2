import type { NodeImpl, PortValue } from './types.js';

interface PartSlot { id: string; name: string }

export const textConcatNode: NodeImpl = {
  schema: {
    type: 'textConcat',
    label: 'Text concat',
    category: 'transform',
    description: 'Concatenate multiple text inputs in the configured order, joined by a separator.',
    inputs: [],
    outputs: [
      { id: 'out', name: 'out', type: 'text' },
    ],
    configSchema: [],
  },
  async run(_ctx, inputs, node) {
    const parts = (node.config.parts as PartSlot[] | undefined) ?? [];
    const separator = String(node.config.separator ?? '');
    const pieces = parts.map((p) => {
      const name = String(p.name ?? '').trim();
      if (!name) return '';
      const value = inputs.get(name);
      return value?.type === 'text' ? value.data : '';
    });
    const out: PortValue = { type: 'text', data: pieces.join(separator) };
    return new Map([['out', out]]);
  },
};
