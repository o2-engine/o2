import type { NodeImpl, PortValue } from './types.js';
import { effectiveApiKey } from '../config/settings.js';
import { geminiGenerateImage } from '../providers/gemini.js';

export const nanoBananaGenNode: NodeImpl = {
  schema: {
    type: 'nanoBananaGen',
    label: 'Image gen (Nano Banana 2)',
    category: 'ai',
    description: 'Generate an image via Google Gemini image model. Optional reference image input.',
    inputs: [
      { id: 'prompt', name: 'prompt', type: 'text' },
      { id: 'reference', name: 'reference', type: 'image' },
    ],
    outputs: [
      { id: 'out', name: 'out', type: 'image' },
    ],
    configSchema: [
      { key: 'model', label: 'Model', type: 'string', default: 'gemini-3-pro-image-preview' },
      { key: 'extraPrompt', label: 'Extra prompt', type: 'multiline', placeholder: 'Style hints appended after the prompt' },
    ],
  },
  async run(ctx, inputs, node) {
    const promptVal = inputs.get('prompt');
    if (!promptVal) throw new Error('nanoBananaGen: input "prompt" is not connected');
    if (promptVal.type !== 'text') throw new Error('nanoBananaGen: prompt must be text');

    const refVal = inputs.get('reference');
    if (refVal && refVal.type !== 'image') throw new Error('nanoBananaGen: reference must be image');

    const apiKey = effectiveApiKey(ctx.settings, 'gemini');
    const model = String(node.config.model ?? 'gemini-3-pro-image-preview');
    const extra = String(node.config.extraPrompt ?? '');
    const prompt = extra ? `${promptVal.data}\n${extra}` : promptVal.data;

    ctx.log(`nanoBananaGen: model=${model} promptChars=${prompt.length} hasRef=${refVal ? 'yes' : 'no'}`);

    const png = await geminiGenerateImage({
      apiKey,
      prompt,
      model,
      reference: refVal ? { mimeType: refVal.mimeType, data: refVal.data } : undefined,
    });

    const value: PortValue = { type: 'image', data: png, mimeType: 'image/png' };
    return new Map([['out', value]]);
  },
};
