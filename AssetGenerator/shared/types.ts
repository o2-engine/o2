export type PortType = 'text' | 'image';

export interface PortDef {
  id: string;
  name: string;
  type: PortType;
}

export interface NodeData {
  id: string;
  type: string;
  position: { x: number; y: number };
  size?: { width: number; height: number };
  config: Record<string, unknown>;
  inputs: PortDef[];
  outputs: PortDef[];
}

export interface EdgeData {
  id: string;
  fromNodeId: string;
  fromPortId: string;
  toNodeId: string;
  toPortId: string;
}

export interface Pipeline {
  schemaVersion: 1;
  id: string;
  name: string;
  nodes: NodeData[];
  edges: EdgeData[];
}

export interface NodeTypeSchema {
  type: string;
  label: string;
  category: 'flow' | 'source' | 'transform' | 'ai' | 'output';
  description: string;
  inputs: PortDef[];
  outputs: PortDef[];
  configSchema: ConfigFieldSchema[];
  hasPlay?: boolean;
}

export interface ConfigFieldSchema {
  key: string;
  label: string;
  type: 'string' | 'multiline' | 'select' | 'filePath' | 'enum' | 'keyValueMap';
  options?: string[];
  default?: unknown;
  placeholder?: string;
}

export type AppSettings = {
  apiKeys: {
    gemini: string;
    claude: string;
    openai: string;
  };
  directories: {
    assets: string;
    contentDatabase: string;
  };
};

export const DEFAULT_SETTINGS: AppSettings = {
  apiKeys: { gemini: '', claude: '', openai: '' },
  directories: { assets: 'Assets', contentDatabase: 'ContentDatabase' },
};

export interface ExecuteRequest {
  pipeline: Pipeline;
  targetNodeId: string;
}

export type ExecutionEvent =
  | { type: 'node-state'; nodeId: string; state: 'running' | 'done' | 'error'; error?: string }
  | { type: 'node-output'; nodeId: string; mediaType: 'image' | 'text'; url?: string; text?: string }
  | { type: 'log'; nodeId?: string; message: string }
  | { type: 'done' }
  | { type: 'fatal'; error: string };
