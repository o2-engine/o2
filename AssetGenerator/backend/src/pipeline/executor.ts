import fs from 'node:fs/promises';
import crypto from 'node:crypto';
import type { AppSettings, ExecutionEvent, NodeData, Pipeline } from '../../../shared/types.js';
import { getNodeImpl } from '../nodes/registry.js';
import type { PortValue } from '../nodes/types.js';
import { readSettings } from '../config/settings.js';
import { validatePipeline } from './validate.js';
import {
  cacheFilePath, cacheUrlFor, ensureCacheDir,
  contentCachePath, ensureContentCacheDir,
} from '../routes/cache.js';
import { safeJoin } from '../utils/files.js';

const FINISH_TYPES = new Set(['finishText', 'finishImage']);
const SOURCE_TYPES = new Set(['sourceText', 'sourceTextFile', 'sourceImage']);

class CancelledError extends Error {
  constructor() { super('Cancelled'); this.name = 'CancelledError'; }
}

function checkAborted(signal?: AbortSignal): void {
  if (signal?.aborted) throw new CancelledError();
}

function canonicalize(v: unknown): unknown {
  if (Array.isArray(v)) return v.map(canonicalize);
  if (v && typeof v === 'object') {
    const obj: Record<string, unknown> = {};
    for (const k of Object.keys(v as object).sort()) {
      obj[k] = canonicalize((v as Record<string, unknown>)[k]);
    }
    return obj;
  }
  return v;
}

async function computeSignature(
  node: NodeData,
  upstreamSigsByPortName: Record<string, string>,
  settings: AppSettings,
): Promise<string> {
  let extra = '';
  if (node.type === 'sourceImage') {
    try {
      const root = (node.config.sourceRoot as 'assets' | 'contentDb') ?? 'contentDb';
      const rel = String(node.config.filePath ?? '').trim();
      if (rel) {
        const abs = safeJoin(root, rel, settings);
        const stat = await fs.stat(abs);
        extra = `|file:${stat.mtimeMs}:${stat.size}`;
      }
    } catch { extra = '|file:missing'; }
  }
  const sortedInputs = Object.entries(upstreamSigsByPortName).sort(([a], [b]) => a.localeCompare(b));
  const payload = JSON.stringify({
    type: node.type,
    config: canonicalize(node.config),
    inputs: sortedInputs,
  }) + extra;
  return crypto.createHash('sha256').update(payload).digest('hex').slice(0, 32);
}

async function cacheNodeOutput(
  nodeId: string,
  source: 'output' | 'input',
  value: PortValue,
  emit: (event: ExecutionEvent) => void,
): Promise<void> {
  if (value.type === 'image') {
    await ensureCacheDir();
    const file = cacheFilePath(nodeId, '.png');
    await fs.writeFile(file, value.data);
    emit({ type: 'node-output', nodeId, mediaType: 'image', url: cacheUrlFor(nodeId, '.png') });
  } else {
    await ensureCacheDir();
    const file = cacheFilePath(nodeId, '.txt');
    await fs.writeFile(file, value.data, 'utf8');
    emit({
      type: 'node-output', nodeId, mediaType: 'text', url: cacheUrlFor(nodeId, '.txt'),
      text: value.data.length > 2000 ? value.data.slice(0, 2000) + '…' : value.data,
    });
  }
  void source;
}

async function loadContentCache(sig: string, type: 'text' | 'image'): Promise<PortValue | null> {
  const ext = type === 'image' ? '.png' : '.txt';
  const file = contentCachePath(sig, ext);
  try {
    if (type === 'image') {
      const data = await fs.readFile(file);
      return { type: 'image', data, mimeType: 'image/png' };
    }
    const data = await fs.readFile(file, 'utf8');
    return { type: 'text', data };
  } catch {
    return null;
  }
}

async function saveContentCache(sig: string, value: PortValue): Promise<void> {
  await ensureContentCacheDir();
  if (value.type === 'image') {
    await fs.writeFile(contentCachePath(sig, '.png'), value.data);
  } else {
    await fs.writeFile(contentCachePath(sig, '.txt'), value.data, 'utf8');
  }
}

async function readCachedPortValue(sourceNodeId: string, type: 'text' | 'image'): Promise<PortValue> {
  const ext = type === 'image' ? '.png' : '.txt';
  const file = cacheFilePath(sourceNodeId, ext);
  try {
    if (type === 'image') {
      const data = await fs.readFile(file);
      return { type: 'image', data, mimeType: 'image/png' };
    }
    const data = await fs.readFile(file, 'utf8');
    return { type: 'text', data };
  } catch {
    throw new Error(`Cached output of upstream node ${sourceNodeId} not found — run the full pipeline first`);
  }
}

export async function executePipeline(
  pipeline: Pipeline,
  targetNodeId: string,
  emit: (event: ExecutionEvent) => void,
  signal?: AbortSignal,
): Promise<void> {
  const errors = validatePipeline(pipeline);
  if (errors.length) {
    emit({ type: 'fatal', error: errors.map((e) => e.message).join('; ') });
    return;
  }

  const settings = await readSettings();
  const nodesById = new Map<string, NodeData>(pipeline.nodes.map((n) => [n.id, n]));
  if (!nodesById.has(targetNodeId)) {
    emit({ type: 'fatal', error: `Target node ${targetNodeId} not found` });
    return;
  }

  const outputs = new Map<string, Map<string, PortValue>>();
  const sigByNode = new Map<string, string>();
  const visiting = new Set<string>();
  const ctx = { settings, log: (msg: string) => emit({ type: 'log', message: msg }) };

  async function evaluate(nodeId: string): Promise<void> {
    checkAborted(signal);
    if (outputs.has(nodeId)) return;
    if (visiting.has(nodeId)) throw new Error(`Cycle detected at node ${nodeId}`);
    visiting.add(nodeId);

    const node = nodesById.get(nodeId)!;
    const incoming = pipeline.edges.filter((e) => e.toNodeId === nodeId);
    for (const e of incoming) await evaluate(e.fromNodeId);

    const inputs = new Map<string, PortValue>();
    const upstreamSigs: Record<string, string> = {};
    for (const e of incoming) {
      const fromOutputs = outputs.get(e.fromNodeId);
      const value = fromOutputs?.get(e.fromPortId);
      const port = node.inputs.find((p) => p.id === e.toPortId);
      if (!port) throw new Error(`Edge target port ${e.toPortId} not found on node ${node.type}`);
      if (value) inputs.set(port.name, value);
      const upstreamSig = sigByNode.get(e.fromNodeId);
      if (upstreamSig) upstreamSigs[port.name] = upstreamSig;
    }

    const sig = await computeSignature(node, upstreamSigs, settings);
    sigByNode.set(nodeId, sig);

    const isFinish = FINISH_TYPES.has(node.type);
    const cacheable = !isFinish && node.outputs.length === 1;

    if (cacheable) {
      const outPort = node.outputs[0];
      const cached = await loadContentCache(sig, outPort.type);
      if (cached) {
        const byPortId = new Map<string, PortValue>([[outPort.id, cached]]);
        outputs.set(nodeId, byPortId);
        emit({ type: 'node-state', nodeId, state: 'running' });
        await cacheNodeOutput(nodeId, 'output', cached, emit);
        ctx.log(`${node.type}: cache hit (${sig.slice(0, 8)})`);
        emit({ type: 'node-state', nodeId, state: 'done' });
        visiting.delete(nodeId);
        return;
      }
    }

    emit({ type: 'node-state', nodeId, state: 'running' });
    try {
      checkAborted(signal);
      const impl = getNodeImpl(node.type);
      const result = await impl.run(ctx, inputs, node);
      checkAborted(signal);

      const byPortId = new Map<string, PortValue>();
      for (const [name, value] of result.entries()) {
        const port = node.outputs.find((p) => p.name === name);
        if (!port) throw new Error(`Node ${node.type} produced unknown output "${name}"`);
        byPortId.set(port.id, value);
      }
      outputs.set(nodeId, byPortId);

      if (cacheable) {
        const outPort = node.outputs[0];
        const value = byPortId.get(outPort.id);
        if (value) await saveContentCache(sig, value);
      }

      const previewValue = isFinish
        ? (inputs.size ? inputs.values().next().value : undefined)
        : (byPortId.size ? byPortId.values().next().value : undefined);
      if (previewValue) await cacheNodeOutput(nodeId, isFinish ? 'input' : 'output', previewValue, emit);

      emit({ type: 'node-state', nodeId, state: 'done' });
    } catch (err: any) {
      if (err instanceof CancelledError) throw err;
      emit({ type: 'node-state', nodeId, state: 'error', error: String(err?.message ?? err) });
      throw err;
    } finally {
      visiting.delete(nodeId);
    }
  }

  try {
    await evaluate(targetNodeId);
    emit({ type: 'done' });
  } catch (err: any) {
    if (err instanceof CancelledError) {
      emit({ type: 'fatal', error: 'Cancelled' });
    } else {
      emit({ type: 'fatal', error: String(err?.message ?? err) });
    }
  }
  void SOURCE_TYPES;
}

export async function executeSingleNode(
  pipeline: Pipeline,
  nodeId: string,
  emit: (event: ExecutionEvent) => void,
  signal?: AbortSignal,
): Promise<void> {
  const settings = await readSettings();
  const nodesById = new Map<string, NodeData>(pipeline.nodes.map((n) => [n.id, n]));
  const node = nodesById.get(nodeId);
  if (!node) {
    emit({ type: 'fatal', error: `Node ${nodeId} not found` });
    return;
  }

  const ctx = { settings, log: (msg: string) => emit({ type: 'log', message: msg }) };
  const inputs = new Map<string, PortValue>();

  try {
    const incoming = pipeline.edges.filter((e) => e.toNodeId === nodeId);
    for (const e of incoming) {
      checkAborted(signal);
      const sourceNode = nodesById.get(e.fromNodeId);
      if (!sourceNode) continue;
      const sourcePort = sourceNode.outputs.find((p) => p.id === e.fromPortId);
      if (!sourcePort) continue;
      const targetPort = node.inputs.find((p) => p.id === e.toPortId);
      if (!targetPort) continue;
      const value = await readCachedPortValue(e.fromNodeId, sourcePort.type);
      inputs.set(targetPort.name, value);
    }
  } catch (err: any) {
    if (err instanceof CancelledError) { emit({ type: 'fatal', error: 'Cancelled' }); return; }
    emit({ type: 'node-state', nodeId, state: 'error', error: String(err?.message ?? err) });
    emit({ type: 'fatal', error: String(err?.message ?? err) });
    return;
  }

  emit({ type: 'node-state', nodeId, state: 'running' });
  try {
    checkAborted(signal);
    const impl = getNodeImpl(node.type);
    const result = await impl.run(ctx, inputs, node);
    checkAborted(signal);

    const byPortId = new Map<string, PortValue>();
    for (const [name, value] of result.entries()) {
      const port = node.outputs.find((p) => p.name === name);
      if (!port) throw new Error(`Node ${node.type} produced unknown output "${name}"`);
      byPortId.set(port.id, value);
    }

    const isFinish = FINISH_TYPES.has(node.type);
    const previewValue = isFinish
      ? (inputs.size ? inputs.values().next().value : undefined)
      : (byPortId.size ? byPortId.values().next().value : undefined);
    if (previewValue) await cacheNodeOutput(nodeId, isFinish ? 'input' : 'output', previewValue, emit);

    emit({ type: 'node-state', nodeId, state: 'done' });
    emit({ type: 'done' });
  } catch (err: any) {
    if (err instanceof CancelledError) {
      emit({ type: 'fatal', error: 'Cancelled' });
      return;
    }
    emit({ type: 'node-state', nodeId, state: 'error', error: String(err?.message ?? err) });
    emit({ type: 'fatal', error: String(err?.message ?? err) });
  }
}
