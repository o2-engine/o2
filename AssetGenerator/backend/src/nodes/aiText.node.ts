import type { NodeImpl, PortValue } from './types.js';
import { effectiveApiKey } from '../config/settings.js';
import { geminiGenerateText } from '../providers/gemini.js';

export const aiTextNode: NodeImpl = {
  schema: {
    type: 'aiText',
    label: 'AI text (Gemini)',
    category: 'ai',
    description: 'Generate text via Google Gemini. Takes a prompt input.',
    inputs: [
      { id: 'prompt', name: 'prompt', type: 'text' },
    ],
    outputs: [
      { id: 'out', name: 'out', type: 'text' },
    ],
    configSchema: [
      { key: 'model', label: 'Model', type: 'string', default: 'gemini-2.5-flash' },
      { key: 'systemPrompt', label: 'System prompt', type: 'multiline', placeholder: 'Optional system instructions' },
    ],
  },
  async run(ctx, inputs, node) {
    const promptVal = inputs.get('prompt');
    if (!promptVal) throw new Error('aiText: input "prompt" is not connected');
    if (promptVal.type !== 'text') throw new Error('aiText: prompt must be text');

    const apiKey = effectiveApiKey(ctx.settings, 'gemini');
    const model = String(node.config.model ?? 'gemini-2.5-flash');
    const systemPrompt = String(node.config.systemPrompt ?? '').trim();
    const prompt = systemPrompt ? `${systemPrompt}\n\n${promptVal.data}` : promptVal.data;

    ctx.log(`aiText: model=${model} promptChars=${prompt.length}`);
    const text = await geminiGenerateText({ apiKey, prompt, model });

    const value: PortValue = { type: 'text', data: text };
    return new Map([['out', value]]);
  },
};
