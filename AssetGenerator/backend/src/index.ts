import Fastify from 'fastify';
import cors from '@fastify/cors';
import { settingsRoutes } from './routes/settings.js';
import { pipelinesRoutes } from './routes/pipelines.js';
import { assetsRoutes } from './routes/assets.js';
import { executeRoutes } from './routes/execute.js';
import { nodeTypesRoutes } from './routes/nodeTypes.js';
import { cacheRoutes } from './routes/cache.js';

const app = Fastify({
  logger: { level: 'info' },
  bodyLimit: 64 * 1024 * 1024,
});

await app.register(cors, { origin: true });

await app.register(settingsRoutes, { prefix: '/api/settings' });
await app.register(pipelinesRoutes, { prefix: '/api/pipelines' });
await app.register(assetsRoutes, { prefix: '/api/files' });
await app.register(executeRoutes, { prefix: '/api/execute' });
await app.register(nodeTypesRoutes, { prefix: '/api/node-types' });
await app.register(cacheRoutes, { prefix: '/api/cache' });

app.get('/api/health', async () => ({ ok: true }));

const port = Number(process.env.PORT ?? 8765);
try {
  await app.listen({ port, host: '0.0.0.0' });
} catch (err) {
  app.log.error(err);
  process.exit(1);
}
