import path from 'node:path';
import fs from 'node:fs/promises';
import { PROJECT_ROOT } from '../config/paths.js';
import type { AppSettings } from '../../../shared/types.js';

export type RootKey = 'assets' | 'contentDb';

export function rootBase(root: RootKey, settings: AppSettings): string {
  const sub = root === 'assets' ? settings.directories.assets : settings.directories.contentDatabase;
  return path.resolve(PROJECT_ROOT, sub);
}

export function safeJoin(root: RootKey, relative: string, settings: AppSettings): string {
  const base = rootBase(root, settings);
  const joined = path.resolve(base, relative);
  const baseNorm = base + path.sep;
  if (joined !== base && !joined.startsWith(baseNorm)) {
    throw new Error(`Path escapes root: ${relative}`);
  }
  return joined;
}

export async function ensureDirFor(filePath: string): Promise<void> {
  await fs.mkdir(path.dirname(filePath), { recursive: true });
}

export async function listDir(dir: string): Promise<{ name: string; isDir: boolean }[]> {
  try {
    const entries = await fs.readdir(dir, { withFileTypes: true });
    return entries.map((e) => ({ name: e.name, isDir: e.isDirectory() }));
  } catch (err: any) {
    if (err?.code === 'ENOENT') return [];
    throw err;
  }
}
