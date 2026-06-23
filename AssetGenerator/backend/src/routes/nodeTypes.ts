import type { FastifyInstance } from 'fastify';
import { listNodeSchemas } from '../nodes/registry.js';

export async function nodeTypesRoutes(app: FastifyInstance): Promise<void> {
  app.get('/', async () => listNodeSchemas());
}
