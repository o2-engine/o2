import type { NodeData, PortDef } from '../../../../shared/types.js';
import { NODE_HEADER_HEIGHT, NODE_PAD_BOTTOM, NODE_PAD_TOP, NODE_PORT_ROW, NODE_WIDTH } from './constants.js';
import { bodyHeightFor } from './nodeRenderers.js';

export const NODE_MIN_WIDTH = 180;
export const NODE_MIN_HEIGHT = 80;

export function portsRowsHeight(node: NodeData): number {
  const rows = Math.max(node.inputs.length, node.outputs.length);
  return rows * NODE_PORT_ROW;
}

export function autoHeight(node: NodeData): number {
  return NODE_HEADER_HEIGHT + NODE_PAD_TOP + portsRowsHeight(node) + bodyHeightFor(node) + NODE_PAD_BOTTOM;
}

export function nodeWidth(node: NodeData): number {
  return node.size?.width ?? NODE_WIDTH;
}

export function nodeHeight(node: NodeData): number {
  return node.size?.height ?? autoHeight(node);
}

export function bodyAreaHeight(node: NodeData): number {
  const headerAndPorts = NODE_HEADER_HEIGHT + NODE_PAD_TOP + portsRowsHeight(node);
  return Math.max(0, nodeHeight(node) - headerAndPorts - NODE_PAD_BOTTOM);
}

export function nodeBounds(node: NodeData): { x: number; y: number; w: number; h: number } {
  return { x: node.position.x, y: node.position.y, w: nodeWidth(node), h: nodeHeight(node) };
}

export function portPosition(node: NodeData, port: PortDef, side: 'input' | 'output'): { x: number; y: number } {
  const index = side === 'input'
    ? node.inputs.findIndex((p) => p.id === port.id)
    : node.outputs.findIndex((p) => p.id === port.id);
  const localY = NODE_HEADER_HEIGHT + NODE_PAD_TOP + (index + 0.5) * NODE_PORT_ROW;
  const localX = side === 'input' ? 0 : nodeWidth(node);
  return { x: node.position.x + localX, y: node.position.y + localY };
}

export function rectsIntersect(a: { x: number; y: number; w: number; h: number }, b: { x: number; y: number; w: number; h: number }): boolean {
  return !(a.x + a.w < b.x || b.x + b.w < a.x || a.y + a.h < b.y || b.y + b.h < a.y);
}
