import { create } from 'zustand';

export type NodeRunState = 'idle' | 'queued' | 'running' | 'done' | 'error';

export interface NodeOutput {
  mediaType: 'image' | 'text';
  url?: string;
  text?: string;
}

interface ExecutionStore {
  states: Map<string, NodeRunState>;
  errors: Map<string, string>;
  outputs: Map<string, NodeOutput>;
  log: string[];
  runningRequests: Map<string, AbortController>;

  reset(): void;
  clearNodeState(nodeId: string): void;
  setQueued(nodeIds: Iterable<string>): void;
  setState(nodeId: string, state: NodeRunState, error?: string): void;
  setOutput(nodeId: string, output: NodeOutput): void;
  appendLog(line: string): void;

  startRequest(nodeId: string, ac: AbortController): void;
  endRequest(nodeId: string): void;
  abortRequest(nodeId: string): void;
}

export const useExecutionStore = create<ExecutionStore>((set, get) => ({
  states: new Map(),
  errors: new Map(),
  outputs: new Map(),
  log: [],
  runningRequests: new Map(),

  reset() { set((s) => ({ states: new Map(), errors: new Map(), outputs: s.outputs, log: [] })); },
  clearNodeState(nodeId) {
    set((s) => {
      const states = new Map(s.states); states.delete(nodeId);
      const errors = new Map(s.errors); errors.delete(nodeId);
      return { states, errors };
    });
  },
  setQueued(nodeIds) {
    set((s) => {
      const states = new Map(s.states);
      for (const id of nodeIds) states.set(id, 'queued');
      return { states };
    });
  },
  setState(nodeId, state, error) {
    set((s) => {
      const states = new Map(s.states);
      states.set(nodeId, state);
      const errors = new Map(s.errors);
      if (error) errors.set(nodeId, error); else errors.delete(nodeId);
      return { states, errors };
    });
  },
  setOutput(nodeId, output) {
    set((s) => {
      const outputs = new Map(s.outputs);
      outputs.set(nodeId, output);
      return { outputs };
    });
  },
  appendLog(line) { set((s) => ({ log: [...s.log.slice(-199), line] })); },

  startRequest(nodeId, ac) {
    set((s) => {
      const next = new Map(s.runningRequests);
      next.set(nodeId, ac);
      return { runningRequests: next };
    });
  },
  endRequest(nodeId) {
    set((s) => {
      if (!s.runningRequests.has(nodeId)) return {};
      const next = new Map(s.runningRequests);
      next.delete(nodeId);
      return { runningRequests: next };
    });
  },
  abortRequest(nodeId) {
    const ac = get().runningRequests.get(nodeId);
    if (ac) ac.abort();
    get().endRequest(nodeId);
  },
}));
