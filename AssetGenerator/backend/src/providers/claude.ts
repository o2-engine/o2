export interface ClaudeTextInput {
  apiKey: string;
  prompt: string;
  model?: string;
}

export async function claudeGenerateText(_input: ClaudeTextInput): Promise<string> {
  throw new Error('Claude provider not yet implemented.');
}
