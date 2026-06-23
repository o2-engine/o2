import { v4 as uuid } from 'uuid';
import type { NodeData, PortDef } from '../../../../shared/types.js';

export interface Slot { id: string; name: string }

interface RegenResult {
  inputs?: PortDef[];
  outputs?: PortDef[];
}

type Generator = (node: NodeData) => RegenResult;

const generators: Record<string, Generator> = {
  textCompose(node) {
    const vars = ((node.config.vars as Slot[] | undefined) ?? []);
    const tmpl = node.inputs.find((p) => p.name === 'template')
      ?? { id: uuid(), name: 'template', type: 'text' as const };
    const varPorts: PortDef[] = vars.map((v) => {
      const existing = node.inputs.find((p) => p.id === v.id);
      return existing ? { ...existing, name: v.name, type: 'text' } : { id: v.id, name: v.name, type: 'text' };
    });
    return { inputs: [tmpl, ...varPorts] };
  },
  textConcat(node) {
    const parts = ((node.config.parts as Slot[] | undefined) ?? []);
    const partPorts: PortDef[] = parts.map((p) => {
      const existing = node.inputs.find((pp) => pp.id === p.id);
      return existing ? { ...existing, name: p.name, type: 'text' } : { id: p.id, name: p.name, type: 'text' };
    });
    return { inputs: partPorts };
  },
};

export function regenerateDynamicPorts(node: NodeData): NodeData {
  const g = generators[node.type];
  if (!g) return node;
  const r = g(node);
  return { ...node, inputs: r.inputs ?? node.inputs, outputs: r.outputs ?? node.outputs };
}

export function newSlot(name = ''): Slot {
  return { id: uuid(), name };
}
