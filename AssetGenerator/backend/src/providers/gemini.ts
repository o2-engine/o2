import { GoogleGenAI } from '@google/genai';

export interface GenerateImageInput {
  apiKey: string;
  prompt: string;
  reference?: { mimeType: string; data: Buffer };
  model?: string;
}

export interface GenerateTextInput {
  apiKey: string;
  prompt: string;
  model?: string;
}

const DEFAULT_IMAGE_MODEL = 'gemini-2.5-flash-image';
const DEFAULT_TEXT_MODEL = 'gemini-2.5-flash';

function client(apiKey: string): GoogleGenAI {
  if (!apiKey) throw new Error('Gemini API key is not configured (Settings → API keys).');
  return new GoogleGenAI({ apiKey });
}

const RETRY_STATUSES = new Set([429, 500, 502, 503, 504]);
const MAX_ATTEMPTS = 4;

function extractStatus(err: any): number | undefined {
  return err?.status ?? err?.code ?? err?.response?.status;
}

async function callWithRetry<T>(model: string, op: string, fn: () => Promise<T>): Promise<T> {
  let lastErr: any;
  for (let attempt = 1; attempt <= MAX_ATTEMPTS; attempt++) {
    try {
      return await fn();
    } catch (err) {
      lastErr = err;
      const status = extractStatus(err);
      const retryable = status !== undefined && RETRY_STATUSES.has(status);
      if (!retryable || attempt === MAX_ATTEMPTS) throw err;
      const delayMs = 800 * Math.pow(2, attempt - 1);  // 800ms, 1.6s, 3.2s
      // eslint-disable-next-line no-console
      console.warn(`[gemini] ${op} ${model} attempt ${attempt}/${MAX_ATTEMPTS} got ${status}, retrying in ${delayMs}ms`);
      await new Promise((r) => setTimeout(r, delayMs));
    }
  }
  throw lastErr;
}

function wrapError(model: string, op: string, err: any): Error {
  const status = err?.status ?? err?.code ?? err?.response?.status;
  const causeMsg = err?.cause ? (err.cause?.message ?? String(err.cause)) : '';
  const detail = err?.response?.body ? (typeof err.response.body === 'string' ? err.response.body : JSON.stringify(err.response.body)) : '';
  const parts = [
    `${op} failed for model ${model}`,
    err?.message ? `: ${err.message}` : '',
    status ? ` [${status}]` : '',
    causeMsg ? `\nCause: ${causeMsg}` : '',
    detail ? `\nBody: ${detail.slice(0, 400)}` : '',
  ];
  // Surface in backend logs too so user can `docker compose logs backend`
  // eslint-disable-next-line no-console
  console.error(`[gemini] ${op} error (model=${model}):`, err);
  return new Error(parts.join(''));
}

export async function geminiGenerateText(input: GenerateTextInput): Promise<string> {
  const ai = client(input.apiKey);
  const model = input.model ?? DEFAULT_TEXT_MODEL;
  let res: any;
  try {
    res = await callWithRetry(model, 'Gemini text call', () => ai.models.generateContent({
      model,
      contents: [{ role: 'user', parts: [{ text: input.prompt }] }],
    }));
  } catch (err) {
    throw wrapError(model, 'Gemini text call', err);
  }

  const candidates = (res as any)?.candidates ?? [];
  const parts = candidates[0]?.content?.parts ?? [];
  const out = parts.map((p: any) => p.text ?? '').join('').trim();
  if (!out) throw new Error(`Gemini returned no text (model ${model}).`);
  return out;
}

export async function geminiGenerateImage(input: GenerateImageInput): Promise<Buffer> {
  const ai = client(input.apiKey);
  const model = input.model ?? DEFAULT_IMAGE_MODEL;

  // Imagen models use a different endpoint
  if (model.toLowerCase().startsWith('imagen')) {
    return imagenGenerate(ai, model, input);
  }

  const parts: any[] = [{ text: input.prompt }];
  if (input.reference) {
    parts.push({
      inlineData: {
        mimeType: input.reference.mimeType,
        data: input.reference.data.toString('base64'),
      },
    });
  }

  let res: any;
  try {
    res = await callWithRetry(model, 'Gemini image call', () => ai.models.generateContent({
      model,
      contents: [{ role: 'user', parts }],
    }));
  } catch (err) {
    throw wrapError(model, 'Gemini image call', err);
  }

  const candidates = (res as any)?.candidates ?? [];
  const outParts = candidates[0]?.content?.parts ?? [];
  for (const p of outParts) {
    const inline = p.inlineData ?? p.inline_data;
    if (inline?.data) {
      return Buffer.from(inline.data, 'base64');
    }
  }

  // If the model returned text instead of an image, surface that — it usually
  // means a wrong model id or content policy refusal.
  const textParts = outParts.map((p: any) => p.text).filter(Boolean).join('\n');
  if (textParts) {
    throw new Error(`Gemini returned text instead of an image (model ${model}). Try gemini-2.5-flash-image or gemini-2.5-flash-image-preview.\nMessage: ${textParts.slice(0, 300)}`);
  }
  throw new Error(`Gemini returned no image data (model ${model}). Response keys: ${Object.keys(res ?? {}).join(', ')}`);
}

async function imagenGenerate(ai: GoogleGenAI, model: string, input: GenerateImageInput): Promise<Buffer> {
  let res: any;
  try {
    res = await callWithRetry(model, 'Imagen call', () => (ai.models as any).generateImages({
      model,
      prompt: input.prompt,
      config: { numberOfImages: 1, aspectRatio: '1:1' },
    }));
  } catch (err) {
    throw wrapError(model, 'Imagen call', err);
  }
  const gen = res?.generatedImages?.[0];
  const data = gen?.image?.imageBytes;
  if (data) return Buffer.from(data, 'base64');
  throw new Error(`Imagen returned no image data (model ${model}).`);
}
