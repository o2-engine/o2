import path from 'node:path';

export const PROJECT_ROOT = process.env.PROJECT_ROOT ?? '/workspace';
export const DATA_DIR = path.resolve(process.env.DATA_DIR ?? path.resolve(process.cwd(), 'data'));
export const SETTINGS_FILE = path.join(DATA_DIR, 'settings.json');

export function resolveRoot(root: 'assets' | 'contentDb', relative: string, settings: { directories: { assets: string; contentDatabase: string } }): string {
  const base = root === 'assets' ? settings.directories.assets : settings.directories.contentDatabase;
  return path.join(PROJECT_ROOT, base, relative);
}
