import type { FastifyInstance } from 'fastify';
import fs from 'node:fs/promises';
import path from 'node:path';
import { DATA_DIR } from '../config/paths.js';

export const CACHE_DIR = path.join(DATA_DIR, 'cache');
export const CONTENT_CACHE_DIR = path.join(DATA_DIR, 'cache-content');

export async function ensureCacheDir(): Promise<void> {
  await fs.mkdir(CACHE_DIR, { recursive: true });
}

export async function ensureContentCacheDir(): Promise<void> {
  await fs.mkdir(CONTENT_CACHE_DIR, { recursive: true });
}

export function cacheFilePath(nodeId: string, ext = '.png'): string {
  const safe = nodeId.replace(/[^a-zA-Z0-9_-]/g, '_');
  return path.join(CACHE_DIR, `${safe}${ext}`);
}

export function cacheUrlFor(nodeId: string, ext = '.png'): string {
  const safe = nodeId.replace(/[^a-zA-Z0-9_-]/g, '_');
  return `/api/cache/${safe}${ext}?t=${Date.now()}`;
}

export function contentCachePath(sig: string, ext: string): string {
  return path.join(CONTENT_CACHE_DIR, `${sig}${ext}`);
}

export async function cacheRoutes(app: FastifyInstance): Promise<void> {
  app.get('/:file', async (req, reply) => {
    const { file } = req.params as { file: string };
    if (!/^[a-zA-Z0-9_.-]+$/.test(file)) {
      reply.code(400);
      return { error: 'invalid name' };
    }
    const full = path.join(CACHE_DIR, file);
    try {
      const buf = await fs.readFile(full);
      const ext = path.extname(file).toLowerCase();
      const mime =
        ext === '.png' ? 'image/png' :
        ext === '.jpg' || ext === '.jpeg' ? 'image/jpeg' :
        ext === '.webp' ? 'image/webp' :
        'application/octet-stream';
      reply.header('content-type', mime);
      reply.header('cache-control', 'no-store');
      return reply.send(buf);
    } catch (err: any) {
      if (err?.code === 'ENOENT') {
        reply.code(404);
        return { error: 'Not found' };
      }
      throw err;
    }
  });
}
