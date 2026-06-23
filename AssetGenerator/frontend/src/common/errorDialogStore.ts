import { create } from 'zustand';
import { v4 as uuid } from 'uuid';

export interface ErrorEntry {
  id: string;
  message: string;
  details?: string;
  nodeId?: string;
  ts: number;
}

interface ErrorDialogStore {
  queue: ErrorEntry[];
  push(input: { message: string; details?: string; nodeId?: string }): void;
  dismiss(id: string): void;
  clear(): void;
}

export const useErrorDialogStore = create<ErrorDialogStore>((set) => ({
  queue: [],
  push(input) {
    const entry: ErrorEntry = { id: uuid(), message: input.message, details: input.details, nodeId: input.nodeId, ts: Date.now() };
    set((s) => ({ queue: [...s.queue, entry] }));
  },
  dismiss(id) { set((s) => ({ queue: s.queue.filter((e) => e.id !== id) })); },
  clear() { set({ queue: [] }); },
}));
