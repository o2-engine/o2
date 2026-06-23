import fs from 'node:fs/promises';
import path from 'node:path';
import type { AppSettings } from '../../../shared/types.js';
import { DEFAULT_SETTINGS } from '../../../shared/types.js';
import { DATA_DIR, SETTINGS_FILE } from './paths.js';

async function ensureDataDir(): Promise<void> {
  await fs.mkdir(DATA_DIR, { recursive: true });
}

export async function readSettings(): Promise<AppSettings> {
  await ensureDataDir();
  try {
    const raw = await fs.readFile(SETTINGS_FILE, 'utf8');
    const parsed = JSON.parse(raw) as Partial<AppSettings>;
    return {
      apiKeys: { ...DEFAULT_SETTINGS.apiKeys, ...(parsed.apiKeys ?? {}) },
      directories: { ...DEFAULT_SETTINGS.directories, ...(parsed.directories ?? {}) },
    };
  } catch (err: any) {
    if (err?.code === 'ENOENT') {
      await writeSettings(DEFAULT_SETTINGS);
      return DEFAULT_SETTINGS;
    }
    throw err;
  }
}

export async function writeSettings(settings: AppSettings): Promise<void> {
  await ensureDataDir();
  const tmp = SETTINGS_FILE + '.tmp';
  await fs.writeFile(tmp, JSON.stringify(settings, null, 2), 'utf8');
  await fs.rename(tmp, SETTINGS_FILE);
}

export function effectiveApiKey(settings: AppSettings, provider: 'gemini' | 'claude' | 'openai'): string {
  const fromSettings = settings.apiKeys[provider];
  if (fromSettings && fromSettings.trim()) return fromSettings.trim();
  const envName =
    provider === 'gemini' ? 'GEMINI_API_KEY' :
    provider === 'claude' ? 'ANTHROPIC_API_KEY' :
    'OPENAI_API_KEY';
  return (process.env[envName] ?? '').trim();
}
