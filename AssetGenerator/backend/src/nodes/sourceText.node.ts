import type { NodeImpl, PortValue } from './types.js';

export const sourceTextNode: NodeImpl = {
  schema: {
    type: 'sourceText',
    label: 'Text source',
    category: 'source',
    description: 'Provide an inline text value.',
    inputs: [],
    outputs: [
      { id: 'out', name: 'out', type: 'text' },
    ],
    configSchema: [
      { key: 'text', label: 'Text', type: 'multiline', placeholder: 'Inline text…' },
    ],
  },
  async run(_ctx, _inputs, node) {
    const text = String(node.config.text ?? '');
    const value: PortValue = { type: 'text', data: text };
    return new Map([['out', value]]);
  },
};
