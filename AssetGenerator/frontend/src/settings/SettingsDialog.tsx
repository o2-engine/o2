import React, { useEffect, useState } from 'react';
import { useSettingsStore } from './settingsStore.js';
import type { AppSettings } from '../../../shared/types.js';

export function SettingsDialog({ onClose }: { onClose: () => void }): JSX.Element {
  const stored = useSettingsStore((s) => s.settings);
  const load = useSettingsStore((s) => s.load);
  const save = useSettingsStore((s) => s.save);

  const [draft, setDraft] = useState<AppSettings>(stored);
  const [saving, setSaving] = useState(false);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    load().catch((e) => setError(String(e)));
  }, [load]);

  useEffect(() => {
    setDraft(stored);
  }, [stored]);

  const setKey = (provider: keyof AppSettings['apiKeys'], v: string) =>
    setDraft({ ...draft, apiKeys: { ...draft.apiKeys, [provider]: v } });

  const setDir = (key: keyof AppSettings['directories'], v: string) =>
    setDraft({ ...draft, directories: { ...draft.directories, [key]: v } });

  const onSave = async () => {
    setSaving(true);
    setError(null);
    try {
      await save(draft);
      onClose();
    } catch (e) {
      setError(String(e));
    } finally {
      setSaving(false);
    }
  };

  return (
    <div className="dialog-backdrop" onMouseDown={(e) => { if (e.target === e.currentTarget) onClose(); }}>
      <div className="dialog">
        <h2>Settings</h2>

        <div className="dialog-section">API keys</div>
        <div className="dialog-row">
          <label>Gemini</label>
          <input type="password" value={draft.apiKeys.gemini} onChange={(e) => setKey('gemini', e.target.value)} placeholder="AIza…" />
        </div>
        <div className="dialog-row">
          <label>Claude</label>
          <input type="password" value={draft.apiKeys.claude} onChange={(e) => setKey('claude', e.target.value)} placeholder="(not yet used)" />
        </div>
        <div className="dialog-row">
          <label>OpenAI</label>
          <input type="password" value={draft.apiKeys.openai} onChange={(e) => setKey('openai', e.target.value)} placeholder="(not yet used)" />
        </div>

        <div className="dialog-section">Directories (relative to mount root)</div>
        <div className="dialog-row">
          <label>Assets</label>
          <input type="text" value={draft.directories.assets} onChange={(e) => setDir('assets', e.target.value)} placeholder="Assets" />
        </div>
        <div className="dialog-row">
          <label>ContentDatabase</label>
          <input type="text" value={draft.directories.contentDatabase} onChange={(e) => setDir('contentDatabase', e.target.value)} placeholder="ContentDatabase" />
        </div>

        {error && <div style={{ color: 'var(--red)', marginTop: 8 }}>{error}</div>}

        <div className="dialog-actions">
          <button onClick={onClose}>Cancel</button>
          <button onClick={onSave} disabled={saving}>{saving ? 'Saving…' : 'Save'}</button>
        </div>
      </div>
    </div>
  );
}
