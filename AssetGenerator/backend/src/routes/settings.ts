import type { FastifyInstance } from 'fastify';
import { readSettings, writeSettings } from '../config/settings.js';
import type { AppSettings } from '../../../shared/types.js';
import { DEFAULT_SETTINGS } from '../../../shared/types.js';

export async function settingsRoutes(app: FastifyInstance): Promise<void> {
  app.get('/', async () => readSettings());

  app.put('/', async (req, reply) => {
    const body = req.body as Partial<AppSettings> | undefined;
    if (!body || typeof body !== 'object') {
      reply.code(400);
      return { error: 'Invalid body' };
    }
    const next: AppSettings = {
      apiKeys: { ...DEFAULT_SETTINGS.apiKeys, ...(body.apiKeys ?? {}) },
      directories: { ...DEFAULT_SETTINGS.directories, ...(body.directories ?? {}) },
    };
    await writeSettings(next);
    return next;
  });
}
