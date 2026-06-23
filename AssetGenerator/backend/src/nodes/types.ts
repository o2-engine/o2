import type { AppSettings, NodeData, NodeTypeSchema, PortType } from '../../../shared/types.js';

export type PortValue =
  | { type: 'text'; data: string }
  | { type: 'image'; data: Buffer; mimeType: string };

export interface ExecutionContext {
  settings: AppSettings;
  log: (msg: string) => void;
}

export interface NodeImpl {
  schema: NodeTypeSchema;
  run(ctx: ExecutionContext, inputs: Map<string, PortValue>, node: NodeData): Promise<Map<string, PortValue>>;
}

export function valueOfType(v: PortValue | undefined, type: PortType): PortValue | undefined {
  if (!v) return undefined;
  if (v.type !== type) throw new Error(`Expected ${type} input, got ${v.type}`);
  return v;
}
