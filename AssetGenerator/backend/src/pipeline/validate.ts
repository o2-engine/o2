import type { EdgeData, NodeData, Pipeline } from '../../../shared/types.js';

export interface ValidationError {
  message: string;
  nodeId?: string;
  edgeId?: string;
}

export function validatePipeline(pipeline: Pipeline): ValidationError[] {
  const errors: ValidationError[] = [];
  const nodesById = new Map<string, NodeData>(pipeline.nodes.map((n) => [n.id, n]));

  for (const edge of pipeline.edges) {
    const from = nodesById.get(edge.fromNodeId);
    const to = nodesById.get(edge.toNodeId);
    if (!from) {
      errors.push({ edgeId: edge.id, message: `Edge ${edge.id}: source node ${edge.fromNodeId} not found` });
      continue;
    }
    if (!to) {
      errors.push({ edgeId: edge.id, message: `Edge ${edge.id}: target node ${edge.toNodeId} not found` });
      continue;
    }
    const fromPort = from.outputs.find((p) => p.id === edge.fromPortId);
    const toPort = to.inputs.find((p) => p.id === edge.toPortId);
    if (!fromPort) {
      errors.push({ edgeId: edge.id, message: `Edge ${edge.id}: source port ${edge.fromPortId} missing on node ${from.type}` });
      continue;
    }
    if (!toPort) {
      errors.push({ edgeId: edge.id, message: `Edge ${edge.id}: target port ${edge.toPortId} missing on node ${to.type}` });
      continue;
    }
    if (fromPort.type !== toPort.type) {
      errors.push({ edgeId: edge.id, message: `Edge ${edge.id}: type mismatch ${fromPort.type} → ${toPort.type}` });
    }
  }

  const seen = new Map<string, EdgeData>();
  for (const edge of pipeline.edges) {
    const key = `${edge.toNodeId}:${edge.toPortId}`;
    if (seen.has(key)) {
      errors.push({ edgeId: edge.id, message: `Target port ${key} has more than one incoming edge` });
    } else {
      seen.set(key, edge);
    }
  }

  const adj = new Map<string, string[]>();
  for (const n of pipeline.nodes) adj.set(n.id, []);
  for (const e of pipeline.edges) {
    if (adj.has(e.fromNodeId)) adj.get(e.fromNodeId)!.push(e.toNodeId);
  }
  const WHITE = 0, GRAY = 1, BLACK = 2;
  const color = new Map<string, number>(pipeline.nodes.map((n) => [n.id, WHITE]));
  const cycleStart = (() => {
    for (const node of pipeline.nodes) {
      if (color.get(node.id) !== WHITE) continue;
      const stack: { id: string; idx: number }[] = [{ id: node.id, idx: 0 }];
      color.set(node.id, GRAY);
      while (stack.length) {
        const top = stack[stack.length - 1];
        const children = adj.get(top.id) ?? [];
        if (top.idx >= children.length) {
          color.set(top.id, BLACK);
          stack.pop();
          continue;
        }
        const next = children[top.idx++];
        const c = color.get(next);
        if (c === GRAY) return next;
        if (c === WHITE) {
          color.set(next, GRAY);
          stack.push({ id: next, idx: 0 });
        }
      }
    }
    return null;
  })();
  if (cycleStart) errors.push({ nodeId: cycleStart, message: `Pipeline contains a cycle (involves node ${cycleStart})` });

  return errors;
}

export function reachableFrom(pipeline: Pipeline, startNodeId: string): Set<string> {
  const adj = new Map<string, string[]>();
  for (const n of pipeline.nodes) adj.set(n.id, []);
  for (const e of pipeline.edges) adj.get(e.fromNodeId)?.push(e.toNodeId);

  const seen = new Set<string>([startNodeId]);
  const queue = [startNodeId];
  while (queue.length) {
    const id = queue.shift()!;
    for (const next of adj.get(id) ?? []) {
      if (!seen.has(next)) {
        seen.add(next);
        queue.push(next);
      }
    }
  }
  return seen;
}

export function topoOrder(pipeline: Pipeline, restrict: Set<string>): string[] {
  const inDeg = new Map<string, number>();
  for (const id of restrict) inDeg.set(id, 0);
  for (const e of pipeline.edges) {
    if (restrict.has(e.fromNodeId) && restrict.has(e.toNodeId)) {
      inDeg.set(e.toNodeId, (inDeg.get(e.toNodeId) ?? 0) + 1);
    }
  }
  const queue: string[] = [];
  for (const [id, d] of inDeg) if (d === 0) queue.push(id);
  const order: string[] = [];
  while (queue.length) {
    const id = queue.shift()!;
    order.push(id);
    for (const e of pipeline.edges) {
      if (e.fromNodeId !== id) continue;
      if (!restrict.has(e.toNodeId)) continue;
      const d = (inDeg.get(e.toNodeId) ?? 0) - 1;
      inDeg.set(e.toNodeId, d);
      if (d === 0) queue.push(e.toNodeId);
    }
  }
  return order;
}
