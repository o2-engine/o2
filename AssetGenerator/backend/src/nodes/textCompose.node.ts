import type { NodeImpl, PortValue } from './types.js';

interface VarSlot { id: string; name: string }

export const textComposeNode: NodeImpl = {
  schema: {
    type: 'textCompose',
    label: 'Text compose',
    category: 'transform',
    description: 'Substitute {name} placeholders in the template with text from per-variable inputs.',
    inputs: [
      { id: 'template', name: 'template', type: 'text' },
    ],
    outputs: [
      { id: 'out', name: 'out', type: 'text' },
    ],
    configSchema: [],
  },
  async run(_ctx, inputs, node) {
    const templateValue = inputs.get('template');
    if (!templateValue) throw new Error('textCompose: template input is not connected');
    if (templateValue.type !== 'text') throw new Error('textCompose: template input must be text');

    const vars = (node.config.vars as VarSlot[] | undefined) ?? [];
    let result = templateValue.data;
    for (const v of vars) {
      const name = String(v.name ?? '').trim();
      if (!name) continue;
      const value = inputs.get(name);
      const text = value?.type === 'text' ? value.data : '';
      result = result.split(`{${name}}`).join(text);
    }
    const out: PortValue = { type: 'text', data: result };
    return new Map([['out', out]]);
  },
};
