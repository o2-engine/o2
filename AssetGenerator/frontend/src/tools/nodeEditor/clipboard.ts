import { v4 as uuid } from 'uuid';
import type { EdgeData, NodeData } from '../../../../shared/types.js';

export interface Clipboard {
  nodes: NodeData[];
  edges: EdgeData[];
}

let clipboard: Clipboard | null = null;

export function copySelection(nodes: NodeData[], edges: EdgeData[], selection: Set<string>): void {
  const sel = new Set(selection);
  const selectedNodes = nodes.filter((n) => sel.has(n.id));
  const innerEdges = edges.filter((e) => sel.has(e.fromNodeId) && sel.has(e.toNodeId));
  clipboard = {
    nodes: selectedNodes.map((n) => structuredClone(n)),
    edges: innerEdges.map((e) => structuredClone(e)),
  };
}

export function hasClipboard(): boolean { return !!clipboard; }

export function pasteClipboard(offset: { x: number; y: number }): { nodes: NodeData[]; edges: EdgeData[]; newIds: string[] } {
  if (!clipboard) return { nodes: [], edges: [], newIds: [] };

  const nodeIdMap = new Map<string, string>();
  const portIdMap = new Map<string, string>();

  const newNodes: NodeData[] = clipboard.nodes.map((n) => {
    const newId = uuid();
    nodeIdMap.set(n.id, newId);
    const inputs = n.inputs.map((p) => {
      const id = uuid();
      portIdMap.set(p.id, id);
      return { ...p, id };
    });
    const outputs = n.outputs.map((p) => {
      const id = uuid();
      portIdMap.set(p.id, id);
      return { ...p, id };
    });
    return {
      ...n,
      id: newId,
      position: { x: n.position.x + offset.x, y: n.position.y + offset.y },
      inputs,
      outputs,
    };
  });

  const newEdges: EdgeData[] = clipboard.edges.map((e) => ({
    id: uuid(),
    fromNodeId: nodeIdMap.get(e.fromNodeId) ?? e.fromNodeId,
    fromPortId: portIdMap.get(e.fromPortId) ?? e.fromPortId,
    toNodeId: nodeIdMap.get(e.toNodeId) ?? e.toNodeId,
    toPortId: portIdMap.get(e.toPortId) ?? e.toPortId,
  }));

  return { nodes: newNodes, edges: newEdges, newIds: [...nodeIdMap.values()] };
}
