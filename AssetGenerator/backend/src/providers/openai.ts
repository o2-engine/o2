export interface OpenAITextInput {
  apiKey: string;
  prompt: string;
  model?: string;
}

export async function openaiGenerateText(_input: OpenAITextInput): Promise<string> {
  throw new Error('OpenAI provider not yet implemented.');
}
