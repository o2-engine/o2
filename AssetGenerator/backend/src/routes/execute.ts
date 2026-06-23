import type { FastifyInstance, FastifyReply } from 'fastify';
import type { ExecuteRequest, ExecutionEvent, Pipeline } from '../../../shared/types.js';
import { executePipeline, executeSingleNode } from '../pipeline/executor.js';

function openSseStream(reply: FastifyReply): { emit: (event: ExecutionEvent) => void; ac: AbortController } {
  reply.raw.writeHead(200, {
    'Content-Type': 'text/event-stream',
    'Cache-Control': 'no-cache, no-transform',
    Connection: 'keep-alive',
    'X-Accel-Buffering': 'no',
  });
  const ac = new AbortController();
  // ServerResponse 'close' fires when the underlying socket closes (client disconnects
  // or we end() the response). Safe either way — if the work already finished, abort is a no-op.
  reply.raw.on('close', () => ac.abort());
  return {
    emit: (event: ExecutionEvent) => {
      try { reply.raw.write(`data: ${JSON.stringify(event)}\n\n`); } catch { /* socket closed */ }
    },
    ac,
  };
}

export async function executeRoutes(app: FastifyInstance): Promise<void> {
  app.post('/', async (req, reply) => {
    const body = req.body as ExecuteRequest | undefined;
    if (!body || !body.pipeline || !body.targetNodeId) {
      reply.code(400);
      return { error: 'pipeline and targetNodeId required' };
    }
    const { emit, ac } = openSseStream(reply);
    try {
      await executePipeline(body.pipeline, body.targetNodeId, emit, ac.signal);
    } catch (err: any) {
      emit({ type: 'fatal', error: String(err?.message ?? err) });
    } finally {
      try { reply.raw.end(); } catch { /* closed */ }
    }
  });

  app.post('/single', async (req, reply) => {
    const body = req.body as { pipeline?: Pipeline; nodeId?: string } | undefined;
    if (!body || !body.pipeline || !body.nodeId) {
      reply.code(400);
      return { error: 'pipeline and nodeId required' };
    }
    const { emit, ac } = openSseStream(reply);
    try {
      await executeSingleNode(body.pipeline, body.nodeId, emit, ac.signal);
    } catch (err: any) {
      emit({ type: 'fatal', error: String(err?.message ?? err) });
    } finally {
      try { reply.raw.end(); } catch { /* closed */ }
    }
  });
}
