import { create } from 'zustand';
import { v4 as uuid } from 'uuid';

export type ToastLevel = 'info' | 'error';

export interface Toast {
  id: string;
  level: ToastLevel;
  message: string;
  details?: string;
  ts: number;
}

interface ToastStore {
  toasts: Toast[];
  push(input: { level: ToastLevel; message: string; details?: string }): string;
  dismiss(id: string): void;
  clear(): void;
}

const AUTO_DISMISS_MS = 5000;

export const useToastStore = create<ToastStore>((set, get) => ({
  toasts: [],
  push(input) {
    const id = uuid();
    const toast: Toast = { id, level: input.level, message: input.message, details: input.details, ts: Date.now() };
    set((s) => ({ toasts: [...s.toasts, toast] }));
    if (input.level === 'info') {
      setTimeout(() => {
        if (get().toasts.some((t) => t.id === id)) get().dismiss(id);
      }, AUTO_DISMISS_MS);
    }
    return id;
  },
  dismiss(id) { set((s) => ({ toasts: s.toasts.filter((t) => t.id !== id) })); },
  clear() { set({ toasts: [] }); },
}));
