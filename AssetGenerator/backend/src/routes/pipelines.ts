import type { FastifyInstance } from 'fastify';
import fs from 'node:fs/promises';
import path from 'node:path';
import { v4 as uuid } from 'uuid';
import type { Pipeline } from '../../../shared/types.js';
import { readSettings } from '../config/settings.js';
import { ensureDirFor, listDir, safeJoin } from '../utils/files.js';

const PIPELINES_REL = 'NodeImageGen/Pipelines';

function sanitizeName(name: string): string {
  const trimmed = name.trim();
  if (!trimmed) throw new Error('Pipeline name is empty');
  if (/[\\/:*?"<>|]/.test(trimmed)) throw new Error('Pipeline name has invalid characters');
  return trimmed;
}

export async function pipelinesRoutes(app: FastifyInstance): Promise<void> {
  app.get('/', async () => {
    const settings = await readSettings();
    const dir = safeJoin('contentDb', PIPELINES_REL, settings);
    const entries = await listDir(dir);
    const files = entries.filter((e) => !e.isDir && e.name.endsWith('.json'));
    // Sort by mtime ascending so existing entries keep stable positions and new
    // saves consistently land at the bottom — no jitter when a pipeline is renamed.
    const withStat = await Promise.all(files.map(async (f) => {
      try {
        const stat = await fs.stat(path.join(dir, f.name));
        return { name: f.name, mtime: stat.mtimeMs };
      } catch {
        return { name: f.name, mtime: 0 };
      }
    }));
    withStat.sort((a, b) => a.mtime - b.mtime);
    return withStat.map((f) => ({ name: f.name.replace(/\.json$/, '') }));
  });

  app.get('/:name', async (req, reply) => {
    const { name } = req.params as { name: string };
    const settings = await readSettings();
    const filePath = safeJoin('contentDb', `${PIPELINES_REL}/${sanitizeName(name)}.json`, settings);
    try {
      const raw = await fs.readFile(filePath, 'utf8');
      return JSON.parse(raw) as Pipeline;
    } catch (err: any) {
      if (err?.code === 'ENOENT') {
        reply.code(404);
        return { error: 'Not found' };
      }
      throw err;
    }
  });

  app.put('/:name', async (req, reply) => {
    const { name } = req.params as { name: string };
    const body = req.body as Pipeline;
    if (!body || typeof body !== 'object' || !Array.isArray(body.nodes) || !Array.isArray(body.edges)) {
      reply.code(400);
      return { error: 'Invalid pipeline body' };
    }
    const safeName = sanitizeName(name);
    const settings = await readSettings();
    const filePath = safeJoin('contentDb', `${PIPELINES_REL}/${safeName}.json`, settings);
    await ensureDirFor(filePath);

    const toSave: Pipeline = {
      schemaVersion: 1,
      id: body.id || uuid(),
      name: safeName,
      nodes: body.nodes,
      edges: body.edges,
    };
    const tmp = filePath + '.tmp';
    await fs.writeFile(tmp, JSON.stringify(toSave, null, 2), 'utf8');
    await fs.rename(tmp, filePath);
    return toSave;
  });

  app.delete('/:name', async (req, reply) => {
    const { name } = req.params as { name: string };
    const settings = await readSettings();
    const filePath = safeJoin('contentDb', `${PIPELINES_REL}/${sanitizeName(name)}.json`, settings);
    try {
      await fs.unlink(filePath);
      return { ok: true };
    } catch (err: any) {
      if (err?.code === 'ENOENT') {
        reply.code(404);
        return { error: 'Not found' };
      }
      throw err;
    }
  });
}
