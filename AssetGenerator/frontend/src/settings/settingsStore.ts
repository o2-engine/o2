import { create } from 'zustand';
import type { AppSettings } from '../../../shared/types.js';
import { DEFAULT_SETTINGS } from '../../../shared/types.js';
import { api } from '../api/client.js';

interface SettingsStore {
  settings: AppSettings;
  loaded: boolean;
  load(): Promise<void>;
  save(next: AppSettings): Promise<void>;
}

export const useSettingsStore = create<SettingsStore>((set) => ({
  settings: DEFAULT_SETTINGS,
  loaded: false,
  async load() {
    const s = await api.getSettings();
    set({ settings: s, loaded: true });
  },
  async save(next: AppSettings) {
    const saved = await api.putSettings(next);
    set({ settings: saved });
  },
}));
