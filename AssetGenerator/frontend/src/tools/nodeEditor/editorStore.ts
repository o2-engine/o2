import { create } from 'zustand';
import { v4 as uuid } from 'uuid';
import type { EdgeData, NodeData, NodeTypeSchema, Pipeline, PortDef } from '../../../../shared/types.js';
import type { Camera } from './camera.js';
import { GRID_STEP } from './constants.js';
import { regenerateDynamicPorts } from './dynamicPorts.js';

export interface PendingEdgeContext {
  fromNodeId: string;
  fromPortId: string;
  portType: 'text' | 'image';
  side: 'output' | 'input';
}

export interface ContextMenuState {
  screenX: number;
  screenY: number;
  kind: 'background' | 'node' | 'edge' | 'create-from-edge';
  targetId?: string;
  worldX: number;
  worldY: number;
  pendingEdge?: PendingEdgeContext;
}

export interface PipelineSnapshot {
  pipelineId: string;
  name: string;
  nodes: NodeData[];
  edges: EdgeData[];
}

interface EditorStore {
  pipelineId: string;
  pipelineName: string;
  dirty: boolean;
  nodes: NodeData[];
  edges: EdgeData[];
  selection: Set<string>;
  camera: Camera;
  schemas: NodeTypeSchema[];
  contextMenu: ContextMenuState | null;
  originalState: PipelineSnapshot | null;

  loadSchemas(schemas: NodeTypeSchema[]): void;
  loadPipeline(p: Pipeline): void;
  newPipeline(): void;
  captureSnapshot(): void;
  revertToSnapshot(): void;

  addNodeAt(type: string, world: { x: number; y: number }): string;
  removeNodes(ids: string[]): void;
  updateNodeConfig(id: string, patch: Record<string, unknown>): void;
  moveNodes(ids: string[], dx: number, dy: number): void;
  snapSelectionToGrid(): void;
  resizeNode(id: string, width: number, height: number, position?: { x: number; y: number }): void;

  addEdge(fromNodeId: string, fromPortId: string, toNodeId: string, toPortId: string): void;
  removeEdges(ids: string[]): void;
  appendPasted(nodes: NodeData[], edges: EdgeData[], selectIds: string[]): void;
  addNodeFromPendingEdge(type: string, world: { x: number; y: number }, pending: PendingEdgeContext): string;

  setSelection(ids: string[]): void;
  toggleSelection(id: string): void;
  clearSelection(): void;
  selectAll(): void;

  setCamera(c: Camera): void;

  openContextMenu(s: ContextMenuState): void;
  closeContextMenu(): void;

  markDirty(): void;
  markClean(): void;
  setName(name: string): void;

  toPipeline(): Pipeline;
}

function snap(v: number): number {
  return Math.round(v / GRID_STEP) * GRID_STEP;
}

function defaultPortsFor(schema: NodeTypeSchema): { inputs: PortDef[]; outputs: PortDef[] } {
  return {
    inputs: schema.inputs.map((p) => ({ ...p, id: uuid() })),
    outputs: schema.outputs.map((p) => ({ ...p, id: uuid() })),
  };
}

function defaultConfigFor(schema: NodeTypeSchema): Record<string, unknown> {
  const cfg: Record<string, unknown> = {};
  for (const f of schema.configSchema) {
    if (f.default !== undefined) cfg[f.key] = f.default;
    else if (f.type === 'keyValueMap') cfg[f.key] = {};
    else cfg[f.key] = '';
  }
  return cfg;
}

export const useEditorStore = create<EditorStore>((set, get) => ({
  pipelineId: uuid(),
  pipelineName: 'untitled',
  dirty: false,
  nodes: [],
  edges: [],
  selection: new Set<string>(),
  camera: { x: 0, y: 0, scale: 1 },
  schemas: [],
  contextMenu: null,
  originalState: null,

  loadSchemas(schemas) { set({ schemas }); },

  captureSnapshot() {
    set((s) => ({
      originalState: {
        pipelineId: s.pipelineId,
        name: s.pipelineName,
        nodes: structuredClone(s.nodes),
        edges: structuredClone(s.edges),
      },
      dirty: false,
    }));
  },

  revertToSnapshot() {
    const orig = get().originalState;
    if (!orig) return;
    set({
      pipelineId: orig.pipelineId,
      pipelineName: orig.name,
      nodes: structuredClone(orig.nodes),
      edges: structuredClone(orig.edges),
      selection: new Set<string>(),
      dirty: false,
    });
  },

  loadPipeline(p) {
    const nodes = p.nodes.map((n) => regenerateDynamicPorts(n));
    const inputIdsByNode = new Map(nodes.map((n) => [n.id, new Set(n.inputs.map((pt) => pt.id))]));
    const outputIdsByNode = new Map(nodes.map((n) => [n.id, new Set(n.outputs.map((pt) => pt.id))]));
    const edges = p.edges.filter((e) =>
      (inputIdsByNode.get(e.toNodeId)?.has(e.toPortId) ?? false) &&
      (outputIdsByNode.get(e.fromNodeId)?.has(e.fromPortId) ?? false)
    );
    set({
      pipelineId: p.id,
      pipelineName: p.name,
      nodes,
      edges,
      selection: new Set<string>(),
      dirty: false,
      originalState: {
        pipelineId: p.id,
        name: p.name,
        nodes: structuredClone(nodes),
        edges: structuredClone(edges),
      },
    });
  },

  newPipeline() {
    const finishSchema = get().schemas.find((s) => s.type === 'finishImage');
    const nodes: NodeData[] = [];
    if (finishSchema) {
      nodes.push({
        id: uuid(),
        type: 'finishImage',
        position: { x: snap(400), y: snap(120) },
        config: defaultConfigFor(finishSchema),
        ...defaultPortsFor(finishSchema),
      });
    }
    const pipelineId = uuid();
    set({
      pipelineId,
      pipelineName: 'untitled',
      nodes,
      edges: [],
      selection: new Set<string>(),
      dirty: false,
      originalState: {
        pipelineId,
        name: 'untitled',
        nodes: structuredClone(nodes),
        edges: [],
      },
    });
  },

  addNodeAt(type, world) {
    const schema = get().schemas.find((s) => s.type === type);
    if (!schema) return '';
    const id = uuid();
    const node: NodeData = {
      id,
      type,
      position: { x: snap(world.x), y: snap(world.y) },
      config: defaultConfigFor(schema),
      ...defaultPortsFor(schema),
    };
    set((s) => ({ nodes: [...s.nodes, node], dirty: true }));
    return id;
  },

  removeNodes(ids) {
    if (!ids.length) return;
    const idSet = new Set(ids);
    set((s) => ({
      nodes: s.nodes.filter((n) => !idSet.has(n.id)),
      edges: s.edges.filter((e) => !idSet.has(e.fromNodeId) && !idSet.has(e.toNodeId)),
      selection: new Set([...s.selection].filter((id) => !idSet.has(id))),
      dirty: true,
    }));
  },

  updateNodeConfig(id, patch) {
    set((s) => {
      let updatedNode: NodeData | null = null;
      const nodes = s.nodes.map((n) => {
        if (n.id !== id) return n;
        const next = { ...n, config: { ...n.config, ...patch } };
        const regenerated = regenerateDynamicPorts(next);
        updatedNode = regenerated;
        return regenerated;
      });
      if (!updatedNode) return { nodes, dirty: true };
      // Prune edges whose endpoints reference ports that no longer exist on the updated node
      const inPortIds = new Set((updatedNode as NodeData).inputs.map((p) => p.id));
      const outPortIds = new Set((updatedNode as NodeData).outputs.map((p) => p.id));
      const edges = s.edges.filter((e) => {
        if (e.toNodeId === id && !inPortIds.has(e.toPortId)) return false;
        if (e.fromNodeId === id && !outPortIds.has(e.fromPortId)) return false;
        return true;
      });
      return { nodes, edges, dirty: true };
    });
  },

  moveNodes(ids, dx, dy) {
    if (!ids.length) return;
    const idSet = new Set(ids);
    set((s) => ({
      nodes: s.nodes.map((n) => idSet.has(n.id) ? { ...n, position: { x: n.position.x + dx, y: n.position.y + dy } } : n),
      dirty: true,
    }));
  },

  snapSelectionToGrid() {
    const sel = get().selection;
    if (!sel.size) return;
    set((s) => ({
      nodes: s.nodes.map((n) => sel.has(n.id) ? { ...n, position: { x: snap(n.position.x), y: snap(n.position.y) } } : n),
      dirty: true,
    }));
  },

  resizeNode(id, width, height, position) {
    set((s) => ({
      nodes: s.nodes.map((n) => n.id === id
        ? {
            ...n,
            size: { width: snap(width), height: snap(height) },
            position: position ? { x: snap(position.x), y: snap(position.y) } : n.position,
          }
        : n),
      dirty: true,
    }));
  },

  addEdge(fromNodeId, fromPortId, toNodeId, toPortId) {
    if (fromNodeId === toNodeId) return;
    const state = get();
    const from = state.nodes.find((n) => n.id === fromNodeId);
    const to = state.nodes.find((n) => n.id === toNodeId);
    if (!from || !to) return;
    const fromPort = from.outputs.find((p) => p.id === fromPortId);
    const toPort = to.inputs.find((p) => p.id === toPortId);
    if (!fromPort || !toPort) return;
    if (fromPort.type !== toPort.type) return;
    set((s) => ({
      edges: [
        ...s.edges.filter((e) => !(e.toNodeId === toNodeId && e.toPortId === toPortId)),
        { id: uuid(), fromNodeId, fromPortId, toNodeId, toPortId },
      ],
      dirty: true,
    }));
  },

  removeEdges(ids) {
    if (!ids.length) return;
    const set2 = new Set(ids);
    set((s) => ({ edges: s.edges.filter((e) => !set2.has(e.id)), dirty: true }));
  },

  appendPasted(nodes, edges, selectIds) {
    set((s) => ({
      nodes: [...s.nodes, ...nodes],
      edges: [...s.edges, ...edges],
      selection: new Set(selectIds),
      dirty: true,
    }));
  },

  addNodeFromPendingEdge(type, world, pending) {
    const schema = get().schemas.find((s) => s.type === type);
    if (!schema) return '';
    const id = uuid();
    const node: NodeData = {
      id,
      type,
      position: { x: snap(world.x), y: snap(world.y) },
      config: defaultConfigFor(schema),
      ...defaultPortsFor(schema),
    };

    let newEdge: EdgeData | null = null;
    if (pending.side === 'output') {
      const targetPort = node.inputs.find((p) => p.type === pending.portType);
      if (targetPort) {
        newEdge = {
          id: uuid(),
          fromNodeId: pending.fromNodeId,
          fromPortId: pending.fromPortId,
          toNodeId: id,
          toPortId: targetPort.id,
        };
      }
    } else {
      const sourcePort = node.outputs.find((p) => p.type === pending.portType);
      if (sourcePort) {
        newEdge = {
          id: uuid(),
          fromNodeId: id,
          fromPortId: sourcePort.id,
          toNodeId: pending.fromNodeId,
          toPortId: pending.fromPortId,
        };
      }
    }

    set((s) => ({
      nodes: [...s.nodes, node],
      edges: newEdge ? [...s.edges.filter((e) => !(e.toNodeId === newEdge!.toNodeId && e.toPortId === newEdge!.toPortId)), newEdge] : s.edges,
      selection: new Set([id]),
      dirty: true,
    }));
    return id;
  },

  setSelection(ids) { set({ selection: new Set(ids) }); },
  toggleSelection(id) {
    set((s) => {
      const next = new Set(s.selection);
      if (next.has(id)) next.delete(id); else next.add(id);
      return { selection: next };
    });
  },
  clearSelection() { set({ selection: new Set() }); },
  selectAll() { set((s) => ({ selection: new Set(s.nodes.map((n) => n.id)) })); },

  setCamera(c) { set({ camera: c }); },

  openContextMenu(s) { set({ contextMenu: s }); },
  closeContextMenu() { set({ contextMenu: null }); },

  markDirty() { set({ dirty: true }); },
  markClean() { /* no-op kept for compat */ },
  setName(name) { set({ pipelineName: name, dirty: true }); },

  toPipeline() {
    const s = get();
    return {
      schemaVersion: 1,
      id: s.pipelineId,
      name: s.pipelineName,
      nodes: s.nodes,
      edges: s.edges,
    };
  },
}));
