import type { AppSettings, ExecuteRequest, ExecutionEvent, NodeTypeSchema, Pipeline } from '../../../shared/types.js';

async function jsonReq<T>(input: string, init?: RequestInit): Promise<T> {
  const hasBody = init?.body != null;
  const headers: Record<string, string> = { ...(init?.headers as Record<string, string> | undefined ?? {}) };
  // Only declare JSON content-type when actually sending a body.
  // Fastify rejects e.g. DELETE with `content-type: application/json` + empty body.
  if (hasBody && !headers['content-type']) headers['content-type'] = 'application/json';
  const res = await fetch(input, { ...init, headers });
  if (!res.ok) {
    const text = await res.text();
    throw new Error(`HTTP ${res.status}: ${text}`);
  }
  return res.json() as Promise<T>;
}

export const api = {
  getSettings(): Promise<AppSettings> {
    return jsonReq('/api/settings');
  },
  putSettings(s: AppSettings): Promise<AppSettings> {
    return jsonReq('/api/settings', { method: 'PUT', body: JSON.stringify(s) });
  },
  listNodeTypes(): Promise<NodeTypeSchema[]> {
    return jsonReq('/api/node-types');
  },
  listPipelines(): Promise<{ name: string }[]> {
    return jsonReq('/api/pipelines');
  },
  getPipeline(name: string): Promise<Pipeline> {
    return jsonReq(`/api/pipelines/${encodeURIComponent(name)}`);
  },
  savePipeline(name: string, pipeline: Pipeline): Promise<Pipeline> {
    return jsonReq(`/api/pipelines/${encodeURIComponent(name)}`, {
      method: 'PUT',
      body: JSON.stringify(pipeline),
    });
  },
  deletePipeline(name: string): Promise<void> {
    return jsonReq(`/api/pipelines/${encodeURIComponent(name)}`, { method: 'DELETE' });
  },
  execute(req: ExecuteRequest, onEvent: (e: ExecutionEvent) => void, signal?: AbortSignal): Promise<void> {
    return streamSse('/api/execute', req, onEvent, signal);
  },
  executeSingle(req: { pipeline: import('../../../shared/types.js').Pipeline; nodeId: string }, onEvent: (e: ExecutionEvent) => void, signal?: AbortSignal): Promise<void> {
    return streamSse('/api/execute/single', req, onEvent, signal);
  },
};

async function streamSse(url: string, body: unknown, onEvent: (e: ExecutionEvent) => void, signal?: AbortSignal): Promise<void> {
  const res = await fetch(url, {
    method: 'POST',
    headers: { 'content-type': 'application/json' },
    body: JSON.stringify(body),
    signal,
  });
  if (!res.ok || !res.body) throw new Error(`Request failed: HTTP ${res.status}`);
  const reader = res.body.getReader();
  const decoder = new TextDecoder();
  let buf = '';
  while (true) {
    const { done, value } = await reader.read();
    if (done) break;
    buf += decoder.decode(value, { stream: true });
    let nl: number;
    while ((nl = buf.indexOf('\n\n')) >= 0) {
      const chunk = buf.slice(0, nl);
      buf = buf.slice(nl + 2);
      for (const line of chunk.split('\n')) {
        if (line.startsWith('data:')) {
          try {
            const event = JSON.parse(line.slice(5).trim()) as ExecutionEvent;
            onEvent(event);
          } catch {
            /* swallow malformed */
          }
        }
      }
    }
  }
}
