import type { FastifyInstance } from 'fastify';
import fs from 'node:fs/promises';
import path from 'node:path';
import { readSettings } from '../config/settings.js';
import { listDir, safeJoin, ensureDirFor } from '../utils/files.js';

type RootKey = 'assets' | 'contentDb';

export async function assetsRoutes(app: FastifyInstance): Promise<void> {
  app.get('/list', async (req, reply) => {
    const { root, path: rel = '' } = (req.query ?? {}) as { root?: RootKey; path?: string };
    if (root !== 'assets' && root !== 'contentDb') {
      reply.code(400);
      return { error: 'root must be assets or contentDb' };
    }
    const settings = await readSettings();
    const dir = safeJoin(root, rel, settings);
    const entries = await listDir(dir);
    return { root, path: rel, entries };
  });

  app.get('/read', async (req, reply) => {
    const { root, path: rel } = (req.query ?? {}) as { root?: RootKey; path?: string };
    if (root !== 'assets' && root !== 'contentDb') {
      reply.code(400);
      return { error: 'root must be assets or contentDb' };
    }
    if (!rel) {
      reply.code(400);
      return { error: 'path required' };
    }
    const settings = await readSettings();
    const filePath = safeJoin(root, rel, settings);
    try {
      const stat = await fs.stat(filePath);
      if (stat.isDirectory()) {
        reply.code(400);
        return { error: 'path is a directory' };
      }
      const buf = await fs.readFile(filePath);
      const ext = path.extname(filePath).toLowerCase();
      const mime =
        ext === '.png' ? 'image/png' :
        ext === '.jpg' || ext === '.jpeg' ? 'image/jpeg' :
        ext === '.webp' ? 'image/webp' :
        ext === '.gif' ? 'image/gif' :
        'application/octet-stream';
      reply.header('content-type', mime);
      return reply.send(buf);
    } catch (err: any) {
      if (err?.code === 'ENOENT') {
        reply.code(404);
        return { error: 'Not found' };
      }
      throw err;
    }
  });

  app.put('/write', async (req, reply) => {
    const body = req.body as { root?: RootKey; path?: string; contentBase64?: string };
    if (!body || (body.root !== 'assets' && body.root !== 'contentDb') || !body.path || typeof body.contentBase64 !== 'string') {
      reply.code(400);
      return { error: 'root, path, contentBase64 required' };
    }
    const settings = await readSettings();
    const filePath = safeJoin(body.root, body.path, settings);
    await ensureDirFor(filePath);
    const buf = Buffer.from(body.contentBase64, 'base64');
    await fs.writeFile(filePath, buf);
    return { ok: true, bytes: buf.length };
  });
}
